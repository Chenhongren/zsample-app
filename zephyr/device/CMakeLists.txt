# Copyright (c) 2025 ITE Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

FILE(GLOB DEVICE_SRC
	flash/*.c)

target_sources(app PRIVATE ${DEVICE_SRC})

zephyr_include_directories(flash)
