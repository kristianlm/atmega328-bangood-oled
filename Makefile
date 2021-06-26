
# atmega328 are have CKDIV8 fuse set by default from the factory.
# you can run this to program that fuse and make things 8 times faster:
#
#     avrdude -c atmelice_isp -p m328p -U lfuse:w:0xE2:m

# also, this is the uart command I'm using:
#
#     picocom --echo --databits 8 --stopbits 1 --flow n --parity e --baud 9600 /dev/ttyUSB0

firmware=oled
F_CPU=8000000
SOURCES=${firmware}.c twi.c font.c
CFLAGS=-DF_CPU=${F_CPU} -DTWI_FREQUENCY=100000

flash: ${firmware}.hex
	sudo avrdude -B 1MHz -c avrisp2 -p m328p -U flash:w:${firmware}.hex:i

.PHONY: flash

${firmware}.hex: ${firmware}.elf
	avr-objcopy -O ihex ${firmware}.elf ${firmware}.hex

${firmware}.elf: ${SOURCES}
	avr-gcc ${CFLAGS} ${SOURCES} -O5 -mmcu=atmega328 -g -o ${firmware}.elf -Wl,-u,vfprintf -lprintf_flt -lm

fft: fft.c
	${CC} ${CFLAGS} ${LDFLAGS} fft.c -o fft -lm

