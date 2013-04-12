#!/usr/bin/awk -f

BEGIN { ep = -1 ; just_entered = 0 ; cycles = 1}

/<fetch/ { next }

{
	if (! /\|\|/ && ep >= 0) {
		ep = ep + cycles;
		cycles = 1;
	}
	if (ep > 5) {
		printf "E%d\n", ep
		print ";======================================";
		ep = -1;
		cycles = 1;
		if (!just_entered)
			inside = 0;
	}
	if (just_entered)
		just_entered = 0;
}

/^c0064764/ || /^c005f744/ || /^c005d8a8/ {
	ep = 0;
}

/[^]] b .S2 b3$/ || / b .S2 irp$/ || / b .S2X a25$/{
	ep = 0;
}

/[^]] bnop .S2 b[83],/ {
	ep = 0;
        cycles += gensub(/.*b3,([0-9]+).*/, "\\1", "g", $0);
}

/callp .*,a3/ {
	if (ep > 5) {
		printf "E%d; JUMP\n", ep
		cycles = 1;
	}
	if (inside) {
		ep = 0;
		cycles += 5;
	}
}

/ nop [0-9]+/ {
	if (ep >= 0)
		cycles += gensub(/.*nop ([0-9]+).*/, "\\1", "g", $0) - 1;
}

{
	if (!inside && ! / nop /) {
		print ";--------------------------------------";
		print "; Fragment start ";
		print ";"
		inside = 1;
		just_entered = 1;
	}
	if (ep >= 0) {
		printf "E%d ", ep
	} else {
		printf "   "
	}
	print $0
}
