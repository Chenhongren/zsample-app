#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_sources(app PRIVATE main.c)

target_sources_ifdef(CONFIG_SPI_TEST app PRIVATE spi_test/spi_test.c)
zephyr_include_directories_ifdef(CONFIG_SPI_TEST spi_test)
