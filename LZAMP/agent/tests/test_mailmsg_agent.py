import contextlib
import io
import json
import os
import struct
import sys
import unittest
from unittest import mock


sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import mailmsg_agent


class DecisionTest(unittest.TestCase):
    def test_exact_decision(self):
        decision = mailmsg_agent.parse_decision(
            '{"name":"zephyr_increment","arguments":{"value":41}}'
        )
        self.assertEqual(decision["arguments"]["value"], 41)

    def test_tool_call_wrapper(self):
        decision = mailmsg_agent.parse_decision(
            '<tool_call>{"name":"zephyr_increment","arguments":{"value":7}}</tool_call>'
        )
        self.assertEqual(decision["name"], "zephyr_increment")

    def test_native_tool_call_format(self):
        decision = mailmsg_agent.parse_decision(
            "<tool_call>\n"
            "<function=zephyr_increment>\n"
            "<parameter=value>\n"
            "41\n"
            "</parameter>\n"
            "</function>\n"
            "</tool_call>"
        )
        self.assertEqual(decision, {"name": "zephyr_increment", "arguments": {"value": 41}})

    def test_native_tool_call_rejects_extra_prose(self):
        with self.assertRaises(mailmsg_agent.AgentError) as caught:
            mailmsg_agent.parse_decision(
                "ready\n<tool_call><function=zephyr_increment>"
                "<parameter=value>41</parameter></function></tool_call>"
            )
        self.assertEqual(caught.exception.code, "invalid_json")

    def test_rejects_unknown_tool(self):
        with self.assertRaises(mailmsg_agent.AgentError) as caught:
            mailmsg_agent.parse_decision(
                '{"name":"run_shell","arguments":{"value":1}}'
            )
        self.assertEqual(caught.exception.code, "tool_not_allowed")

    def test_rejects_extra_argument(self):
        with self.assertRaises(mailmsg_agent.AgentError) as caught:
            mailmsg_agent.parse_decision(
                '{"name":"zephyr_increment","arguments":{"value":1,"command":"id"}}'
            )
        self.assertEqual(caught.exception.code, "invalid_decision")

    def test_rejects_boolean_and_overflow(self):
        for value in (True, -1, 0xFFFFFFFF):
            with self.subTest(value=value):
                with self.assertRaises(mailmsg_agent.AgentError):
                    mailmsg_agent.parse_decision(
                        json.dumps({"name": "zephyr_increment", "arguments": {"value": value}})
                    )


class FrameTest(unittest.TestCase):
    def test_user_record_is_48_bytes(self):
        data = mailmsg_agent.pack_frame(1, mailmsg_agent.MAILMSG_MSG_PING, 41)
        self.assertEqual(len(data), 48)
        priority, message_type, sequence, length, payload = struct.unpack("<IIII32s", data)
        self.assertEqual((priority, message_type, sequence, length), (1, 1, 0, 4))
        self.assertEqual(struct.unpack_from("<I", payload)[0], 41)

    def test_unpack_rejects_wrong_priority(self):
        data = mailmsg_agent.MAILMSG_FRAME.pack(2, 2, 1, 4, struct.pack("<I", 42) + bytes(28))
        with self.assertRaises(mailmsg_agent.AgentError) as caught:
            mailmsg_agent.unpack_frame(data)
        self.assertEqual(caught.exception.code, "wrong_priority")


class ModelRequestTest(unittest.TestCase):
    def test_strict_json_mode_registers_tool_in_prompt_only(self):
        request = mailmsg_agent.build_model_request("rkllm", "increment 41")
        self.assertNotIn("tools", request)
        self.assertEqual(len(request["messages"]), 1)
        self.assertEqual(request["messages"][0]["role"], "user")
        self.assertIn("zephyr_increment", request["messages"][0]["content"])
        self.assertIn("increment 41", request["messages"][0]["content"])

    def test_native_mode_exposes_function_schema(self):
        request = mailmsg_agent.build_model_request(
            "rkllm", "increment 41", native_tools=True
        )
        self.assertEqual(request["tools"], [mailmsg_agent.TOOL_SCHEMA])
        self.assertEqual([m["role"] for m in request["messages"]], ["system", "user"])

    def test_chat_smoke_request_has_no_tools(self):
        request = mailmsg_agent.build_chat_request("rkllm", "Reply with READY")
        self.assertNotIn("tools", request)
        self.assertEqual(
            request["messages"],
            [{"role": "user", "content": "Reply with READY"}],
        )


class MainTest(unittest.TestCase):
    def test_chat_smoke_never_executes_mailmsg(self):
        output = io.StringIO()
        with mock.patch.object(
            mailmsg_agent,
            "request_model_content",
            return_value="READY",
        ), mock.patch.object(
            mailmsg_agent,
            "execute_zephyr_increment",
            side_effect=AssertionError("MailMsg must not execute"),
        ), contextlib.redirect_stdout(output):
            result = mailmsg_agent.main(["--chat-smoke", "Reply with READY"])

        self.assertEqual(result, 0)
        reply = json.loads(output.getvalue())
        self.assertTrue(reply["ok"])
        self.assertFalse(reply["executed"])
        self.assertEqual(reply["response"], "READY")

    def test_validate_only_never_executes_mailmsg(self):
        output = io.StringIO()
        with mock.patch.object(
            mailmsg_agent,
            "execute_zephyr_increment",
            side_effect=AssertionError("MailMsg must not execute"),
        ), contextlib.redirect_stdout(output):
            result = mailmsg_agent.main(
                [
                    "--decision",
                    '{"name":"zephyr_increment","arguments":{"value":41}}',
                    "--validate-only",
                ]
            )

        self.assertEqual(result, 0)
        reply = json.loads(output.getvalue())
        self.assertTrue(reply["ok"])
        self.assertFalse(reply["executed"])
        self.assertEqual(reply["decision"]["arguments"]["value"], 41)

    def test_validate_only_checks_model_output_without_execution(self):
        output = io.StringIO()
        with mock.patch.object(
            mailmsg_agent,
            "request_model_decision",
            return_value='<tool_call>{"name":"zephyr_increment","arguments":{"value":7}}</tool_call>',
        ) as request_decision, mock.patch.object(
            mailmsg_agent,
            "execute_zephyr_increment",
            side_effect=AssertionError("MailMsg must not execute"),
        ), contextlib.redirect_stdout(output):
            result = mailmsg_agent.main(
                ["--prompt", "increment 7", "--native-tools", "--validate-only"]
            )

        self.assertEqual(result, 0)
        request_decision.assert_called_once()
        self.assertEqual(json.loads(output.getvalue())["decision"]["arguments"]["value"], 7)


if __name__ == "__main__":
    unittest.main()
