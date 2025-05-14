# Copyright (c) 2025 ITE Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(DEFINED CONFIG_BOARD_MARS_5)
	add_subdirectory(mars_5)
endif()
