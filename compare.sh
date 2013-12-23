#!/bin/sh
export LD_LIBRARY_PATH=~/opt/i686-pc-linux-gnu/tic6x-none-elf/lib
make
cat G3-v2.00.asm | grep -v '<fetch packet' | cut -c26- | sed 's/[[:blank:]]*;.*//' | grep -v '^[[:blank:]]*$' > G3-v2.00.asm.orig.stripped
cat disasm/G3-v2.00.asm | grep '^[0-9a-f]\{8\} ' | cut -c26- | sed 's/[[:blank:]]*;.*//' | grep -v '^[[:blank:]]*$' > G3-v2.00.asm.current.stripped
diff -u G3-v2.00.asm.orig.stripped G3-v2.00.asm.current.stripped | vim -
