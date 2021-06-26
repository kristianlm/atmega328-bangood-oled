#include <math.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdio.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "twi.h"
#include "font.h"

#include "uart.c"

// Constants
int const address = 60;
int const commands = 0x00;
int const onecommand = 0x80;
int const data = 0x40;
int const onedata = 0xC0;

// OLED display **********************************************

void oled_init () {
  twi_start();
  twi_addressWrite(address);
  twi_write(commands);

  twi_write(0x20); // set memory mode
  //                       ,-- not supported on my devices 😦 (ignored)
  twi_write(0x02); // 00=horizontal, 01=vertical, 02=page

  twi_write(0xA1); // horizontal flip: 0xA0=off 0xA1=on
  twi_write(0xC8); // vertical flip:   0xC0=off 0xC8=on

  twi_write(0xDB); // set vcom detect
  twi_write(0x40); // brightness

  twi_write(0x81); twi_write(0x80); // set constrast

  twi_write(0x21); twi_write(0); twi_write(127);  // column range
  twi_write(0x22); twi_write(0); twi_write(7);  // page range


  // only charge pump and display on were required for my Bangood
  // purchase (https://banggood.app.link/nO5rGtZfphb)
  twi_write(0x8D); twi_write(0x14); // charge pump
  twi_write(0xAF); // display on
  twi_stop();
}

// send filler number times. set filler to 0 to clear the current
// page, for example.
void oled_fill(uint8_t number, uint8_t filler) {
  twi_start();
  twi_addressWrite(address);
  twi_write(0x40); // data mode (sending screen content)

  for(uint8_t i = 0 ; i < number ; i++) {
    twi_write(filler);
  }
  twi_stop();
}

// change the current page
void oled_page(uint8_t page) {
  twi_start();
  twi_addressWrite(address);
  twi_write(commands);
  twi_write(0xB0 | page);
  twi_stop();
}

// change the current column and page
void oled_goto(uint8_t column, uint8_t page) {
  twi_start();
  twi_addressWrite(address);
  twi_write(commands);
  twi_write(0x00 | (0x0F & (column<<0))); // column lower nibble
  twi_write(0x10 | (0x0F & (column>>4))); // column upper nibble
  twi_write(0xB0 | page);
  twi_stop();
}

// turn all pixels of the screen on or off
void oled_clear(uint8_t white) {
  for(uint8_t page = 0 ; page < 8 ; page++) {
    oled_page(page);
    oled_fill(128, white ? 0xFF : 0x00);
  }
}

// using send font57 pixels for character ch
void oled_print(const char ch) {
  twi_start();
  twi_addressWrite(address);
  twi_write(0x40); // data mode (sending screen content)

  twi_write( 0x00 );
  twi_write(pgm_read_byte(&(font57[((unsigned char)ch*5 )])));
  twi_write(pgm_read_byte(&(font57[((unsigned char)ch*5 + 1)])));
  twi_write(pgm_read_byte(&(font57[((unsigned char)ch*5 + 2)])));
  twi_write(pgm_read_byte(&(font57[((unsigned char)ch*5 + 3)])));
  twi_write(pgm_read_byte(&(font57[((unsigned char)ch*5 + 4)])));
  twi_write( 0x00 );

  twi_stop();
}

// stdout-able output
static uint8_t oled_x = 0;
static uint8_t oled_y = 0;
static int oled_putchar(char c, FILE *stream) {
  if(c == '\n') {
    oled_x = 0;
    oled_goto(0, ++oled_y);
    return 0;
  }
  oled_print(c);
  oled_x++;
  if(oled_x >= 18) {
    oled_x = 0;
    oled_goto(0, ++oled_y);
  }
  return 0;
}

static FILE stdout_oled = FDEV_SETUP_STREAM(oled_putchar, NULL, _FDEV_SETUP_WRITE);

// power OLED directly from GPIO. should be safe since it only
// consumes ~10mA.
void power_oled_on() {
  DDRC  |=  (1<<DDC2) | (1<<DDC3);
  PORTC |=  (1<<PORTC2) | (1<<PORTC3);
}

void power_oled_off() {
  PORTC &=  ~(1<<PORTC2) | ~(1<<PORTC3);
  DDRC  &=  ~(1<<DDC2) | ~(1<<DDC3);
}

int main() {
  stdout = &stdout_oled;
  DDRC  =  (1<<DDC4) | (1<<DDC5);
  PORTC |=  (1<<PORTC4) | (1<<PORTC5); // enable pullups
  power_oled_on();
  twi_init();
  oled_init();

  // flash a bit so we know we're alive
  oled_clear(1);
  oled_clear(0);

  printf("   Hello World!\n");

  for(uint8_t i = 1 ; i < 127 ; i++) {
    putc(i == '\n' ? 0 : i, stdout);
  }
  while(1) {}
}
