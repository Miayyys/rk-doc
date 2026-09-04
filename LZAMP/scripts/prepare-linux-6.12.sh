#!/usr/bin/env bash
# SPDX-License-Identifier: MIT

set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
lzamp_root=$(cd -- "${script_dir}/.." && pwd)
kernel_src=${1:-"${lzamp_root}/third_party/linux-rockchip"}
expected_commit=470f9dccbdc42e7b8a824d0a5c5640a10e9457d2
patch_dir="${lzamp_root}/linux/patches/rockchip-6.12"

if [[ ! -d "${kernel_src}/.git" && ! -f "${kernel_src}/.git" ]]; then
	printf 'error: not a Git worktree: %s\n' "${kernel_src}" >&2
	exit 2
fi

actual_commit=$(git -C "${kernel_src}" rev-parse HEAD)
if [[ "${actual_commit}" != "${expected_commit}" ]]; then
	printf 'error: expected Rockchip commit %s, got %s\n' \
		"${expected_commit}" "${actual_commit}" >&2
	exit 2
fi

if [[ -n "$(git -C "${kernel_src}" status --porcelain)" ]]; then
	printf 'error: kernel worktree is not clean: %s\n' "${kernel_src}" >&2
	exit 2
fi

for patch in \
	"${patch_dir}/0001-arm64-dts-add-lzamp-rk3588s-bringup-board.patch" \
	"${patch_dir}/0002-soc-rockchip-integrate-lzamp-mailmsg.patch"
do
	git -C "${kernel_src}" apply --check "${patch}"
	git -C "${kernel_src}" apply "${patch}"
done

cmp \
	"${lzamp_root}/linux/dts/rk3588s-lzamp-linux.dts" \
	"${kernel_src}/arch/arm64/boot/dts/rockchip/rk3588s-lzamp-linux.dts"

printf 'prepared Rockchip Linux 6.12 tree at %s\n' "${kernel_src}"
printf 'next: make -C LZAMP linux-6.12-configure linux-6.12-build\n'
