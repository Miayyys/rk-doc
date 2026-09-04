CC ?= cc
CROSS_CC ?= aarch64-linux-gnu-gcc
CFLAGS ?= -O2 -g
CPPFLAGS += -Iprotocol/mailmsg
CFLAGS += -std=c11 -Wall -Wextra -Werror

TEST_NAMES := mailmsg_protocol_test mailmsg_endpoint_test mailmsg_notify_test mailmsg_mailbox0_test
TEST_BINS := $(addprefix build/host-tests/,$(TEST_NAMES))
BOARD_TOOL_NAMES := mailmsg-user-client mailmsg-window-bench mailmsg-exclusive-reader-test mailmsg-offline-wait-test
BOARD_TOOL_BINS := $(addprefix build/board-tools/,$(BOARD_TOOL_NAMES))
LINUX_6_12_SRC ?= $(CURDIR)/third_party/linux-rockchip
LINUX_6_12_OUT ?= $(CURDIR)/build/linux-rockchip-6.12
LINUX_JOBS ?= $(shell nproc)

.PHONY: all test agent-test board-tools linux-6.12-prepare linux-6.12-configure linux-6.12-build clean

all: test

test: $(TEST_BINS) agent-test
	@set -e; for test_bin in $(TEST_BINS); do ./$$test_bin; done

agent-test:
	python3 -m unittest discover -s agent/tests -p 'test_*.py'

board-tools: $(BOARD_TOOL_BINS)

linux-6.12-prepare:
	bash scripts/prepare-linux-6.12.sh "$(LINUX_6_12_SRC)"

linux-6.12-configure:
	$(MAKE) -C "$(LINUX_6_12_SRC)" O="$(LINUX_6_12_OUT)" \
		ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- rockchip_linux_defconfig
	"$(LINUX_6_12_SRC)/scripts/kconfig/merge_config.sh" -m -O "$(LINUX_6_12_OUT)" \
		"$(LINUX_6_12_OUT)/.config" linux/config/rockchip-6.12-lzamp.fragment
	$(MAKE) -C "$(LINUX_6_12_SRC)" O="$(LINUX_6_12_OUT)" \
		ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig

linux-6.12-build:
	$(MAKE) -C "$(LINUX_6_12_SRC)" O="$(LINUX_6_12_OUT)" \
		ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j"$(LINUX_JOBS)" \
		Image rockchip/rk3588s-lzamp-linux.dtb modules

build/host-tests:
	mkdir -p $@

build/board-tools:
	mkdir -p $@

build/host-tests/mailmsg_protocol_test: tests/host/mailmsg_protocol_test.c protocol/mailmsg/mailmsg.c | build/host-tests
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

build/host-tests/mailmsg_endpoint_test: tests/host/mailmsg_endpoint_test.c protocol/mailmsg/mailmsg.c protocol/mailmsg/mailmsg_endpoint.c | build/host-tests
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

build/host-tests/mailmsg_notify_test: tests/host/mailmsg_notify_test.c | build/host-tests
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

build/host-tests/mailmsg_mailbox0_test: tests/host/mailmsg_mailbox0_test.c protocol/mailmsg/mailmsg_mailbox0.c | build/host-tests
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

build/board-tools/mailmsg-user-client: tools/mailmsg_user_client.c | build/board-tools
	$(CROSS_CC) $(CPPFLAGS) -O2 -g -std=gnu11 -Wall -Wextra -Werror -static $< -o $@

build/board-tools/mailmsg-window-bench: tools/mailmsg_window_bench.c | build/board-tools
	$(CROSS_CC) $(CPPFLAGS) -O2 -g -std=gnu11 -Wall -Wextra -Werror -static $< -o $@

build/board-tools/mailmsg-exclusive-reader-test: tests/board/mailmsg_exclusive_reader_test.c | build/board-tools
	$(CROSS_CC) $(CPPFLAGS) -O2 -g -std=gnu11 -Wall -Wextra -Werror -static $< -o $@

build/board-tools/mailmsg-offline-wait-test: tests/board/mailmsg_offline_wait_test.c | build/board-tools
	$(CROSS_CC) $(CPPFLAGS) -O2 -g -std=gnu11 -Wall -Wextra -Werror -static $< -o $@

clean:
	rm -rf build/host-tests build/board-tools
