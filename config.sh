#!/bin/bash
export TELEGRAM_TOKEN
export TELEGRAM_CHAT
export GITHUB_TOKEN="$GITOKEN"
export ARCH="arm"
export SUBARCH="arm"
export PATH="/usr/lib/ccache:$PATH"
export KBUILD_BUILD_USER="wulan17"
export KBUILD_BUILD_HOST="Github"
#export tc_repo="https://github.com/wulan17/linaro_arm-linux-gnueabihf-7.5.git"
export tc_url="https://developer.arm.com/-/media/Files/downloads/gnu-a/8.3-2019.03/binrel/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf.tar.xz?revision=e09a1c45-0ed3-4a8e-b06b-db3978fd8d56&la=en&hash=93ED4444B8B3A812B893373B490B90BBB28FD2E3"
export tc_filename="gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf"
export tc_name="arm-linux-gnueabihf"
export tc_v="8.3"
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
