#!/usr/bin/awk -f

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

{
	if (seen_ret && ep >= 5 ) {
		seen_ret = 0;
		print ";";
		print "; Function or Fragment boundary";
        print ";==========================================";
		ep = 0;
		in_func = 0;
		in_fragment = 0;
		pushed = 0
	}
}

/ nop [0-9]+/ { if (seen_ret) { ep += gensub(/.*nop ([0-9]+).*/, "\\1", "g", $0); } }
! /^;/ && ! / nop [0-9]+/  && $3 ~ /^[^\|]/ && ! /<fetch/ { if (seen_ret) ep++ ; }

! /^;/ && ! /<fetch/ && ! /,\*b15--\([0-9]+\)/ && ! /*\+\+b15\([0-9]+\),/ && ! /nop [0-9]+/ && ! /addk .*,b15/ {
	if (!in_func && !in_fragment) {
		in_fragment = 1; 
		print ";------------------------------------------";
		print "; Fragment start";
		print ";";
	}
}

/,\*b15--\([0-9]+\)/ || /addk \.S[12] -[0-9]+,b15/ {
	if (in_func && pushed <= 0) {
		seen_ret = 0;
		if (!in_fragment) {
			print ";";
			print "; Function or Fragment boundary";
	    	print ";==========================================";
		}
		ep = 0;
		pushed = 0;
		in_func = 0;
		in_fragment = 0;
	}
	was_in_func = in_func;
	in_func = 1;
	if (in_fragment && !in_func ) {
		print ";";
		print "; Fragment boundary";
	    print ";==========================================";
		in_fragment = 0;
	}
	if (!was_in_func && !in_fragment) {
		print ";------------------------------------------";
		print "; Function start";
		print ";";
	}
	in_fragment = 0;
	x = gensub(/.*b15--\(([0-9]+)\).*/, "\\1", "g", $0);
    if (x == x + 0) pushed += x;
}

/addk .*,b15/ {
	x = gensub(/.* addk \.S[12] (-?[0-9]+),b15.*/, "\\1", "g", $0);
	pushed += x
}

/*\+\+b15\([0-9]+\),/ {
	pushed -= gensub(/.*b15\(([0-9]+)\).*/, "\\1", "g", $0);
}

/ b .S2 b3/ {
	in_func = 1
	seen_ret = 1;
	ep = 0;
}
/bnop .S2 b3,/ {
	in_func = 1
	seen_ret = 1;
	ep = gensub(/.*b3,([0-9]+).*/, "\\1", "g", $0);
}
{ xref = "" ; }
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
  printf "%s", $0
  if (xref != "") { printf "\t\t\t; xref [%s] @%s", areg, xref ; }
#  if (pushed) { printf "\t(stack:%s)", pushed ; }
#  if (seen_ret) { printf "\t(ep:%s)", ep ; }
  printf "\n";
}
