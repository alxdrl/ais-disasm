#!/usr/bin/awk -f

function xref_mvk(reg)
{
	if (msb[reg] != "" && lsb[reg] != "") {
		xref = "0x" msb[reg] lsb[reg];
		msb[reg] = "";
		lsb[reg] = "";
	}	
}
{ xref = "" ; }
{ ret = 0 ; }
/mvk .[^ ]+ 0x..../ {
	areg = gensub(/.* 0x[a-f0-9]+,([ab][0-9]+).*/, "\\1", "g", $0)
	word = gensub(/.* 0x([a-f0-9]+),.*/, "\\1", "g", $0);
	word = gensub(/^ffff(....)$/, "\\1", "g", word);
	word = gensub(/^(....)$/, "\\1", "g", word);
	lsb[areg] = sprintf("%04s", word);
	xref_mvk(areg);
}
/mvkh .[^ ]+ 0x........,/ {
	areg = gensub(/.* 0x........,([ab][0-9]+).*/, "\\1", "g", $0)
	msb[areg] = gensub(/.* 0x(....)....,[ab][0-9]+.*/, "\\1", "g", $0)
	xref_mvk(areg);
}

{
  if (xref != "") { print $0 "\t\t; xref @" xref ; }
  else { print $0 }
}
