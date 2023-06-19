#include "clib/u8g2.h"
#include <util/delay.h>
// #include "lcdbase.h"

#define P_CPU_NS (1000000000UL / F_CPU)

uint8_t u8x8_avr_delay (u8x8_t * u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	uint8_t cycles;

	switch(msg) {
		case U8X8_MSG_DELAY_NANO:  // delay arg_int * 1 nano second
			// At 20Mhz, each cycle is 50ns, the call itself is slower.
			break;
		case U8X8_MSG_DELAY_100NANO:       // delay arg_int * 100 nano seconds
			// Approximate best case values...
			#define CALL_CYCLES 26UL
			#define CALC_CYCLES 4UL
			#define RETURN_CYCLES 4UL
			#define CYCLES_PER_LOOP 4UL

			cycles = (100UL * arg_int) / (P_CPU_NS * CYCLES_PER_LOOP);

			if (cycles > CALL_CYCLES + RETURN_CYCLES + CALC_CYCLES)
				break;

			__asm__ __volatile__ (
			  "1: sbiw %0,1" "\n\t"  // 2 cycles
			  "brne 1b":"=w" (cycles):"0" (cycles)  // 2 cycles
			);
			break;
		case U8X8_MSG_DELAY_10MICRO: // delay arg_int * 10 micro seconds
			while( arg_int-- ) _delay_us(10);
			break;
		case U8X8_MSG_DELAY_MILLI:  // delay arg_int * 1 milli second
			while( arg_int-- ) _delay_ms(1);
			break;
		default:
			return 0;
	}

	return 1;
}

// #define SPI_CS		(SRXE_PORTB | PIN0)
// #define SPI_SCK		(SRXE_PORTB | PIN1)
// #define SPI_MOSI	(SRXE_PORTB | PIN2)
// #define SPI_MISO	(SRXE_PORTB | PIN3)

// #define LCD_CS	 	(SRXE_PORTE | PIN7)
// #define LCD_DC	 	(SRXE_PORTD | PIN6)
// #define LCD_RESET	(SRXE_PORTG | PIN2)

#define CS_DDR DDRE
#define CS_PORT PORTE
#define CS_BIT PIN7

#define DC_DDR DDRD
#define DC_PORT PORTD
#define DC_BIT PIN6

#define RESET_DDR DDRG
#define RESET_PORT PORTG
#define RESET_BIT PIN2

// https://github.com/olikraus/u8g2/blob/master/sys/avr/avr-libc/atmega328p/main.c
uint8_t
u8x8_gpio_and_delay (u8x8_t * u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  // Re-use library for delays
  if (u8x8_avr_delay(u8x8, msg, arg_int, arg_ptr))
    return 1;

  switch (msg) {
    // called once during init phase of u8g2/u8x8
    // can be used to setup pins
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
      CS_DDR |= _BV(CS_BIT);
      DC_DDR |= _BV(DC_BIT);
      RESET_DDR |= _BV(RESET_BIT);
      break;
    // CS (chip select) pin: Output level in arg_int
    case U8X8_MSG_GPIO_CS:
      if (arg_int)
        CS_PORT |= _BV(CS_BIT);
      else
        CS_PORT &= ~_BV(CS_BIT);
      break;
    // DC (data/cmd, A0, register select) pin: Output level in arg_int
    case U8X8_MSG_GPIO_DC:
      if (arg_int)
        DC_PORT |= _BV(DC_BIT);
      else
        DC_PORT &= ~_BV(DC_BIT);
      break;
    // Reset pin: Output level in arg_int
    case U8X8_MSG_GPIO_RESET:
      if (arg_int)
        RESET_PORT |= _BV(RESET_BIT);
      else
        RESET_PORT &= ~_BV(RESET_BIT);
      break;
    default:
      u8x8_SetGPIOResult(u8x8, 1);
      break;
  }
  return 1;
}

int main() {
    u8g2_t u8g2; // a structure which will contain all the data for one display
    u8g2_Setup_st7586s_s028hn118a_2(&u8g2, U8G2_R0, u8x8_byte_4wire_sw_spi, u8x8_gpio_and_delay);  // init u8g2 structure
    u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,
    u8g2_SetPowerSave(&u8g2, 0); // wake up display

    while(1) {
        // hello world
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
        u8g2_DrawStr(&u8g2, 0, 15, "Hello World!");
        u8g2_SendBuffer(&u8g2);
    }

    return 1;
}