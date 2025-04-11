# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SmartlessRelay32)

if(CONFIG_IDF_TARGET_ESP32C2)
    include(relinker)
endif()

idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
# For RISCV chips, project_include.cmake sets -Wno-format, but does not clear various
# flags that depend on -Wformat
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security" APPEND)

#if(CONFIG_IDF_TARGET_ESP32C3)
    # Adds necessary definitions for compiling it using Serial symbol attached to the HW USB CDC port
#    list(APPEND compile_definitions "ARDUINO_USB_CDC_ON_BOOT=1")
#    list(APPEND compile_definitions "ARDUINO_USB_MODE=1")
#endif()
