#!/bin/bash

version=0.1.1
build=0
update=0

func_help() {
	printf "HELP: zbuild, version %s" "$version"
	printf "\n\nUsage:\n\tbash %s -r {board reference} -f {board function} [options]\n" "$0"
	printf "Supported ite board ref:\n"
	find boards/ite/ -mindepth 1 -maxdepth 1 -type d -printf "  - %f\n"
	printf "Options:\n"
	printf "\t--build  | -b Build the firmware, default: disable\n"
	printf "\t--update | -u Update the firmware after compiling, default: disable\n"
	printf "\t--help   | -h Help\n"
	exit 1
}

while :
do
	if [ "$1" = "--ref" ] || [ "$1" = "-r" ]; then
		board_ref=$2
		if [ "${board_ref}" == "" ]; then
			printf "ERROR: board ref is null\n"
			func_help
			exit 1
		fi
		shift 2
	elif [ "$1" = "--func" ] || [ "$1" = "-f" ]; then
		board_func=$2
		if [ "${board_func}" == "" ]; then
			printf "ERROR: board func is null\n"
			func_help
			exit 1
		fi
		shift 2
	elif [ "$1" = "--build" ] || [ "$1" = "-b" ]; then
		build=1
		shift 1
	elif [ "$1" = "--update" ] || [ "$1" = "-u" ]; then
		update=1
		shift 1
	elif [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
		func_help
		shift 1
	else
		break
	fi
done

fname=boards/"${board_ref}"/"${board_func}"
arg_overlay="-DDTC_OVERLAY_FILE=${fname}.overlay"
arg_prj_cfg="-DCONF_FILE=${fname}.conf"
build_dir="build/${board_ref}_${board_func}"

if [ ! -e app/"${fname}".overlay ]; then
	printf "Error: overlay file(%s.overlay) is missing\n" "${fname}"
	func_help
	exit 1
fi

if [ ! -e app/"${fname}".conf ]; then
	printf "Error: Project config(%s.conf) is missing\n""${fname}"
	func_help
	exit 1
fi

if [ ! -d "build" ]; then
	mkdir build
fi

printf "\n\nINFO: Apply zephyr patches...\n"
bash zpatch/apply-patches.sh "$(pwd)/zpatch" "$(pwd)/../zephyr"

if [ "${build}" == "1" ]; then
	printf "\n\nINFO: Build image...\n"
	if ! west build -p always -b "${board_ref}" -f app -d "${build_dir}" "${arg_overlay}" "${arg_prj_cfg}"; then
		exit
	fi
else
	printf "\n\nINFO: Skip firmware build(%s) update...\n" "${board_ref}_${board_func}"
fi

binary_fw="${build_dir}"/zephyr/zephyr.bin

if [ "${update}" == "1" ]; then
	if [ ! -e "${binary_fw}" ]; then
		printf "\n\nERROR: Cannot find firmware image(%s)\n" "${binary_fw}"
		exit
	fi

	printf "\n\nINFO: Update firmware...\n"
	sudo ite -f "${binary_fw}"
else
	printf "\n\nINFO: Skip firmware(%s) update...\n" "${binary_fw}"
fi
