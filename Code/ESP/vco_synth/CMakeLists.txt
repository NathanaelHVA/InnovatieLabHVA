# ESP-IDF project: VCO-synth op een ESP32-S3
#   2 VCO's -> 2x DAC80501, ST7735-scherm, 12 toetsen, octaaf-, golfvorm- en reserveknop, volumepotmeter
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(vco_synth)
