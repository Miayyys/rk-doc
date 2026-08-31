#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# One entry point for MailMsg V1 revision 6 artifact upload and board tests.
# Host mode validates and uploads the RAM-only artifacts over SSH.  Remote
# mode runs exactly one profile because timeout profiles require a fresh
# U-Boot RAM boot before another profile can be attempted.

set -Eeuo pipefail
IFS=$'\n\t'

readonly SCRIPT_NAME=${0##*/}
readonly DEFAULT_R1_HOST=10.42.0.193
readonly DEFAULT_R1_USER=root
readonly DEFAULT_REMOTE_ROOT=/userdata/mailmsg-v1-r6

MODE=host
PROFILE=
UPLOAD_ONLY=0
CHECK_ONLY=0
R1_HOST=${R1_HOST:-$DEFAULT_R1_HOST}
R1_USER=${R1_USER:-$DEFAULT_R1_USER}
R1_IDENTITY=${R1_IDENTITY:-${HOME}/.ssh/id_ed25519}
REMOTE_ROOT=${MAILMSG_REMOTE_ROOT:-$DEFAULT_REMOTE_ROOT}

usage()
{
	cat <<EOF
Usage:
  $SCRIPT_NAME [--host ADDRESS] [--user USER] [--identity KEY] [--upload-only]
  $SCRIPT_NAME [host options] --profile PROFILE
  $SCRIPT_NAME --check-only
  $SCRIPT_NAME --remote --remote-root DIR --profile PROFILE

Profiles (one fresh RAM boot per terminal profile):
  controlled     Full normal/data/error/queue/STOP_READY/rearm regression.
  stop-refused   Normal image must refuse STOP and remain usable.
  stop-timeout   STOP request is deliberately not consumed; ends terminal.
  start-timeout  SESSION_READY is deliberately withheld; ends terminal.

Host environment overrides: R1_HOST, R1_USER, R1_IDENTITY,
MAILMSG_REMOTE_ROOT.  Uploads only to /userdata; never writes a boot partition.
Before --profile, manually RAM-boot the uploaded FIT from U-Boot; this script
does not automate the U-Boot console or change the persistent boot image.
EOF
}

fail()
{
	printf 'FAIL: %s\n' "$*" >&2
	return 1
}

note()
{
	printf '\n[%s] %s\n' "$(date '+%H:%M:%S')" "$*"
}

pass()
{
	printf 'PASS: %s\n' "$*"
}

while (($#)); do
	case $1 in
	--remote)
		MODE=remote
		shift
		;;
	--profile)
		(($# >= 2)) || { usage >&2; exit 2; }
		PROFILE=$2
		shift 2
		;;
	--host)
		(($# >= 2)) || { usage >&2; exit 2; }
		R1_HOST=$2
		shift 2
		;;
	--user)
		(($# >= 2)) || { usage >&2; exit 2; }
		R1_USER=$2
		shift 2
		;;
	--identity)
		(($# >= 2)) || { usage >&2; exit 2; }
		R1_IDENTITY=$2
		shift 2
		;;
	--remote-root)
		(($# >= 2)) || { usage >&2; exit 2; }
		REMOTE_ROOT=$2
		shift 2
		;;
	--upload-only)
		UPLOAD_ONLY=1
		shift
		;;
	--check-only)
		CHECK_ONLY=1
		shift
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		printf 'Unknown argument: %s\n' "$1" >&2
		usage >&2
		exit 2
		;;
	esac
done

case ${PROFILE:-none} in
	none|controlled|stop-refused|stop-timeout|start-timeout) ;;
	*) fail "unknown profile: $PROFILE"; exit 2 ;;
esac

if [[ ! $REMOTE_ROOT =~ ^/userdata(/[A-Za-z0-9._-]+)+$ ||
	$REMOTE_ROOT == */./* || $REMOTE_ROOT == */../* ||
	$REMOTE_ROOT == */. || $REMOTE_ROOT == */.. ]]; then
	fail "remote root must be an absolute, simple /userdata path: $REMOTE_ROOT"
	exit 2
fi

if ((CHECK_ONLY)) && { ((UPLOAD_ONLY)) || [[ -n $PROFILE ]] || [[ $MODE == remote ]]; }; then
	fail "--check-only cannot be combined with upload, profile, or remote mode"
	exit 2
fi
if ((UPLOAD_ONLY)) && [[ -n $PROFILE ]]; then
	fail "--upload-only and --profile are mutually exclusive"
	exit 2
fi

expected_sha256()
{
	case $1 in
	mailmsg-v1-final.img)
		printf '%s\n' eade522dbd360d2e42d4d42e39f7ad7340e31adab4b3229e246ea6991b7d2b50 ;;
	mailmsg-v1-normal.bin)
		printf '%s\n' 1b2636f3dfdc9529ebee9059c7f5b3eb5716bbb526db0829ad62a50d77222b83 ;;
	mailmsg-v1-controlled-stop.bin)
		printf '%s\n' 3876efefc0f5dd221d33fd089ca80b91dbce2187aff9bf4faefa164537b8aae6 ;;
	mailmsg-v1-start-timeout.bin)
		printf '%s\n' dcc335bbfb65c0fd5fea1dba09ea890bbcdf45933ead6804e02106535b7490b4 ;;
	mailmsg-v1-stop-timeout.bin)
		printf '%s\n' 6f3c9c741da63860683a6cf9790dbcec3c224dd5ec4b51f490b66399b15b312e ;;
	mailmsg-user-client-aarch64)
		printf '%s\n' c85c829789c15445f665e9ccdbf02ccc9ce30cf0fdc0b499514e50d2c1d0ae55 ;;
	mailmsg-exclusive-reader-test-aarch64)
		printf '%s\n' 36e9582afc62aa0d8b73dfd6118c2fba37812ac40b4ad03150f5eed43a30dbfa ;;
	mailmsg-offline-wait-test-aarch64)
		printf '%s\n' 695462dc3986af841afb35c32da21624c1ff19e7c48f7fc48e86a1c4597dfaca ;;
	*) return 1 ;;
	esac
}

readonly ARTIFACT_NAMES=(
	mailmsg-v1-final.img
	mailmsg-v1-normal.bin
	mailmsg-v1-controlled-stop.bin
	mailmsg-v1-start-timeout.bin
	mailmsg-v1-stop-timeout.bin
	mailmsg-user-client-aarch64
	mailmsg-exclusive-reader-test-aarch64
	mailmsg-offline-wait-test-aarch64
)

verify_file_hash()
{
	local path=$1
	local name=${path##*/}
	local expected actual

	expected=$(expected_sha256 "$name") || return 1
	actual=$(sha256sum "$path")
	actual=${actual%% *}
	[[ $actual == "$expected" ]] || {
		printf 'hash mismatch: %s\n  expected %s\n  actual   %s\n' \
			"$path" "$expected" "$actual" >&2
		return 1
	}
}

host_main()
{
	local script_dir repo_root artifact_dir target remote_cmd
	local name local_path remote_hash expected local_hash
	local -a files ssh_opts

	script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
	repo_root=$(cd -- "$script_dir/.." && pwd)
	artifact_dir=$repo_root/build/local/mailmsg-v1-final

	note "checking local MailMsg V1 revision 6 artifacts"
	for name in "${ARTIFACT_NAMES[@]}"; do
		local_path=$artifact_dir/$name
		[[ -f $local_path ]] || { fail "missing artifact: $local_path"; return 1; }
		verify_file_hash "$local_path" || return 1
		files+=("$local_path")
	done
	[[ -f $artifact_dir/artifact-manifest.txt ]] || {
		fail "missing artifact manifest"
		return 1
	}
	files+=("$artifact_dir/artifact-manifest.txt" "$script_dir/$SCRIPT_NAME")
	pass "all local artifact hashes match the revision 6 manifest"

	if ((CHECK_ONLY)); then
		bash -n "$script_dir/$SCRIPT_NAME"
		pass "script syntax and local artifacts"
		return 0
	fi

	command -v ssh >/dev/null || { fail "ssh is not installed"; return 1; }
	command -v scp >/dev/null || { fail "scp is not installed"; return 1; }
	[[ -r $R1_IDENTITY ]] || { fail "SSH identity is not readable: $R1_IDENTITY"; return 1; }

	ssh_opts=(
		-F /dev/null
		-i "$R1_IDENTITY"
		-o IdentitiesOnly=yes
		-o PasswordAuthentication=no
		-o ConnectTimeout=6
		-o StrictHostKeyChecking=accept-new
	)
	target=$R1_USER@$R1_HOST

	note "read-only SSH identity and target preflight: $target"
	ssh "${ssh_opts[@]}" "$target" \
		'id -un; hostname; uname -r; test -d /userdata; printf "userdata-ok\\n"'

	note "uploading isolated test bundle to $target:$REMOTE_ROOT"
	printf -v remote_cmd 'umask 022; mkdir -p -- %q' "$REMOTE_ROOT"
	ssh "${ssh_opts[@]}" "$target" "$remote_cmd"
	scp "${ssh_opts[@]}" -- "${files[@]}" "$target:$REMOTE_ROOT/"

	printf -v remote_cmd 'chmod 0755 -- %q %q %q %q' \
		"$REMOTE_ROOT/$SCRIPT_NAME" \
		"$REMOTE_ROOT/mailmsg-user-client-aarch64" \
		"$REMOTE_ROOT/mailmsg-exclusive-reader-test-aarch64" \
		"$REMOTE_ROOT/mailmsg-offline-wait-test-aarch64"
	ssh "${ssh_opts[@]}" "$target" "$remote_cmd"

	note "verifying every transferred artifact on R1"
	for name in "${ARTIFACT_NAMES[@]}"; do
		expected=$(expected_sha256 "$name")
		printf -v remote_cmd 'sha256sum -- %q' "$REMOTE_ROOT/$name"
		remote_hash=$(ssh "${ssh_opts[@]}" "$target" "$remote_cmd")
		remote_hash=${remote_hash%% *}
		[[ $remote_hash == "$expected" ]] || {
			fail "R1 hash mismatch for $name"
			return 1
		}
		printf 'verified: %s\n' "$name"
	done
	for local_path in "$artifact_dir/artifact-manifest.txt" "$script_dir/$SCRIPT_NAME"; do
		name=${local_path##*/}
		local_hash=$(sha256sum "$local_path")
		local_hash=${local_hash%% *}
		printf -v remote_cmd 'sha256sum -- %q' "$REMOTE_ROOT/$name"
		remote_hash=$(ssh "${ssh_opts[@]}" "$target" "$remote_cmd")
		remote_hash=${remote_hash%% *}
		[[ $remote_hash == "$local_hash" ]] || {
			fail "R1 hash mismatch for $name"
			return 1
		}
		printf 'verified: %s\n' "$name"
	done
	pass "upload and R1-side SHA-256 verification"
	printf 'U-Boot RAM FIT path: %s/mailmsg-v1-final.img\n' "$REMOTE_ROOT"

	if ((UPLOAD_ONLY)) || [[ -z $PROFILE ]]; then
		printf 'Upload complete. No board test was started.\n'
		return 0
	fi

	note "running remote profile: $PROFILE"
	printf -v remote_cmd 'exec %q --remote --remote-root %q --profile %q' \
		"$REMOTE_ROOT/$SCRIPT_NAME" "$REMOTE_ROOT" "$PROFILE"
	ssh -tt "${ssh_opts[@]}" "$target" "$remote_cmd"
}

# Remote board-test implementation starts here.
AMP=
REPORT=
CLIENT=
EXCLUSIVE_TEST=
SESSION_GENERATION=0

remote_snapshot()
{
	set +e
	printf '\n--- diagnostic snapshot ---\n'
	[[ -n ${AMP:-} && -r $AMP/status ]] && cat "$AMP/status"
	[[ -n ${AMP:-} && -r $AMP/mailmsg_stats ]] && cat "$AMP/mailmsg_stats"
	[[ -n ${AMP:-} && -r $AMP/affinity_state ]] && cat "$AMP/affinity_state"
	printf '%s\n' '--- end snapshot ---'
	set -e
}

remote_fail()
{
	printf 'FAIL: %s\n' "$*" >&2
	remote_snapshot
	exit 1
}

discover_amp()
{
	local candidate
	local -a matches=()

	for candidate in /sys/bus/platform/devices/*; do
		[[ -f $candidate/status && -f $candidate/mailmsg_stats &&
		   -f $candidate/start && -f $candidate/image ]] || continue
		matches+=("$candidate")
	done
	((${#matches[@]} == 1)) || remote_fail \
		"expected one MailMsg AMP platform device, found ${#matches[@]}"
	AMP=${matches[0]}
	[[ -r $AMP/of_node/compatible ]] ||
		remote_fail "MailMsg platform device has no readable OF compatible"
	grep -aq 'youyeetoo,r1-amp-psci-cpu-on-heartbeat' \
		"$AMP/of_node/compatible" ||
		remote_fail "unexpected MailMsg platform-device compatible"
}

mailmsg_status()
{
	cat "$AMP/status"
}

mailmsg_state()
{
	local status
	status=$(mailmsg_status)
	status=${status#*mailmsg_state=}
	printf '%s\n' "${status%% *}"
}

wait_state()
{
	local expected=$1
	local attempts=${2:-40}
	local index state

	for ((index = 0; index < attempts; index++)); do
		state=$(mailmsg_state)
		[[ $state == "$expected" ]] && return 0
		sleep 0.2
	done
	remote_fail "expected state $expected, got $(mailmsg_state)"
}

assert_session_active()
{
	local status pair local_generation peer_generation

	wait_state active 45
	status=$(mailmsg_status)
	pair=${status#*session=}
	pair=${pair%% *}
	local_generation=${pair%/*}
	peer_generation=${pair#*/}
	[[ $local_generation =~ ^[0-9]+$ && $peer_generation =~ ^[0-9]+$ ]] ||
		remote_fail "cannot parse session generation from status"
	((local_generation > 0 && local_generation == peer_generation)) ||
		remote_fail "session mismatch: $local_generation/$peer_generation"
	((local_generation > SESSION_GENERATION)) ||
		remote_fail "session generation did not advance: $SESSION_GENERATION -> $local_generation"
	SESSION_GENERATION=$local_generation
	grep -q 'session_result=0' <<<"$status" || remote_fail "SESSION_READY result is not zero"
	grep -q 'state=on (0)' "$AMP/affinity_state" || remote_fail "CPU3 is not ON"
	pass "SESSION_READY generation $local_generation and ACTIVE state"
}

prepare_unarmed()
{
	local state
	state=$(mailmsg_state)
	if [[ $state == offline ]]; then
		grep -q 'state=off (1)' "$AMP/affinity_state" ||
			remote_fail "OFFLINE state without fresh PSCI OFF"
		printf 'rearm\n' >"$AMP/rearm"
		state=$(mailmsg_state)
	fi
	[[ $state == unarmed ]] || remote_fail \
		"profile needs fresh unarmed state; current state is $state"
	grep -q 'state=off (1)' "$AMP/affinity_state" ||
		remote_fail "CPU3 must be OFF before image load"
}

start_image()
{
	local image=$1
	local status

	prepare_unarmed
	[[ -f $image ]] || remote_fail "missing Zephyr image: $image"
	verify_file_hash "$image" || remote_fail "bad Zephyr image hash: $image"
	note "loading ${image##*/} into the reserved CPU3 image window"
	command cat "$image" >"$AMP/image"
	status=$(mailmsg_status)
	grep -q 'image=41008/41008' <<<"$status" ||
		remote_fail "kernel did not accept the complete 41008-byte image"
	printf 'start\n' >"$AMP/start"
}

assert_all_depths_zero()
{
	local stats
	stats=$(cat "$AMP/mailmsg_stats")
	awk '
		/^p[0-3] / {
			seen++
			depths = 0
			for (i = 1; i <= NF; i++)
				if ($i == "depth=0") depths++
			if (depths != 2) bad = 1
		}
		END { exit !(seen == 4 && !bad) }
	' <<<"$stats" || remote_fail "one or more MailMsg rings are not empty"
}

run_client()
{
	local priority=$1
	local value=$2
	local expected=$((value + 1))
	local output

	if ! output=$("$CLIENT" "$priority" "$value" 2>&1); then
		printf '%s\n' "$output" >&2
		remote_fail "priority $priority user client failed"
	fi
	printf '%s\n' "$output"
	grep -q "value=$expected" <<<"$output" ||
		remote_fail "priority $priority did not return PONG value $expected"
	if ((priority <= 1)); then
		grep -Eq 'type=3 .*status=0' <<<"$output" ||
			remote_fail "reliable priority $priority did not return ACK"
	fi
}

run_four_priorities()
{
	local priority
	local -a values=(100 200 300 400)

	note "four-priority user ABI regression"
	for priority in 0 1 2 3; do
		run_client "$priority" "${values[$priority]}"
	done
	assert_all_depths_zero
	pass "p0/p1 ACK+PONG and p2/p3 PONG"
}

run_exclusive_reader()
{
	note "single-reader ownership and handoff"
	"$EXCLUSIVE_TEST" || remote_fail "exclusive-reader test failed"
	pass "second reader EBUSY and handoff EAGAIN"
}

run_crc_fault()
{
	local index output

	note "reliable p0 CRC rejection"
	printf '0 600\n' >"$AMP/mailmsg_crc_inject"
	for ((index = 0; index < 20; index++)); do
		output=$(cat "$AMP/mailmsg_response")
		if grep -Eq 'priority=0 valid=1 type=4 .*status=1' <<<"$output"; then
			printf '%s\n' "$output"
			grep -q 'type=2' <<<"$output" &&
				remote_fail "bad CRC unexpectedly produced a PONG"
			assert_all_depths_zero
			pass "bad CRC produced NACK_BAD_CRC"
			return 0
		fi
		sleep 0.1
	done
	remote_fail "CRC fault did not produce a p0 NACK"
}

p3_notify_failed_count()
{
	awk '
		$1 == "p3" {
			split($6, first, "=")
			split(first[2], count, "/")
			print count[3]
		}
	' "$AMP/mailmsg_stats"
}

run_notify_failure()
{
	local before after index output collected=

	note "p3 notification failure, retained frame, and recovery doorbell"
	before=$(p3_notify_failed_count)
	printf '3 -5 1\n' >"$AMP/mailmsg_notify_inject"
	printf '3 710\n' >"$AMP/mailmsg_ping"
	after=$(p3_notify_failed_count)
	[[ $after =~ ^[0-9]+$ && $before =~ ^[0-9]+$ && $after -eq $((before + 1)) ]] ||
		remote_fail "p3 notify_failed counter did not increment"
	printf 'clear\n' >"$AMP/mailmsg_notify_inject"
	printf '3 711\n' >"$AMP/mailmsg_ping"

	for ((index = 0; index < 30; index++)); do
		output=$(cat "$AMP/mailmsg_response")
		[[ $output == 'valid=0 reason=empty' ]] || collected+=$'\n'$output
		if grep -q 'priority=3 valid=1 type=2 .*value=711' <<<"$collected" &&
		   grep -q 'priority=3 valid=1 type=2 .*value=712' <<<"$collected"; then
			printf '%s\n' "$collected"
			assert_all_depths_zero
			pass "failed doorbell was observable; next doorbell drained both frames"
			return 0
		fi
		sleep 0.1
	done
	remote_fail "notification recovery did not return both p3 PONG frames"
}

run_queue_full_and_recover()
{
	local value index output expected count total depth collected=

	note "p3 seven-slot queue boundary and explicit recovery doorbell"
	for value in 800 801 802 803 804 805 806; do
		printf '3 %s\n' "$value" >"$AMP/mailmsg_queue_push" ||
			remote_fail "p3 queue became full before seven usable slots"
	done
	depth=$(awk '$1 == "p3" { split($5, value, "="); print value[2] }' \
		"$AMP/mailmsg_stats")
	[[ $depth == 7 ]] || remote_fail "p3 depth is $depth after seven queue-only writes"
	if printf '3 807\n' >"$AMP/mailmsg_queue_push" 2>/dev/null; then
		remote_fail "p3 eighth frame was unexpectedly accepted"
	fi
	depth=$(awk '$1 == "p3" { split($5, value, "="); print value[2] }' \
		"$AMP/mailmsg_stats")
	[[ $depth == 7 ]] || remote_fail "rejected eighth frame changed p3 depth to $depth"
	pass "eighth p3 frame was rejected while the seven-slot ring remained full"

	# The queue-only hook deliberately omits notification.  One raw ch3
	# doorbell wakes CPU3; the test service then drains the seven committed
	# frames.  This is test plumbing, not part of the MailMsg wire protocol.
	printf '3 0\n' >"$AMP/doorbell"
	for ((index = 0; index < 30; index++)); do
		output=$(cat "$AMP/mailmsg_response")
		[[ $output == 'valid=0 reason=empty' ]] || collected+=$'\n'$output
		if grep -q 'priority=3 valid=1 type=2 .*value=807' <<<"$collected"; then
			total=$(grep -Ec '^priority=3 valid=1 type=2 .*value=' <<<"$collected")
			[[ $total == 7 ]] || remote_fail "expected seven p3 PONGs, collected $total"
			for expected in 801 802 803 804 805 806 807; do
				count=$(grep -Ec "^priority=3 valid=1 type=2 .*value=$expected$" \
					<<<"$collected")
				[[ $count == 1 ]] || remote_fail \
					"expected one p3 PONG value=$expected, collected $count"
			done
			printf '%s\n' "$collected"
			assert_all_depths_zero
			pass "seven queued p3 frames drained without overwrite"
			return 0
		fi
		sleep 0.1
	done
	remote_fail "p3 queue recovery did not drain all seven frames"
}

run_stop_refused()
{
	local index status

	note "STOP_REFUSED and active data-plane recovery"
	assert_all_depths_zero
	printf 'stop\n' >"$AMP/mailmsg_stop"
	for ((index = 0; index < 35; index++)); do
		status=$(mailmsg_status)
		if grep -q 'mailmsg_state=active' <<<"$status" &&
		   grep -q 'stop_reply=7' <<<"$status" &&
		   grep -q 'stop_result=-125' <<<"$status"; then
			printf '%s\n' "$status"
			run_client 0 900
			pass "STOP_REFUSED preserved ACTIVE and p0 usability"
			return 0
		fi
		sleep 0.2
	done
	remote_fail "normal image did not return STOP_REFUSED"
}

run_controlled_stop()
{
	local index status

	note "STOP_READY, CPU_OFF, and OFFLINE"
	assert_all_depths_zero
	grep -q 'a2b_now=m0:0' <<<"$(mailmsg_status)" ||
		remote_fail "mailbox0 still has an A2B pending bit before STOP"
	printf 'stop\n' >"$AMP/mailmsg_stop"
	for ((index = 0; index < 45; index++)); do
		status=$(mailmsg_status)
		if grep -q 'mailmsg_state=offline' <<<"$status" &&
		   grep -q 'stop_reply=6' <<<"$status" &&
		   grep -q 'stop_result=0' <<<"$status"; then
			grep -q 'state=off (1)' "$AMP/affinity_state" ||
				remote_fail "STOP_READY completed but CPU3 affinity is not OFF"
			grep -Eq 'a2b_now=m0:0x0( |$)' <<<"$status" ||
				remote_fail "CPU3 is OFF but an old A2B doorbell remains pending"
			printf '%s\n' "$status"
			pass "controlled STOP reached OFFLINE"
			return 0
		fi
		sleep 0.2
	done
	remote_fail "controlled STOP did not reach OFFLINE"
}

run_controlled_profile()
{
	local image=$REMOTE_ROOT/mailmsg-v1-controlled-stop.bin

	start_image "$image"
	assert_session_active
	# This must precede the first Linux A2B doorbell.  The current Zephyr
	# test service latches mailbox_irq_seen and polls thereafter, so a
	# later queue-only fill would be consumed immediately.
	run_queue_full_and_recover
	run_four_priorities
	run_exclusive_reader
	run_crc_fault
	run_notify_failure
	note "per-priority final statistics"
	cat "$AMP/mailmsg_stats"
	run_controlled_stop

	note "guarded rearm and second-session regression"
	printf 'rearm\n' >"$AMP/rearm"
	[[ $(mailmsg_state) == unarmed ]] || remote_fail "rearm did not return UNARMED"
	grep -q 'image=0/41008' <<<"$(mailmsg_status)" ||
		remote_fail "rearm did not clear image progress"
	start_image "$image"
	assert_session_active
	run_client 0 1000
	assert_all_depths_zero
	run_controlled_stop
	pass "controlled profile complete; CPU3 is OFF"
}

run_stop_refused_profile()
{
	start_image "$REMOTE_ROOT/mailmsg-v1-normal.bin"
	assert_session_active
	run_client 0 1100
	run_stop_refused
	remote_snapshot
	pass "stop-refused profile complete; CPU3 intentionally remains ON"
}

run_start_timeout_profile()
{
	local status output

	start_image "$REMOTE_ROOT/mailmsg-v1-start-timeout.bin"
	wait_state start-timeout 45
	status=$(mailmsg_status)
	grep -q 'session_result=-110' <<<"$status" ||
		remote_fail "START timeout did not report ETIMEDOUT"
	printf '%s\n' "$status"
	if output=$("$CLIENT" 0 1200 --no-read 2>&1); then
		remote_fail "data plane accepted a write after START timeout"
	fi
	printf '%s\n' "$output"
	grep -q 'Connection timed out' <<<"$output" ||
		remote_fail "post-START-timeout write failed for an unexpected reason"
	pass "START timeout is terminal and data plane remains closed"
	printf 'A fresh U-Boot RAM boot is required before another profile.\n'
}

run_stop_timeout_profile()
{
	local status output

	start_image "$REMOTE_ROOT/mailmsg-v1-stop-timeout.bin"
	assert_session_active
	run_client 1 1300
	assert_all_depths_zero
	printf 'stop\n' >"$AMP/mailmsg_stop"
	wait_state stop-timeout 45
	status=$(mailmsg_status)
	grep -q 'stop_result=-110' <<<"$status" ||
		remote_fail "STOP timeout did not report ETIMEDOUT"
	grep -q 'state=on (0)' "$AMP/affinity_state" ||
		remote_fail "STOP timeout candidate unexpectedly powered CPU3 off"
	printf '%s\n' "$status"
	if output=$("$CLIENT" 1 1301 --no-read 2>&1); then
		remote_fail "data plane accepted a write after STOP timeout"
	fi
	printf '%s\n' "$output"
	grep -q 'Connection timed out' <<<"$output" ||
		remote_fail "post-STOP-timeout write failed for an unexpected reason"
	pass "STOP timeout is terminal and CPU3 remains ON"
	printf 'A fresh U-Boot RAM boot is required before another profile.\n'
}

remote_main()
{
	local name path timestamp

	[[ $(id -u) -eq 0 ]] || { fail "remote tests must run as root"; exit 1; }
	[[ -n $PROFILE ]] || { fail "--profile is required in remote mode"; exit 2; }
	command -v sha256sum >/dev/null || { fail "sha256sum is required"; exit 1; }
	mkdir -p -- "$REMOTE_ROOT/reports"
	timestamp=$(date '+%Y%m%d-%H%M%S')
	REPORT=$REMOTE_ROOT/reports/$PROFILE-$timestamp.log
	exec > >(tee -a "$REPORT") 2>&1

	note "MailMsg V1 revision 6 board profile: $PROFILE"
	printf 'report=%s\n' "$REPORT"
	for name in "${ARTIFACT_NAMES[@]}"; do
		path=$REMOTE_ROOT/$name
		[[ -f $path ]] || remote_fail "missing uploaded artifact: $path"
		verify_file_hash "$path" || remote_fail "artifact hash mismatch: $path"
	done
	pass "R1 artifact hashes"

	discover_amp
	CLIENT=$REMOTE_ROOT/mailmsg-user-client-aarch64
	EXCLUSIVE_TEST=$REMOTE_ROOT/mailmsg-exclusive-reader-test-aarch64
	for path in status mailmsg_stats affinity_state image start rearm \
		mailmsg_ping mailmsg_stop mailmsg_crc_inject mailmsg_notify_inject \
		mailmsg_queue_push mailmsg_response doorbell; do
		[[ -e $AMP/$path ]] || remote_fail "missing driver interface: $path"
	done
	for path in /dev/mailmsg-p0 /dev/mailmsg-p1 /dev/mailmsg-p2 /dev/mailmsg-p3; do
		[[ -e $path ]] || remote_fail "missing character device: $path"
	done
	[[ $(uname -r) == 5.10.252 ]] || remote_fail "unexpected kernel: $(uname -r)"
	[[ $(nproc) == 7 ]] || remote_fail "expected seven Linux CPUs, got $(nproc)"
	[[ -d /proc/device-tree/reserved-memory/zephyr@50000000 ]] ||
		remote_fail "Zephyr reserved-memory node is absent"
	grep -q 'stale=' "$AMP/mailmsg_stats" ||
		remote_fail "running driver lacks the MailMsg V6 statistics ABI"
	grep -q 'session=' "$AMP/status" ||
		remote_fail "running driver lacks the MailMsg V6 session ABI"
	remote_snapshot
	pass "V6 kernel/DT/sysfs preflight"

	case $PROFILE in
	controlled) run_controlled_profile ;;
	stop-refused) run_stop_refused_profile ;;
	start-timeout) run_start_timeout_profile ;;
	stop-timeout) run_stop_timeout_profile ;;
	esac

	note "profile result: PASS"
	printf 'Full report saved at %s\n' "$REPORT"
}

if [[ $MODE == remote ]]; then
	remote_main
else
	host_main
fi
