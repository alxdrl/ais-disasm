#!/bin/sh
set -ex

BINUTILS_TIC6X_DIR=$HOME/binutils-tic6x

#
# tar xjvpf binutils-2.23.1.tar.bz2
# mkdir binutils-build
# cd binutils-build
# ../binutils-2.23.1/opcodes/configure --prefix=${BINUTILS_TIC6X_DIR} --enable-shared --target=tic6x-none-elf --enable-install-libbfd
# make
# make install

SRCDIR=./src
BUILD_ARCH=i686-pc-linux-gnu
TARGET_ARCH=tic6x-none-elf
BINUTILS_TIC6X_TARGET_DIR=${BINUTILS_TIC6X_DIR}/${BUILD_ARCH}/${TARGET_ARCH}
BINUTILS_TIC6X_INCLUDE_DIR=${BINUTILS_TIC6X_TARGET_DIR}/include
BINUTILS_TIC6X_LIBRARY_DIR=${BINUTILS_TIC6X_TARGET_DIR}/lib

LDFLAGS="-lbfd -lopcodes -liberty  -L${BINUTILS_TIC6X_LIBRARY_DIR} -L${BINUTILS_TIC6X_DIR}/lib"
CFLAGS="-I${BINUTILS_TIC6X_INCLUDE_DIR} -Werror"

gcc ${CFLAGS} -c ${SRCDIR}/ais-disasm.c ${CFLAGS}
gcc -o ais-disasm ais-disasm.o ${LDFLAGS}
