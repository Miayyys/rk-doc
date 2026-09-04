# MailMsg V1 revision 7 freeze

- Event date: 2026-09-01 (exact time not recorded)
- Scope: RAM-only RK3588 Linux + CPU3 Zephyr validation
- Linux: 5.10.252, seven Linux CPUs
- Zephyr image slot: 49,152 bytes (`0xc000`)
- Notification path: mailbox0 channels 0–3
- Service path: ISR pending bitmap → semaphore → cooperative worker

## Verified profiles

- normal: SESSION_READY, p0–p3 PING/PONG, seven-frame p3 single-doorbell drain,
  STOP_REFUSED, continued data-plane operation
- controlled-stop: STOP_READY, CPU3 OFF, OFFLINE, rearm, second session
- start-timeout: terminal `-ETIMEDOUT`, data plane closed
- stop-timeout: terminal `-ETIMEDOUT`, CPU3 remains ON, data plane closed

The FIT and four padded Zephyr binaries are the exact files used for the
2026-09-01 board validation.  See `artifact-manifest.txt` for upload hashes.
The three userspace test binaries are unchanged from revision 6.

## Source identity

- Main repository: `c4267d9` (`mailmsg: add event-driven Zephyr worker`)
- Nested kernel repository: `3944bf4a7` (`soc: rockchip: expose MailMsg event worker telemetry`)
- The nested kernel commit is preserved as
  `source-patches/0001-soc-rockchip-expose-MailMsg-event-worker-telemetry.patch`;
  it has not been pushed to a personal kernel remote.

## Evidence boundary

The four profile results are recorded in the repository experiment/status
documents.  Raw serial logs for these runs were not captured into this freeze
directory, so this package must not be described as an independently
replayable raw-log archive.  `SHA256SUMS` covers every frozen file except
itself.

This freeze does not claim throughput, latency, long-duration stress, RKLLM
coexistence, or persistent eMMC installation validation.
