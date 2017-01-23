_AIS = G3-v2.00 B3-v1.10 B3-v1.20

DISASM = $(patsubst %, %.asm, $(_AIS))

%.asm: ./param/%.param
	./ais-disasm -a ais/$(<F:.param=.ais) < $< > $@

disasm: $(DISASM)

all: disasm
