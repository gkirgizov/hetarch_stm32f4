#!/bin/sh

TARGET=usb_test
BUILD_DIR=build
TARGET_ELF=${BUILD_DIR}/${TARGET}.elf

GDB=arm-none-eabi-gdb
GDBFLAGS=

if ! nc -z localhost 3333; then
    echo "\n\t[Error] OpenOCD is not running! Start it with: 'make openocd'\n";
    exit 1;
else
    ${GDB} -ex "target extended localhost:3333" \
        -ex "monitor arm semihosting enable" \
        -ex "monitor reset halt" \
        -ex "load" \
        -ex "monitor reset init" \
        ${GDBFLAGS} ${TARGET_ELF};
fi

