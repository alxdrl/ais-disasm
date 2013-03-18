#!/usr/bin/awk -f

function leave()
{
	if (!is_in) return;
	print ";";
	print "; Fragment boundary";
	print ";==========================================";
	is_in = 0;
	do_leave = 0;
	about_to_leave = 0;
	enter();
}

function enter()
{
	if ((/^;/ || /nop / || /<fetch /) && ! /ld.* \|\| nop/) return ;
	if (do_leave)
		leave();
	if (is_in) return;
	print ";------------------------------------------";
	print "; Fragment start";
	print ";";
	is_in = 1;
}

function xref_mvk(reg)
{
	if (msb[reg] != "" && lsb[reg] != "") {
		xref = sprintf("0x%04s%04s", msb[reg], lsb[reg]);
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
	enter();
	if (seen_ret && ep >= 5) {
		ep = 0;
		pushed = 0
		seen_ret = 0;
		seen_stack_plus = 0;
		leave();
	}
}

/addk \.S[12] [0-9]+,b15/ {
	if (about_to_leave) {
		do_leave = 1;
	} else {
		about_to_leave = 1;
	}
}

/callp .*,a3/ {
	if (about_to_leave) {
		do_leave = 1;
	} else {
		about_to_leave = 1;
	}
}

/[^]] bnop .S2 b3,/ {
	seen_ret = 1;
	ep = gensub(/.*b3,([0-9]+).*/, "\\1", "g", $0);
}

/b .S2 irp/ {
	seen_ret = 1;
}

/ nop [0-9]+/ {
	if (seen_ret) {
		ep += gensub(/.*nop ([0-9]+).*/, "\\1", "g", $0);
	}
}

! /^;/ && ! /\|\|/ && ! /<fetch/ && ! / nop [0-9]+/ && ! / bnop / || / ld.* \|\| nop/  {
	if (seen_ret) {
		ep++ ;
	}
}

/addk? \.[LSD][12] -[0-9]+,b15/ {
	about_to_leave = 0;
}

/,\*b15--\([0-9]+\)/ || /addk? \.[LSD][12] -[0-9]+,b15/ {
	if (pushed <= 0) {
		seen_ret = 0;
		ep = 0;
		pushed = 0;
	}
	x = gensub(/.*b15--\(([0-9]+)\).*/, "\\1", "g", $0);
	if (x == x + 0) pushed += x;
}

/addk? .*,b15/ {
	x = gensub(/.* addk? \.[LSD][12] (-?[0-9]+),b15.*/, "\\1", "g", $0);
	if (x == x + 0) pushed += x
}

/*\+\+b15\([0-9]+\),/ {
	x = gensub(/.*b15\(([0-9]+)\).*/, "\\1", "g", $0);
	if (x == x + 0) pushed -= x
}

/[^]] b .S2X? b3/ {
	seen_ret = 1;
	ep = 0;
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


