#!/usr/bin/env awk --non-decimal-data -f

function xref_mvk(reg)
{
	if (msb[reg] != "" && lsb[reg] != "") {
		xref = sprintf("0x%s%s", msb[reg], lsb[reg]);
		msb[reg] = "";
		lsb[reg] = "";
	}
}
function format_word()
{
	word = gensub(/.* ([a-fx0-9-]+),[ab]([0-9]|[1-3][0-9]).*/, "\\1", "g", $0); 
	word = sprintf("%04x", word);
}
{ xref = "" ; printf "%s", $0 }
{ areg = gensub(/.*,([ab][0-9]+)([^0-9].*|$)/, "\\1", "g", $0) ; }
! /mvk/ && ( /,[ab]([1-3][0-9]|[0-9])[^,]/ || /,[ab]([1-3][0-9]|[0-9])$/ ) {
#	printf "\t\t\t; touch %s", areg
	lsb[areg] = "";
	msb[areg] = "";
}
! /mvk/ && /,[ab]([1-3][0-9]|[0-9]):[ab]([1-3][0-9]|[0-9])([^0-9].*|$)/ {
	breg = gensub(/.*,[ab]([1-3][0-9]|[0-9]):([ab]([1-3][0-9]|[0-9])).*/, "\\2", "g", $0);
#	printf ":%s", breg
	lsb[breg] = "";
	msb[breg] = "";
}
/mvk[^h]/ {
	format_word();
	word = substr(word, length(word) - 3, 4);
	lsb[areg] = word;
#	printf "\t\t\t; mvk word [%s] = %s", areg, word;
	xref_mvk(areg);
}
/mvkh/ {
	format_word();
	if ( word ~ /0000$/ ) {
		word = substr(word, 1, length(word) - 4);
	}
	if (length(word) > 4) {
		word = substr(word, length(word) - 3, 4);
	} 
	msb[areg] = word
#	printf "\t\t\t; mvkh word [%s] = %s", areg, word;
	xref_mvk(areg);
}

{
  if (xref != "") { printf "\t\t\t; xref [%s] @%s", areg, xref ; }
  printf "\n";
}
