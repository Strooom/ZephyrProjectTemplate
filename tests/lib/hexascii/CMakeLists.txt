cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app_lib_custom_test)

target_sources(app PRIVATE test.cpp)

# Include hexascii library sources
target_sources(app PRIVATE ../../../lib/hexascii/hexascii.cpp)
zephyr_include_directories(../../../lib/hexascii)

