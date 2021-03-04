#!/bin/bash
sudo apt update && sudo apt install ccache python3 python3-pip
python3 -m pip install configparser
alias python="python3"
# Export
export device="cereus"
source config.sh

function sync(){
	SYNC_START=$(date +"%s")
	curl -v -F "chat_id=$TELEGRAM_CHAT" -F "parse_mode=html" -F text="Sync Started" https://api.telegram.org/bot$TELEGRAM_TOKEN/sendMessage
	cd "$KERNEL_DIR" && wget --quiet -O "$tc_filename".tar.xz "$tc_url" > /dev/null
        cd "$KERNEL_DIR" && tar -xJf "$tc_filename".tar.xz && mv "$tc_filename" "$tc_name"-"$tc_v" && rm "$tc_filename".tar.xz
	cd "$KERNEL_DIR" && git clone https://github.com/wulan17/AnyKernel3.git -b "$device" anykernel
	chmod -R a+x "$KERNEL_DIR"/"$tc_name"-"$tc_v"
	SYNC_END=$(date +"%s")
	SYNC_DIFF=$((SYNC_END - SYNC_START))
	curl -v -F "chat_id=$TELEGRAM_CHAT" -F "parse_mode=html" -F text="Sync completed successfully in $((SYNC_DIFF / 60)) minute(s) and $((SYNC_DIFF % 60)) seconds" https://api.telegram.org/bot$TELEGRAM_TOKEN/sendMessage > /dev/null
}
function build(){
	BUILD_START=$(date +"%s")
	cd "$KERNEL_DIR"
	export last_tag=$(git log -1 --oneline)
	curl -v -F "chat_id=$TELEGRAM_CHAT" -F "parse_mode=html" -F text="Build Started" https://api.telegram.org/bot$TELEGRAM_TOKEN/sendMessage > /dev/null
	make  O=out "$device"_defconfig "$THREAD" > "$KERNEL_DIR"/kernel.log
	make "$THREAD" O=out >> "$KERNEL_DIR"/kernel.log
	BUILD_END=$(date +"%s")
	BUILD_DIFF=$((BUILD_END - BUILD_START))
	export BUILD_DIFF
}
function release(){
	wget https://github.com/wulan17/builds/raw/master/github-release
	chmod +x github-release
	export full_name="$ZIP_DIR"/"$zip_name".zip
	export tag="$(env TZ='$timezone' date +%Y%m%d)"
	./github-release "$release_repo" "$tag" "master" "Kernel for Xiaomi MT6765 based devices
	Date: $(env TZ=""$timezone"" date)" "$full_name"
}
function success(){
	release
	curl -v -F "chat_id=$TELEGRAM_CHAT" -F "parse_mode=html" -F "text=Build completed successfully in $((BUILD_DIFF / 60)):$((BUILD_DIFF % 60))
Dev : ""$KBUILD_BUILD_USER""
Product : Kernel
Device : #""$device""
Branch : ""$branch""
Host : ""$KBUILD_BUILD_HOST""
Commit : ""$last_tag""
Compiler : ""$(${CROSS_COMPILE}gcc --version | head -n 1)""
Date : ""$(env TZ=Asia/Jakarta date)""
Download: <a href='https://github.com/""$release_repo""/releases/download/""$tag""/""$zip_name"".zip'>""$zip_name"".zip</a>" https://api.telegram.org/bot$TELEGRAM_TOKEN/sendMessage
	
	curl -v -F "chat_id=$TELEGRAM_CHAT" -F document=@"$KERNEL_DIR"/kernel.log https://api.telegram.org/bot$TELEGRAM_TOKEN/sendDocument > /dev/null
	exit 0
}
function failed(){
	curl -v -F "chat_id=$TELEGRAM_CHAT" -F document=@"$KERNEL_DIR"/kernel.log -F "parse_mode=html" -F "caption=Build failed in $((BUILD_DIFF / 60)):$((BUILD_DIFF % 60))" https://api.telegram.org/bot$TELEGRAM_TOKEN/sendDocument > /dev/null
	exit 1
}
function check_build(){
	if [ -e "$KERN_IMG" ]; then
		cp "$KERN_IMG" "$ZIP_DIR"
		cd "$ZIP_DIR"
		zip -r "$zip_name".zip ./*
		success
	else
		failed
	fi
}
function main(){
	sync
	build
	check_build
}

main
