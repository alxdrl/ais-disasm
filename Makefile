_AIS = G3-v2.00 B3-v1.10 G5-v1.10

DISASM = $(patsubst %, %.asm, $(_AIS))

%.asm: %.param
	./ais-disasm -a $(patsubst %.param, ./ais/%.ais, $<) < $< | ./fbound.awk > $@

disasm: $(DISASM)

all: disasm
