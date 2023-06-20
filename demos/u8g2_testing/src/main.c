#include "clib/u8g2.h"
#include <util/delay.h>
#include "common.h"
#include "lcd.h"


// #define SPI_CS		(SRXE_PORTB | PIN0)
// #define SPI_SCK		(SRXE_PORTB | PIN1)
// #define SPI_MOSI	(SRXE_PORTB | PIN2)
// #define SPI_MISO	(SRXE_PORTB | PIN3)

// #define LCD_CS	 	(SRXE_PORTE | PIN7)
// #define LCD_DC	 	(SRXE_PORTD | PIN6)
// #define LCD_RESET	(SRXE_PORTG | PIN2)

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

// https://github.com/olikraus/u8g2/blob/master/sys/avr/avr-libc/lib/u8x8_avr.c
uint8_t u8x8_byte_avr_hw_spi (u8x8_t * u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  uint8_t *data;

  switch (msg) {
    case U8X8_MSG_BYTE_INIT:
      _srxe_spi_init();
      // srxeDigitalWrite(SPI_CS, HIGH);
      // srxePinMode(SPI_CS, OUTPUT);

      // SPCR = (_BV (SPE) | _BV (MSTR));

      // switch (u8x8->display_info->spi_mode) {
      //   case 0:
      //     break;
      //   case 1:
      //     SPCR |= _BV (CPHA);
      //     break;
      //   case 2:
      //     SPCR |= _BV (CPOL);
      //     break;
      //   case 3:
      //     SPCR |= _BV (CPOL);
      //     SPCR |= _BV (CPHA);
      //     break;
      // };

      // switch (F_CPU / u8x8->display_info->sck_clock_hz) {
      //   case 2:
      //     SPSR |= _BV (SPI2X);
      //     break;
      //   case 4:
      //     break;
      //   case 8:
      //     SPSR |= _BV (SPI2X);
      //     SPCR |= _BV (SPR0);
      //     break;
      //   case 16:
      //     SPCR |= _BV (SPR0);
      //     break;
      //   case 32:
      //     SPSR |= _BV (SPI2X);
      //     SPCR |= _BV (SPR1);
      //     break;
      //   case 64:
      //     SPCR |= _BV (SPR1);
      //     break;
      //   case 128:
      //     SPCR |= _BV (SPR1);
      //     SPCR |= _BV (SPR0);
      //     break;
      // }

      // srxePinMode(SPI_SCK, OUTPUT);
      // srxePinMode(SPI_MOSI, OUTPUT);
      // srxeDigitalWrite(SPI_CS, u8x8->display_info->chip_disable_level);
      break;
    case U8X8_MSG_BYTE_SET_DC:
      u8x8_gpio_SetDC(u8x8, arg_int);
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case U8X8_MSG_BYTE_SEND:
      data = (uint8_t *) arg_ptr;
      while (arg_int > 0) {
          SPDR = (uint8_t) * data;
          asm volatile("nop");
          while (!(SPSR & _BV (SPIF)));
          data++;
          arg_int--;
      }
      // _srxe_spi_transfer(*data);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      break;
    default:
      return 0;
  }
  
  return 1;
}

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
      srxePinMode(LCD_CS, OUTPUT);
      srxePinMode(LCD_DC, OUTPUT);
      srxePinMode(LCD_RESET, OUTPUT);

      // srxeDigitalWrite(LCD_CS, HIGH);
      // // then reset the screen
      // srxeDigitalWrite(LCD_RESET, HIGH);
      // _delay_ms(50);
      // srxeDigitalWrite(LCD_RESET, LOW);
      // _delay_ms(5);
      // srxeDigitalWrite(LCD_RESET, HIGH);
      break;
    // CS (chip select) pin: Output level in arg_int
    case U8X8_MSG_GPIO_CS:
      srxeDigitalWrite(LCD_CS, arg_int);
      break;
    // DC (data/cmd, A0, register select) pin: Output level in arg_int
    case U8X8_MSG_GPIO_DC:
      srxeDigitalWrite(LCD_DC, arg_int);
      break;
    // Reset pin: Output level in arg_int
    case U8X8_MSG_GPIO_RESET:
      srxeDigitalWrite(LCD_RESET, arg_int);
      break;
    default:
      u8x8_SetGPIOResult(u8x8, 1);
      break;
  }
  return 1;
}

int main() {
    u8g2_t u8g2; // a structure which will contain all the data for one display
    // u8g2_Setup_st7586s_s028hn118a_2
    // u8g2_Setup_st7586s_jlx384160_2
    // u8g2_Setup_st7586s_erc240160_2
    // u8g2_Setup_st7586s_ymc240160_2
    u8g2_Setup_st7586s_s028hn118a_2(&u8g2, U8G2_R0, u8x8_byte_avr_hw_spi, u8x8_gpio_and_delay);  // init u8g2 structure
    u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,

    for (;;) {
      u8g2_SetPowerSave(&u8g2, 0); // wake up display
      u8g2_ClearDisplay(&u8g2);

      _delay_ms(500);

      // hello world
      u8g2_SetFont(&u8g2, u8x8_font_chroma48medium8_r);
      u8g2_DrawStr(&u8g2, 10, 10, "Hello World!");
    }
    return 1;
}