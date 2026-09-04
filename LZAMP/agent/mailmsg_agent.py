#!/usr/bin/env python3
"""Minimal, allowlisted RKLLM-to-MailMsg tool runner.

The first supported tool is deliberately small: ``zephyr_increment`` sends a
single PING over reliable priority 1 and accepts success only after receiving
both its ACK and the PONG containing ``value + 1``.  Model output and manually
supplied decisions pass through the same strict validator.
"""

import argparse
import errno
import json
import os
import re
import select
import struct
import sys
import time
import urllib.error
import urllib.request


MAILMSG_PRIORITY_CONTROL = 1
MAILMSG_MSG_PING = 1
MAILMSG_MSG_PONG = 2
MAILMSG_MSG_ACK = 3
MAILMSG_MSG_NACK = 4
MAILMSG_FRAME = struct.Struct("<IIII32s")
MAILMSG_PAYLOAD_LIMIT = 28

TOOL_NAME = "zephyr_increment"
TOOL_SCHEMA = {
    "type": "function",
    "function": {
        "name": TOOL_NAME,
        "description": "Ask the Zephyr CPU3 test service to add one to an unsigned 32-bit integer.",
        "parameters": {
            "type": "object",
            "properties": {
                "value": {
                    "type": "integer",
                    "minimum": 0,
                    "maximum": 4294967294,
                }
            },
            "required": ["value"],
            "additionalProperties": False,
        },
    },
}

NATIVE_TOOL_CALL_RE = re.compile(
    r"\A<tool_call>\s*"
    r"<function=(?P<name>[A-Za-z_][A-Za-z0-9_]*)>\s*"
    r"<parameter=(?P<parameter>[A-Za-z_][A-Za-z0-9_]*)>\s*"
    r"(?P<value>[0-9]+)\s*"
    r"</parameter>\s*</function>\s*</tool_call>\Z",
    re.DOTALL,
)


class AgentError(Exception):
    """Expected validation, API, transport, or protocol failure."""

    def __init__(self, code, message, details=None):
        super().__init__(message)
        self.code = code
        self.message = message
        self.details = details or {}

    def as_dict(self):
        result = {"ok": False, "error": self.code, "message": self.message}
        if self.details:
            result["details"] = self.details
        return result


def _strict_object(value, allowed, label):
    if not isinstance(value, dict):
        raise AgentError("invalid_decision", "%s must be an object" % label)
    unknown = sorted(set(value) - set(allowed))
    if unknown:
        raise AgentError(
            "invalid_decision",
            "%s contains unsupported fields" % label,
            {"fields": unknown},
        )


def parse_decision(text):
    """Parse exact JSON or one exact RKLLM native tool-call block."""
    stripped = text.strip()
    native_match = NATIVE_TOOL_CALL_RE.fullmatch(stripped)
    if native_match:
        decision = {
            "name": native_match.group("name"),
            "arguments": {
                native_match.group("parameter"): int(native_match.group("value")),
            },
        }
    else:
        prefix = "<tool_call>"
        suffix = "</tool_call>"
        if stripped.startswith(prefix) and stripped.endswith(suffix):
            stripped = stripped[len(prefix):-len(suffix)].strip()
        try:
            decision = json.loads(stripped)
        except json.JSONDecodeError as exc:
            raise AgentError(
                "invalid_json",
                "model output is not one exact JSON tool call",
                {"line": exc.lineno, "column": exc.colno},
            ) from exc

    _strict_object(decision, ("name", "arguments"), "decision")
    if decision.get("name") != TOOL_NAME:
        raise AgentError(
            "tool_not_allowed",
            "tool is not registered",
            {"name": decision.get("name")},
        )
    arguments = decision.get("arguments")
    _strict_object(arguments, ("value",), "arguments")
    if "value" not in arguments:
        raise AgentError("invalid_arguments", "value is required")
    value = arguments["value"]
    if isinstance(value, bool) or not isinstance(value, int):
        raise AgentError("invalid_arguments", "value must be an integer")
    if value < 0 or value >= 0xFFFFFFFF:
        raise AgentError(
            "invalid_arguments",
            "value must be between 0 and 4294967294",
        )
    return {"name": TOOL_NAME, "arguments": {"value": value}}


def pack_frame(priority, message_type, value):
    payload = struct.pack("<I", value) + bytes(28)
    return MAILMSG_FRAME.pack(priority, message_type, 0, 4, payload)


def unpack_frame(data):
    if len(data) != MAILMSG_FRAME.size:
        raise AgentError(
            "short_frame",
            "MailMsg returned an unexpected record size",
            {"expected": MAILMSG_FRAME.size, "actual": len(data)},
        )
    priority, message_type, sequence, length, payload = MAILMSG_FRAME.unpack(data)
    if priority != MAILMSG_PRIORITY_CONTROL:
        raise AgentError(
            "wrong_priority",
            "received a frame from an unexpected priority",
            {"priority": priority},
        )
    if length > MAILMSG_PAYLOAD_LIMIT:
        raise AgentError(
            "invalid_frame",
            "received payload length exceeds the MailMsg V6 limit",
            {"length": length},
        )
    return {
        "priority": priority,
        "type": message_type,
        "sequence": sequence,
        "length": length,
        "payload": payload[:length],
    }


def _read_u32(payload, offset=0):
    if len(payload) < offset + 4:
        raise AgentError("invalid_frame", "MailMsg payload is too short")
    return struct.unpack_from("<I", payload, offset)[0]


def execute_zephyr_increment(value, device_root="/dev", timeout_ms=1000):
    """Execute exactly one reliable p1 request with at most one in flight."""
    path = os.path.join(device_root, "mailmsg-p1")
    flags = os.O_RDWR | os.O_NONBLOCK
    if hasattr(os, "O_CLOEXEC"):
        flags |= os.O_CLOEXEC
    try:
        fd = os.open(path, flags)
    except OSError as exc:
        raise AgentError(
            "mailmsg_open_failed",
            os.strerror(exc.errno),
            {"path": path, "errno": exc.errno},
        ) from exc

    try:
        request = pack_frame(MAILMSG_PRIORITY_CONTROL, MAILMSG_MSG_PING, value)
        try:
            written = os.write(fd, request)
        except OSError as exc:
            code = "queue_full" if exc.errno == errno.ENOSPC else "mailmsg_write_failed"
            raise AgentError(code, os.strerror(exc.errno), {"errno": exc.errno}) from exc
        if written != MAILMSG_FRAME.size:
            raise AgentError(
                "short_write",
                "MailMsg accepted only part of the request",
                {"expected": MAILMSG_FRAME.size, "actual": written},
            )

        poller = select.poll()
        poller.register(fd, select.POLLIN | select.POLLERR | select.POLLHUP)
        deadline = time.monotonic() + timeout_ms / 1000.0
        ack = None
        pong = None
        while ack is None or pong is None:
            remaining_ms = max(0, int((deadline - time.monotonic()) * 1000))
            if remaining_ms == 0:
                raise AgentError(
                    "timeout",
                    "timed out waiting for Zephyr ACK/PONG",
                    {"ack_received": ack is not None, "pong_received": pong is not None},
                )
            events = poller.poll(remaining_ms)
            if not events:
                continue
            event = events[0][1]
            if event & (select.POLLERR | select.POLLHUP) and not event & select.POLLIN:
                raise AgentError("mailmsg_offline", "MailMsg session went offline")
            try:
                frame = unpack_frame(os.read(fd, MAILMSG_FRAME.size))
            except BlockingIOError:
                continue

            if frame["type"] == MAILMSG_MSG_ACK:
                if frame["length"] < 8:
                    raise AgentError("invalid_ack", "ACK payload is too short")
                status = _read_u32(frame["payload"], 4)
                if status != 0:
                    raise AgentError("ack_failed", "Zephyr returned a nonzero ACK status", {"status": status})
                ack = {"sequence": frame["sequence"], "peer_sequence": _read_u32(frame["payload"])}
            elif frame["type"] == MAILMSG_MSG_NACK:
                reason = _read_u32(frame["payload"], 4) if frame["length"] >= 8 else None
                raise AgentError("nack", "Zephyr rejected the request", {"reason": reason})
            elif frame["type"] == MAILMSG_MSG_PONG:
                result = _read_u32(frame["payload"])
                if result != value + 1:
                    raise AgentError(
                        "unexpected_result",
                        "Zephyr returned the wrong increment result",
                        {"expected": value + 1, "actual": result},
                    )
                pong = {"sequence": frame["sequence"], "value": result}
            else:
                raise AgentError(
                    "unexpected_frame",
                    "received an unexpected MailMsg type",
                    {"type": frame["type"]},
                )
        return {
            "ok": True,
            "tool": TOOL_NAME,
            "arguments": {"value": value},
            "result": {"value": pong["value"]},
            "transport": {
                "priority": MAILMSG_PRIORITY_CONTROL,
                "window": 1,
                "ack_sequence": ack["sequence"],
                "ack_peer_sequence": ack["peer_sequence"],
                "pong_sequence": pong["sequence"],
            },
        }
    finally:
        os.close(fd)


def build_model_request(model, prompt, native_tools=False):
    if native_tools:
        system_prompt = (
            "Choose exactly one registered tool. Return only one "
            "<tool_call>{JSON}</tool_call> block and no prose."
        )
    else:
        system_prompt = (
            "You are a tool decision router. The only registered tool is "
            "zephyr_increment(value), where value is an integer from 0 through "
            "4294967294. Return exactly one JSON object with this shape and no "
            "other text: {\"name\":\"zephyr_increment\","
            "\"arguments\":{\"value\":INTEGER}}"
        )
    if native_tools:
        messages = [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": prompt},
        ]
    else:
        # The vendor RKLLM Flask example forwards only the newest user/tool
        # message when native tools are absent.  Put the complete policy in
        # that message so the model actually receives it.
        messages = [
            {
                "role": "user",
                "content": "%s\nRequest: %s" % (system_prompt, prompt),
            }
        ]

    body = {
        "model": model,
        "messages": messages,
        "stream": False,
        "temperature": 0.0,
        "top_k": 1,
        "max_tokens": 128,
        "enable_thinking": False,
    }
    if native_tools:
        body["tools"] = [TOOL_SCHEMA]
    return body


def build_chat_request(model, prompt):
    return {
        "model": model,
        "messages": [{"role": "user", "content": prompt}],
        "stream": False,
        "temperature": 0.0,
        "top_k": 1,
        "max_tokens": 128,
        "enable_thinking": False,
    }


def request_model_content(server, body, timeout_s):
    url = server.rstrip("/") + "/v1/chat/completions"
    request = urllib.request.Request(
        url,
        data=json.dumps(body).encode("utf-8"),
        headers={"Content-Type": "application/json", "Authorization": "not_required"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout_s) as response:
            reply = json.load(response)
    except (OSError, urllib.error.URLError, json.JSONDecodeError) as exc:
        raise AgentError("model_api_failed", str(exc), {"url": url}) from exc
    try:
        return reply["choices"][0]["message"]["content"]
    except (KeyError, IndexError, TypeError) as exc:
        raise AgentError("invalid_model_response", "model API response has no assistant content") from exc


def request_model_decision(server, model, prompt, timeout_s, native_tools=False):
    body = build_model_request(model, prompt, native_tools)
    return request_model_content(server, body, timeout_s)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--decision", help="exact JSON or one <tool_call> block")
    source.add_argument("--prompt", help="ask the RKLLM API to choose the tool call")
    source.add_argument(
        "--chat-smoke",
        metavar="PROMPT",
        help="request plain text only; never validate or execute a tool",
    )
    parser.add_argument("--server", default="http://127.0.0.1:8080")
    parser.add_argument("--model", default="rkllm")
    parser.add_argument("--device-root", default="/dev")
    parser.add_argument("--timeout-ms", type=int, default=1000)
    parser.add_argument("--api-timeout", type=float, default=120.0)
    parser.add_argument(
        "--native-tools",
        action="store_true",
        help="use the server's native Function Calling template (requires a compatible model)",
    )
    parser.add_argument(
        "--validate-only",
        action="store_true",
        help="validate the model decision without opening or writing a MailMsg device",
    )
    args = parser.parse_args(argv)

    try:
        if args.chat_smoke is not None:
            raw = request_model_content(
                args.server,
                build_chat_request(args.model, args.chat_smoke),
                args.api_timeout,
            )
            if not isinstance(raw, str) or not raw.strip():
                raise AgentError("empty_model_response", "model returned no text")
            print(
                json.dumps(
                    {"ok": True, "executed": False, "response": raw},
                    ensure_ascii=False,
                    separators=(",", ":"),
                )
            )
            return 0

        raw = args.decision
        if raw is None:
            raw = request_model_decision(
                args.server, args.model, args.prompt, args.api_timeout, args.native_tools
            )
        decision = parse_decision(raw)
        if args.validate_only:
            print(
                json.dumps(
                    {"ok": True, "executed": False, "decision": decision},
                    ensure_ascii=False,
                    separators=(",", ":"),
                )
            )
            return 0
        result = execute_zephyr_increment(
            decision["arguments"]["value"], args.device_root, args.timeout_ms
        )
        result["decision"] = decision
        print(json.dumps(result, ensure_ascii=False, separators=(",", ":")))
        return 0
    except AgentError as exc:
        print(json.dumps(exc.as_dict(), ensure_ascii=False, separators=(",", ":")), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
