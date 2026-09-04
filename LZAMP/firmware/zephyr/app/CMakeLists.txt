cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(lzamp_zephyr)

set(LZAMP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_include_directories(app PRIVATE ${LZAMP_ROOT}/protocol/mailmsg)
target_sources(app PRIVATE
  src/main.c
  ${LZAMP_ROOT}/firmware/zephyr/diagnostics/early-checkpoint.S
  ${LZAMP_ROOT}/protocol/mailmsg/mailmsg.c
  ${LZAMP_ROOT}/protocol/mailmsg/mailmsg_endpoint.c
)

set(MAILMSG_TEST_SKIP_PRIORITY "" CACHE STRING "Test-only priority left unconsumed")
if(NOT MAILMSG_TEST_SKIP_PRIORITY STREQUAL "")
  target_compile_definitions(app PRIVATE MAILMSG_TEST_SKIP_PRIORITY=${MAILMSG_TEST_SKIP_PRIORITY})
endif()

set(MAILMSG_TEST_SELF_OFF_VALUE "" CACHE STRING "Test-only PING value that makes CPU3 self-off")
if(NOT MAILMSG_TEST_SELF_OFF_VALUE STREQUAL "")
  target_compile_definitions(app PRIVATE MAILMSG_TEST_SELF_OFF_VALUE=${MAILMSG_TEST_SELF_OFF_VALUE})
endif()

option(MAILMSG_ENABLE_STOP_CONTROL "Enable CPU3 STOP_REQUEST handling" OFF)
if(MAILMSG_ENABLE_STOP_CONTROL)
  target_compile_definitions(app PRIVATE MAILMSG_ENABLE_STOP_CONTROL=1)
endif()

option(MAILMSG_TEST_SKIP_SESSION_READY "Withhold SESSION_READY for timeout testing" OFF)
if(MAILMSG_TEST_SKIP_SESSION_READY)
  target_compile_definitions(app PRIVATE MAILMSG_TEST_SKIP_SESSION_READY=1)
endif()
