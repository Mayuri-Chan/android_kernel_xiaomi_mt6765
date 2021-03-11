#!/bin/bash
export ci_secret
export ci_url="https://ci.wulan17.my.id"
export GITHUB_TOKEN="$GITOKEN"
export ARCH="arm"
export SUBARCH="arm"
export PATH="/usr/lib/ccache:$PATH"
export KBUILD_BUILD_USER="wulan17"
export KBUILD_BUILD_HOST="Github"
export tc_repo="https://gitlab.com/wulan17/prebuilts_gcc_linux-x86_arm_arm-none-linux-gnueabihf.git"
export tc_name="arm-none-linux-gnueabihf"
export tc_v="9.2"
export tc_branch="9.2"
export branch="$(git branch)"
export timezone='Asia/Jakarta'
export zip_name="kernel-""$device""-""$(env TZ='$timezone' date +%Y%m%d)"""
export KERNEL_DIR=$(pwd)
export KERN_IMG="$KERNEL_DIR"/out/arch/"$ARCH"/boot/zImage-dtb
export ZIP_DIR="$KERNEL_DIR"/anykernel
export CONFIG_DIR="$KERNEL_DIR"/arch/"$ARCH"/configs
export CORES=$(grep -c ^processor /proc/cpuinfo)
export THREAD="-j$CORES"
CROSS_COMPILE="ccache "
CROSS_COMPILE+="$KERNEL_DIR"/"$tc_name"-"$tc_v"/bin/"$tc_name-"
export CROSS_COMPILE
export release_repo="wulan17/kernel_build"
