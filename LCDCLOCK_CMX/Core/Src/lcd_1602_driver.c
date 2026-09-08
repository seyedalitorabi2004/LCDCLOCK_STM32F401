//Implementation file for the driver header

#include "lcd_1602_driver.h"
#include <string.h>

//Auxiliary Macros
#define bitat(num, idx) ((num & (1 << idx)) >> (idx))

uint8_t m_lcd_option = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
uint8_t m_display_control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
uint8_t m_display_mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

volatile void delay_us_lcd(uint16_t us, TIM_HandleTypeDef *tim_a){
	__HAL_TIM_SET_COUNTER(tim_a,0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(tim_a) < us);  // wait for the counter to reach the us input in the parameter
}

void lcd_pulse_enable(lcd_4bit_struct *lcd){
	// Enable Low
	HAL_GPIO_WritePin(lcd->pin_en->GPIO_PORT, lcd->pin_en->pin, 0);
	delay_us_lcd(1, lcd->lcd_tim);
	// Enable High
	HAL_GPIO_WritePin(lcd->pin_en->GPIO_PORT, lcd->pin_en->pin, 1);
	delay_us_lcd(1, lcd->lcd_tim);
	// Enable Low
	// at least 37 us
	HAL_GPIO_WritePin(lcd->pin_en->GPIO_PORT, lcd->pin_en->pin, 0);
	delay_us_lcd(100, lcd->lcd_tim);
}

void write_nibble_data(lcd_4bit_struct *lcd, uint8_t data){
	HAL_GPIO_WritePin(lcd->pin_D7->GPIO_PORT, lcd->pin_D7->pin, bitat(data, 3));
	HAL_GPIO_WritePin(lcd->pin_D6->GPIO_PORT, lcd->pin_D6->pin, bitat(data, 2));
	HAL_GPIO_WritePin(lcd->pin_D5->GPIO_PORT, lcd->pin_D5->pin, bitat(data, 1));
	HAL_GPIO_WritePin(lcd->pin_D4->GPIO_PORT, lcd->pin_D4->pin, bitat(data, 0));
	lcd_pulse_enable(lcd);
}

void lcd_send(lcd_4bit_struct *lcd, uint8_t data, uint8_t rs_mode){
	HAL_GPIO_WritePin(lcd->pin_rs->GPIO_PORT, lcd->pin_rs->pin, rs_mode);

	//Write the higher 4Bits first
	write_nibble_data(lcd, data >> 4);
	write_nibble_data(lcd, data);
}

void lcd_init(lcd_4bit_struct *lcd){

	if(lcd->lcd_row > 1) m_lcd_option |= LCD_2LINE;
	lcd_set_row_offset(lcd, 0x00, 0x40, 0x00 + lcd->lcd_col, 0x40 + lcd->lcd_col);

	// We need atleast 40MS Delay after Turning the lcd on.
	delay_us_lcd(50000, lcd->lcd_tim);
	// Setting rs and en to low
	HAL_GPIO_WritePin(lcd->pin_rs->GPIO_PORT, lcd->pin_rs->pin, 0);
	HAL_GPIO_WritePin(lcd->pin_en->GPIO_PORT, lcd->pin_en->pin, 0);

	//Setting D4, D5 to High, Indicating 4bit operation
	//Also we send the command 3 times so that we are successful
	write_nibble_data(lcd, 0x03);
	delay_us_lcd(4100, lcd->lcd_tim);

	write_nibble_data(lcd, 0x03);
	delay_us_lcd(4100, lcd->lcd_tim);

	write_nibble_data(lcd, 0x03);
	delay_us_lcd(150, lcd->lcd_tim);

	//setting it to 4bit mode
	write_nibble_data(lcd, 0x02);

	//Setting LCD Function
	lcd_send_cmd(lcd, LCD_FUNCTIONSET | m_lcd_option);

	lcd_display_on(lcd);

	lcd_clear(lcd);

	lcd_send_cmd(lcd, LCD_ENTRYMODESET | m_display_mode);

}

void lcd_set_row_offset(lcd_4bit_struct *lcd, uint8_t row0, uint8_t row1, uint8_t row2, uint8_t row3){
	lcd->row_offset[0] = row0;
	lcd->row_offset[1] = row1;
	lcd->row_offset[2] = row2;
	lcd->row_offset[3] = row3;
}

//LCD Commands
void lcd_display_on(lcd_4bit_struct *lcd) {
	m_display_control |= LCD_DISPLAYON;
	lcd_send_cmd(lcd, LCD_DISPLAYCONTROL | m_display_control);
}

void lcd_display_off(lcd_4bit_struct *lcd) {
	m_display_control &= ~LCD_DISPLAYON;
	lcd_send_cmd(lcd, LCD_DISPLAYCONTROL | m_display_control);
}

//Clear The Display and set cursor to home, wait for the Command to settle
void lcd_clear(lcd_4bit_struct *lcd) {
	lcd_send_cmd(lcd, LCD_CLEARDISPLAY);
	delay_us_lcd(2000, lcd->lcd_tim);
}

//Set the cursor to home, wait for the Command to settle
void lcd_home(lcd_4bit_struct *lcd) {
	lcd_send_cmd(lcd, LCD_RETURNHOME);
	delay_us_lcd(2000, lcd->lcd_tim);
}

//Set the cursor position
void lcd_set_cursor(lcd_4bit_struct *lcd, uint8_t col, uint8_t row) {
	const size_t max_lines = sizeof(lcd->row_offset)/sizeof(*lcd->row_offset);

	if (row >= max_lines) row = max_lines-1;
	if (row >= lcd->lcd_row) row = lcd->lcd_row - 1;

	lcd_send_cmd(lcd, LCD_SETDDRAMADDR | (col + lcd->row_offset[row]));

}

void lcd_cursor_on(lcd_4bit_struct *lcd) {
	m_display_control |= LCD_CURSORON;
	lcd_send_cmd(lcd, LCD_DISPLAYCONTROL | m_display_control);
}

void lcd_cursor_off(lcd_4bit_struct *lcd) {
	m_display_control &= ~LCD_CURSORON;
	lcd_send_cmd(lcd, LCD_DISPLAYCONTROL | m_display_control);
}

void lcd_blink_on(lcd_4bit_struct *lcd) {
	m_display_control |= LCD_BLINKON;
	lcd_send_cmd(lcd, LCD_DISPLAYCONTROL | m_display_control);
}

void lcd_blink_off(lcd_4bit_struct *lcd) {
	m_display_control &= ~LCD_BLINKON;
	lcd_send_cmd(lcd, LCD_DISPLAYCONTROL | m_display_control);
}

void lcd_scroll_left(lcd_4bit_struct *lcd) {
	lcd_send_cmd(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void lcd_scroll_right(lcd_4bit_struct *lcd) {
	lcd_send_cmd(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void lcd_LTR(lcd_4bit_struct *lcd) {
	m_display_mode |= LCD_ENTRYLEFT;
	lcd_send_cmd(lcd, LCD_ENTRYMODESET | m_display_mode);
}

void lcd_RTL(lcd_4bit_struct *lcd) {
	m_display_mode &= ~LCD_ENTRYLEFT;
	lcd_send_cmd(lcd, LCD_ENTRYMODESET | m_display_mode);
}

void lcd_auto_scroll_on(lcd_4bit_struct *lcd) {
	m_display_mode |= LCD_ENTRYSHIFTINCREMENT;
	lcd_send_cmd(lcd, LCD_ENTRYMODESET | m_display_mode);
}
void lcd_auto_scroll_off(lcd_4bit_struct *lcd) {
	m_display_mode &= ~LCD_ENTRYSHIFTINCREMENT;
	lcd_send_cmd(lcd, LCD_ENTRYMODESET | m_display_mode);
}

void lcd_create_char(lcd_4bit_struct *lcd, uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // Keep only the 3 LSBs
	lcd_send_cmd(lcd, LCD_SETCGRAMADDR | (location << 3));
	for (int i = 0; i < 8; i++){
		lcd_write(lcd, charmap[i]);
	}
}

void lcd_print(lcd_4bit_struct *lcd, char *text) {
	size_t sz = strlen(text);
	while (sz--){
		lcd_write(lcd, *text++);
	}
}

void lcd_println(lcd_4bit_struct *lcd, char *text) {
	lcd_print(lcd, text);
	lcd_print(lcd, "\r\n");
}
