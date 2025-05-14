#!/bin/bash

version=0.1.2
build=0
update=0

func_help() {
	printf "HELP: zbuild, version %s" "$version"
	printf "\n\nUsage:\n\tbash %s -r {board reference} -j {job} [options]\n" "$0"
	printf "Where:\n"
	printf "  {board reference}: check the \"boards\" directory, "
	printf "supported ite board ref: \n\t"
	find boards/ite/ -mindepth 1 -maxdepth 1 -type d -printf "\"%f\", "
	printf "\n"
	printf "  {job}: \"tests\" or \"program\"\n"
	printf "Options:\n"
	printf "\t--func   | -f Function board, %s\n" \
		"this is required if \"job\" option is \"tests\", default: null"
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
	elif [ "$1" = "--job" ] || [ "$1" = "-j" ]; then
		job=$2
		if [ "${job}" == "" ]; then
			printf "ERROR: job is null\n"
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

if [ "${job}" != "tests" ] && [ "${job}" != "program" ]; then
	printf "ERROR: job(%s) is unknown\n\n" "${job}"
	func_help
	exit 1
fi

if [ "${job}" = "tests" ]; then
	fname=boards/"${board_ref}"/"${board_func}"
	build_dir="build/${board_ref}_${board_func}"
else
	fname="program/${board_ref}/boards/prj"
	build_dir="build/${board_ref}"
fi

arg_overlay="-DDTC_OVERLAY_FILE=${fname}.overlay"
arg_prj_cfg="-DCONF_FILE=${fname}.conf"

if [ "${job}" == "tests" ]; then
	if [ ! -e tests/"${fname}".overlay ]; then
		printf "Error: overlay file(%s.overlay) is missing\n" "${fname}"
		func_help
		exit 1
	fi

	if [ ! -e tests/"${fname}".conf ]; then
		printf "Error: Project config(%s.conf) is missing\n""${fname}"
		func_help
		exit 1
	fi
else
	if [ ! -e "zephyr/${fname}".overlay ]; then
		printf "Error: overlay file(%s.overlay) is missing\n" "${fname}"
		func_help
		exit 1
	fi

	if [ ! -e "zephyr/${fname}".conf ]; then
		printf "Error: Project config(%s.conf) is missing\n""${fname}"
		func_help
		exit 1
	fi
fi

if [ ! -d "build" ]; then
	mkdir build
fi

printf "\n\nINFO: Apply zephyr patches...\n"
bash zpatch/apply-patches.sh "$(pwd)/zpatch" "$(pwd)/../zephyr"

if [ "${build}" == "1" ]; then
	if [ "${job}" == "tests" ]; then
		source_dir=tests
	else
		source_dir=zephyr
	fi
	printf "\n\nINFO: Build image...\n"
	printf "INFO: command: %s\n\t\t%s\n\t\t%s\n\n" \
		"west build --force -p always -b "${board_ref}" ${source_dir} -d "${build_dir}" \\" \
		""${arg_overlay}" \\" "${arg_prj_cfg}"

	if ! west build --force -p always -b "${board_ref}" ${source_dir} -d "${build_dir}" "${arg_overlay}" "${arg_prj_cfg}"; then
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
