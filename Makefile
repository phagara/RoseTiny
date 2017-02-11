PRG=rose
SRCS=main.c rc5.c
DEFS=-DF_CPU=9600000 -DIR_PIN=PB1 -DRED_PIN=PB0 -DGREEN_PIN=PB2 -DBLUE_PIN=PB4
MCU_TARGET=attiny13
L_FUSE=0x72
H_FUSE=0xFF
MCU_PROG=usbasp

CC=avr-gcc
CFLAGS=-std=gnu99 -W -Wall -pedantic -Wstrict-prototypes \
	-Wundef -funsigned-char -funsigned-bitfields \
	-ffunction-sections -fpack-struct -fshort-enums \
	-fdata-sections -ffreestanding -Os -g -gdwarf-2 \
	-Wl,--relax,--gc-sections -fno-tree-scev-cprop \
	-finline-limit=3 -fno-split-wide-types \
	-fno-inline-small-functions \
	-flto -fwhole-program -mtiny-stack \
	-mmcu=$(MCU_TARGET) $(DEFS)
LDFLAGS=-Wl,-Map,$(PRG).map

OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump

all: clean $(PRG).elf lst text eeprom check
	
install: flash

flash: text eeprom
	avrdude -p $(MCU_TARGET) -c $(MCU_PROG) \
		-U flash:w:$(PRG).hex \
		-U eeprom:w:$(PRG)_eeprom.hex \
		-U lfuse:w:$(L_FUSE):m \
		-U hfuse:w:$(H_FUSE):m 

check: $(PRG).elf
	avr-size -C --mcu=$(MCU_TARGET) $<

$(PRG).elf:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS)

clean:
	rm -rf *.o *.elf *.lst *.map *.hex *.bin *.srec

lst: $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# .text rom images
text: hex bin srec

hex:  $(PRG).hex
bin:  $(PRG).bin
srec: $(PRG).srec

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# .eprom rom images
eeprom: ehex ebin esrec

ehex:  $(PRG)_eeprom.hex
ebin:  $(PRG)_eeprom.bin
esrec: $(PRG)_eeprom.srec

%_eeprom.hex: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@ \
	|| { echo empty $@ not generated; exit 0; }
