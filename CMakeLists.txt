# Configuration file for using CMake to build.
# This is compatible with the JetBrains CLion IDE (https://www.jetbrains.com/clion).
#
# To configure CLion to cross-compile for the Vex Cortex M3 ARM cpu:
# - Choose the "CLion -> Preferences" menu option
# - Navigate to "Build, Execution, Deployment" and select "CMake"
# - Create a "MinSizeRel" project
# - Add the following to "CMake Options", using the full path to the toolchain file for your operating system:
#    -DCMAKE_TOOLCHAIN_FILE=[path to this project goes here]/toolchain/Toolchain-VexCortexM3-MacOS.cmake
#
# To configure CLion to deploy to the Vex robot:
# - Choose the "Run -> Edit Configurations" menu option
# - Click on the "+" in the top-left corner and select "Application"
# - Enter the following in each field:
#    Single instance only:  <checked>
#    Name:                  Deploy to Robot
#    Target:                output.elf
#    Configuration:         MinSizeRel
#    Executable:            /usr/local/bin/pros
#    Program Arguments:     flash -b cmake-build-minsizerel/output.bin
#    Working Directory:     $PROJECT_DIR$
# - Click OK
# This should add a "Deploy to Robot" option to the toolbar.  Click the green arrow next to it to build and deploy.
#

cmake_minimum_required(VERSION 3.7)
project(vexbot)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES
        include/API.h
        include/hid.h
        include/main.h
        include/motor.h
        include/ports.h
        src/auto.c
        src/hid.c
        src/init.c
        src/motor.c
        src/opcontrol.c)

include_directories(include)

add_executable(output.elf ${SOURCE_FILES})

target_link_libraries(output.elf ${FIRMWARE_ROOT}/libpros.a m)

add_custom_command(TARGET output.elf POST_BUILD
        COMMAND ${GCC_ARM_ROOT}/bin/arm-none-eabi-size output.elf
        COMMAND ${GCC_ARM_ROOT}/bin/arm-none-eabi-objcopy output.elf -O binary output.bin)
