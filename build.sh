#!/bin/sh
set -ex

BINUTILS_TIC6X_DIR=$HOME/binutils-tic6x

#
# wget 'http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
# tar xjvpf binutils-2.23.1.tar.bz2
# mkdir binutils-build
# cd binutils-build
# ../binutils/configure --prefix=${BINUTILS_TIC6X_DIR} --enable-shared --target=tic6x-none-elf --enable-install-libbfd --enable-install-libiberty --with-target-subdir
# make
# make install

SRCDIR=./src
BUILD_ARCH=`./config.guess`
TARGET_ARCH=tic6x-none-elf
TARGET_SUBDIR=
BINUTILS_TIC6X_TARGET_DIR=${BINUTILS_TIC6X_DIR}/${BUILD_ARCH}/${TARGET_ARCH}
BINUTILS_TIC6X_INCLUDE_DIR=${BINUTILS_TIC6X_TARGET_DIR}/include
BINUTILS_TIC6X_LIBRARY_DIR=${BINUTILS_TIC6X_TARGET_DIR}/lib

LDFLAGS="-lbfd -lopcodes -L${BINUTILS_TIC6X_LIBRARY_DIR}"
LDFLAGS="$LDFLAGS -liberty -L${BINUTILS_TIC6X_DIR}/lib/${TARGET_SUBDIR}"

CFLAGS="-I${BINUTILS_TIC6X_INCLUDE_DIR} -I${BINUTILS_TIC6X_DIR}/include -Werror"

gcc ${CFLAGS} -c ${SRCDIR}/ais-disasm.c ${CFLAGS}
gcc -o ais-disasm ais-disasm.o ${LDFLAGS}
