#!/bin/bash
sudo apt update && sudo apt install ccache python3 python3-pip
python3 -m pip install configparser
# Export
export device="cereus"
source config.sh

function sync(){
	SYNC_START=$(date +"%s")
	curl -F secret="$ci_secret" -F text="Sync Started" "$ci_url"/sendMessage
	cd "$KERNEL_DIR" && git clone "$tc_repo" -b "$tc_branch" "$tc_name"-"$tc_v"
	cd "$KERNEL_DIR" && git clone https://github.com/wulan17/AnyKernel3.git -b "$device" anykernel
	SYNC_END=$(date +"%s")
	SYNC_DIFF=$((SYNC_END - SYNC_START))
	curl -F secret="$ci_secret" -F text="Sync completed successfully in $((SYNC_DIFF / 60)) minute(s) and $((SYNC_DIFF % 60)) seconds" "$ci_url"/sendMessage > /dev/null
}
function build(){
	BUILD_START=$(date +"%s")
	cd "$KERNEL_DIR"
	export last_tag=$(git log -1 --oneline)
	curl -F secret="$ci_secret" -F text="Build Started for $device" "$ci_url"/sendMessage > /dev/null
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
	curl -v -F secret="$ci_secret" -F "text=Build completed successfully in $((BUILD_DIFF / 60)):$((BUILD_DIFF % 60))
Download: <a href='https://github.com/""$release_repo""/releases/download/""$tag""/""$zip_name"".zip'>""$zip_name"".zip</a>" "$ci_url"/sendMessage

	curl -v -F secret="$ci_secret" -F document=@"$KERNEL_DIR"/kernel.log -F "$ci_url"/sendDocument > /dev/null
	exit 0
}
function failed(){
	curl -v -F secret="$ci_secret" -F document=@"$KERNEL_DIR"/kernel.log -F "caption=Build failed in $((BUILD_DIFF / 60)):$((BUILD_DIFF % 60))" "$ci_url"/sendDocument > /dev/null
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
