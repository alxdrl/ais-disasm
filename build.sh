#!/bin/sh
set -ex

BINUTILS_PREFIX=$HOME/binutils-tic6x

#
# wget 'http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
# tar xjvpf binutils-2.23.1.tar.bz2
# mkdir binutils-build
# cd binutils-build
# ../binutils/configure --prefix=${BINUTILS_PREFIX} --enable-shared --target=tic6x-none-elf --enable-install-libbfd --enable-install-libiberty --with-target-subdir
# make
# make install

SRCDIR=./src
INCDIR=./include
BUILD_ARCH=`./config.guess`
TARGET_ARCH=tic6x-none-elf
HOST_CPU=`echo $BUILD_ARCH | cut -d- -f1`
if test -d ${BINUTILS_PREFIX}/lib/${HOST_CPU} ; then
	HOST_SUBDIR=${HOST_CPU}
fi
BINUTILS_TIC6X_TARGET_DIR=${BINUTILS_PREFIX}/${BUILD_ARCH}/${TARGET_ARCH}
BINUTILS_TIC6X_INCLUDE_DIR=${BINUTILS_TIC6X_TARGET_DIR}/include
BINUTILS_TIC6X_LIBRARY_DIR=${BINUTILS_TIC6X_TARGET_DIR}/lib

LDFLAGS="-lbfd -lopcodes -L${BINUTILS_TIC6X_LIBRARY_DIR}"
LDFLAGS="$LDFLAGS -liberty -L${BINUTILS_PREFIX}/lib/${HOST_SUBDIR}"
LDFLAGS_TUI="-lncurses"

CFLAGS="-Wall -Werror -I${INCDIR} -I${BINUTILS_TIC6X_INCLUDE_DIR} -I${BINUTILS_PREFIX}/include"

gcc -c ${INCDIR}/ais.h -o /dev/null ${CFLAGS}
gcc -c ${INCDIR}/ais-helper.h -o /dev/null ${CFLAGS}
gcc -c ${INCDIR}/ais-print.h -o /dev/null ${CFLAGS}
gcc -c ${INCDIR}/ais-region.h -o /dev/null ${CFLAGS}
gcc -c ${INCDIR}/ais-function-table.h -o /dev/null ${CFLAGS}

gcc -c ${SRCDIR}/ais-helper.c ${CFLAGS}
gcc -c ${SRCDIR}/ais-print.c ${CFLAGS}
gcc -c ${SRCDIR}/ais-load.c ${CFLAGS}
#gcc -c ${SRCDIR}/ais-space.c ${CFLAGS}
gcc -c ${SRCDIR}/ais-disasm.c ${CFLAGS}
gcc -c ${SRCDIR}/ais-disasm-tui.c ${CFLAGS}
gcc -o ais-disasm ais-disasm.o ais-load.o ais-print.o ais-helper.o ${LDFLAGS}
gcc -o ais-disasm-tui ais-disasm-tui.o ${LDFLAGS_TUI}
