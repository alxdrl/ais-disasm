#!/bin/sh
ASMFILE=$1
BASEDIR=$(cd $(dirname $0)/.. ; pwd)
NBKEEP_HEAD=99
NBKEEP_TAIL=82
FILEBASE=$(echo $(basename $ASMFILE .asm) | sed --e 's/-[0-9]\{8\}-[0-9]\{4\}$//')
PARAMFILE=$BASEDIR/param/${FILEBASE}.param
head -n ${NBKEEP_HEAD} $PARAMFILE
cat $ASMFILE | \
awk --posix '/^[0-9a-f]{8}[^;]+;proc/ {proc=" as proc"} /^[0-9a-f]{8}                 [^:]+:/ {print "define symbol 0x" $1 " " $2 proc ; proc=""}' | tr -d ':'
tail -n ${NBKEEP_TAIL} $PARAMFILE
