#!/bin/sh
#set -ex
set -e
OPT_PREFIX=${HOME}/opt
BUILD_PREFIX=${HOME}/build

BINUTILS_SRC=$1

[ -z "${BINUTILS_SRC}" ] && { echo "*** error: missing binutils source dir argument" ; exit 1 ; }

BINUTILS_SRC=`cd $BINUTILS_SRC ; pwd`

[ -d "${BINUTILS_SRC}" ] || { echo "*** error: $BINUTILS_SRC does not exists ?" ; exit 1 ; }

BINUTILS_NAME=`basename ${BINUTILS_SRC}`

[ -z "${BINUTILS_NAME}" ] && { echo "*** error: unable to get basename for ${BINUTILS_SRC}" ; exit 1 ; }

BINUTILS_PREFIX=${OPT_PREFIX}/binutils-tic6x
BINUTILS_BUILD=$HOME/build/${BINUTILS_NAME}

# [ -d "${BINUTILS_BUILD}" ] && { rm -rf ${BINUTILS_BUILD} ; }

PROC_NUM=`nproc 2> /dev/null || sysctl -n hw.ncpu 2> /dev/null`

mkdir -p "${BINUTILS_BUILD}"
cd "${BINUTILS_BUILD}"
${BINUTILS_SRC}/configure --prefix=${BINUTILS_PREFIX} --enable-shared --target=tic6x-none-elf --enable-install-libbfd --enable-install-libiberty --with-target-subdir --disable-werror
make -j${PROCNUM:-1}
make install
