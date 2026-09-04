# LZAMP

LZAMP is the maintained RK3588 asymmetric-multiprocessing project extracted
from the earlier board-bring-up experiments. Linux keeps seven application
cores and the NPU; Zephyr owns A55 CPU3 (MPIDR 0x300). MailMsg provides four
priority queues in shared memory and uses Rockchip mailbox0 channels 0-3 only
as notifications.

The frozen baseline is the RAM-only Linux 5.10.252 / MailMsg V1 R7 prototype.
The current Rockchip develop-6.12 target is pinned in
manifests/sources.lock.yaml. Its core path has passed RAM-only validation:
Linux boots with seven cores, Zephyr owns A55 CPU3, four MailMsg priorities
work, controlled stop/rearm reaches a second session, and RKNPU/RKLLM can call
a Zephyr tool through MailMsg. The 6.12 R7 FIT is installed in the eMMC boot
partition and has completed one default-boot regression; an intermittent early
userspace boot stall and full board peripheral coverage remain open risks.

## Layout

- protocol/mailmsg: transport-independent MailMsg protocol and endpoint code
- firmware/zephyr: CPU3 application, profiles, and bring-up diagnostics
- linux/patches: reproducible 5.10 and 6.12 integration patches
- tests: host unit tests and board-side lifecycle tests
- tools: Linux userspace clients and benchmarks
- scripts/lzamp-runtime: explicit Zephyr and RKLLM lifecycle launcher
- validation/r7: frozen R7 metadata and hashes; large binaries are not tracked
- third_party: ignored upstream source checkouts
- build: ignored generated output

## Quick host check

Run make test from this directory. The tests compile into build/host-tests and
exercise the protocol, endpoint, notification abstraction, and mailbox mapping.

## Rebuild the Linux 6.12 candidate

Place the pinned Rockchip kernel checkout at `third_party/linux-rockchip`, then
run these commands from the LZAMP directory:

```sh
make linux-6.12-prepare
make linux-6.12-configure
make linux-6.12-build
```

The prepare step refuses a dirty tree or a commit other than the one recorded
in `manifests/sources.lock.yaml`. It applies the tracked DTS/Kbuild patches;
the kernel wrappers compile the canonical driver sources under `linux/drivers`.

## Source policy

Large upstream trees, models, firmware blobs, and generated images are not
vendored. Fetch them into third_party and verify the exact revisions recorded
in manifests/sources.lock.yaml. The 5.10 R7 patch series and the later uncommitted
direct-doorbell experiment are deliberately stored separately: the latter is
not part of the frozen R7 baseline.

## Promotion gates

The 6.12 core path has passed RAM-only boot, eMMC-root access, MT7922 driver
enumeration, RKNPU/RKLLM inference, CPU3 start/stop/rearm, a second MailMsg
session, and a minimal four-priority regression. Ethernet, Wi-Fi association,
full peripheral coverage, long-duration stress, and persistent eMMC
installation remain separate promotion gates.

## Board runtime launcher

Install `scripts/lzamp-runtime` on the board and run it explicitly as root. It
does not register a systemd service or enable boot-time startup.

```sh
lzamp-runtime start
lzamp-runtime status
lzamp-runtime smoke
lzamp-runtime stop
lzamp-runtime stop --with-zephyr
```

Plain `stop` leaves Zephyr running. The `--with-zephyr` form requests the
MailMsg controlled-stop path after stopping RKLLM. Paths can be overridden by
the environment variables declared at the top of the script.

Files retain their own SPDX identifiers where present. A full provenance and
license review is required before publishing the repository.
