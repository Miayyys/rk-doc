cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(zephyr_amp_shmem_ping)
target_sources(app PRIVATE
  src/main.c
  src/early-checkpoint.S
  ../mailmsg/mailmsg.c
  ../mailmsg/mailmsg_endpoint.c
)

# Empty by default.  A RAM-only queue-full test may set one priority (0..3)
# to keep its incoming ring unconsumed; this is not a production policy.
set(MAILMSG_TEST_SKIP_PRIORITY "" CACHE STRING "Test-only MailMsg priority to leave unconsumed")
if(NOT MAILMSG_TEST_SKIP_PRIORITY STREQUAL "")
  target_compile_definitions(app PRIVATE
    MAILMSG_TEST_SKIP_PRIORITY=${MAILMSG_TEST_SKIP_PRIORITY})
endif()

# Test-only CPU3 self-off trigger.  Empty in normal builds; the special
# self-off candidate supplies a reserved 32-bit PING value.
set(MAILMSG_TEST_SELF_OFF_VALUE "" CACHE STRING "Test-only PING value that makes CPU3 self-off")
if(NOT MAILMSG_TEST_SELF_OFF_VALUE STREQUAL "")
  target_compile_definitions(app PRIVATE
    MAILMSG_TEST_SELF_OFF_VALUE=${MAILMSG_TEST_SELF_OFF_VALUE})
endif()

# A distinct RAM-only image enables the Linux-initiated STOP_REQUEST control
# path.  Ordinary PING images deliberately reply STOP_REFUSED instead.
option(MAILMSG_ENABLE_STOP_CONTROL "Enable CPU3 STOP_REQUEST handling" OFF)
if(MAILMSG_ENABLE_STOP_CONTROL)
  target_compile_definitions(app PRIVATE MAILMSG_ENABLE_STOP_CONTROL=1)
endif()

# Test-only startup timeout candidate.  CPU3 binds the shared queues and keeps
# running, but intentionally withholds SESSION_READY so Linux must retain the
# STARTING gate and report a bounded handshake timeout.
option(MAILMSG_TEST_SKIP_SESSION_READY "Withhold the MailMsg SESSION_READY frame" OFF)
if(MAILMSG_TEST_SKIP_SESSION_READY)
  target_compile_definitions(app PRIVATE MAILMSG_TEST_SKIP_SESSION_READY=1)
endif()
