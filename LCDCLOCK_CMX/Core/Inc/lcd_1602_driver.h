/*
 * lcd_1602_driver.h
 *
 *  Created on: Aug 7, 2026
 *      Author: Dmnd098
 */

#ifndef INC_LCD_1602_DRIVER_H_
#define INC_LCD_1602_DRIVER_H_

#include "stm32f4xx_hal.h"

//Auxiliary Macros
#define lcd_send_cmd(lcd, cmd) lcd_send(lcd, cmd, 0)
#define lcd_write(lcd, data) lcd_send(lcd, data, 1)


//The Commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

typedef struct{
	GPIO_TypeDef *GPIO_PORT;
	uint16_t pin;
} LCD_GPIO;

typedef struct{
	// The Pins
	LCD_GPIO *pin_rs, *pin_en, *pin_D7, *pin_D6, *pin_D5, *pin_D4;
	TIM_HandleTypeDef *lcd_tim;
	uint8_t lcd_col, lcd_row;
	uint8_t row_offset[4];
} lcd_4bit_struct;

void lcd_send(lcd_4bit_struct *lcd, uint8_t data, uint8_t rs_mode);
void lcd_init(lcd_4bit_struct *lcd);
void lcd_set_row_offset(lcd_4bit_struct *lcd, uint8_t row0, uint8_t row1, uint8_t row2, uint8_t row3);

//LCD Commands
void lcd_display_on(lcd_4bit_struct *lcd);
void lcd_display_off(lcd_4bit_struct *lcd);
void lcd_clear(lcd_4bit_struct *lcd);
void lcd_home(lcd_4bit_struct *lcd);
void lcd_set_cursor(lcd_4bit_struct *lcd, uint8_t col, uint8_t row);
void lcd_cursor_on(lcd_4bit_struct *lcd);
void lcd_cursor_off(lcd_4bit_struct *lcd);
void lcd_blink_on(lcd_4bit_struct *lcd);
void lcd_blink_off(lcd_4bit_struct *lcd);
void lcd_scroll_left(lcd_4bit_struct *lcd);
void lcd_scroll_right(lcd_4bit_struct *lcd);
void lcd_LTR(lcd_4bit_struct *lcd);
void lcd_RTL(lcd_4bit_struct *lcd);
void lcd_auto_scroll_on(lcd_4bit_struct *lcd);
void lcd_auto_scroll_off(lcd_4bit_struct *lcd);
void lcd_create_char(lcd_4bit_struct *lcd, uint8_t location, uint8_t charmap[]);

//Helper functions
void lcd_print(lcd_4bit_struct *lcd, char *text);
void lcd_println(lcd_4bit_struct *lcd, char *text);

#endif /* INC_LCD_1602_DRIVER_H_ */
