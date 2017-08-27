# CMake Toolchain configuration file
# Prereqs: install PROS from https://pros.cs.purdue.edu/

# Set the path to the gcc cross-compiler for the Vex Cortex-M3 ARM, as installed on MacOS by the PROS installer
set(GCC_ARM_ROOT /Applications/PROS_2.0/gcc-arm-none-eabi-4_7-2014q2)

# A default PROS project should have a 'firmware' subdirectory with: STM32F10x.ld cortex.ld libpros.a
set(FIRMWARE_ROOT ${PROJECT_SOURCE_DIR}/firmware)

## Settings below here configure GCC cross-compiling for the Vex EDR controller.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_C_COMPILER ${GCC_ARM_ROOT}/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${GCC_ARM_ROOT}/bin/arm-none-eabi-g++)
set(CMAKE_FIND_ROOT_PATH ${GCC_ARM_ROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# The following are based on https://github.com/purduesigbots/pros/blob/master/template/common.mk
set(MCUAFLAGS "-mthumb -mcpu=cortex-m3 -mlittle-endian" CACHE STRING "")
set(MCUCFLAGS "-mthumb -mcpu=cortex-m3 -mlittle-endian -mfloat-abi=soft" CACHE STRING "")
set(MCULFLAGS "-nostartfiles -Wl,-static -B${FIRMWARE_ROOT} -Wl,-u,VectorTable -Wl,-T -Xlinker ${FIRMWARE_ROOT}/cortex.ld" CACHE STRING "")
set(COMPILER_FLAGS "-Wall ${MCUCFLAGS} -Os -ffunction-sections -fsigned-char -fomit-frame-pointer -fsingle-precision-constant" CACHE STRING "")

set(CMAKE_C_FLAGS "${COMPILER_FLAGS} -Werror=implicit-function-declaration" CACHE STRING "")
set(CMAKE_CXX_FLAGS "${COMPILER_FLAGS} -fno-exceptions -fno-rtti -felide-constructors" CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS "${MCULFLAGS} -Wl,--gc-sections ${FIRMWARE_ROOT}/libpros.a -lgcc -lm" CACHE STRING "")
