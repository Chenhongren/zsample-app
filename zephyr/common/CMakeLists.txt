# Copyright (c) 2025 ITE Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

FILE(GLOB COMMON_SRC
	main.c
	system_event_log/*.c
	system_info/*.c)

message(STATUS "INFO: common path: ${COMMON_PATH}")

target_sources(app PRIVATE ${COMMON_SRC})

zephyr_include_directories(system_event_log)
zephyr_include_directories(system_info)
