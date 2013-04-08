#!/usr/bin/awk -f

BEGIN { b = 0xc00d7b98 }
{ printf("%s", $0) }
/b14\([0-9]+\)/ {
	off = gensub(/.*b14\(([0-9]+)\).*/, "\\1", "g", $0);
	printf("\t\t\t; lbl_%x [xref]", b + off);
}
{ printf("\n") }
