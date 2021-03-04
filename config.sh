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
export tc_url="https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz\?revision\=fed31ee5-2ed7-40c8-9e0e-474299a3c4ac\&la\=en\&hash\=76DAF56606E7CB66CC5B5B33D8FB90D9F24C9D20"
export tc_filename="gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf"
export tc_name="arm-none-linux-gnueabihf"
export tc_v="9.2"
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
