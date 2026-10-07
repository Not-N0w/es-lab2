#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include "kb.h"
#include "oled.h"
#include "fonts.h"

void SystemClock_Config(void);

static const char keymap[12] = {
	'1','2','3',
	'4','5','6',
	'7','8','9',
	'O','0','='
};

static int a = 0, b = 0, res = 0, len = 0, state = 0;
static char op = '+';

static char Get_Key(void) {
	const uint8_t rows[4] = {ROW1, ROW2, ROW3, ROW4};
	for (int r = 0; r < 4; r++) {
		uint8_t k = Check_Row(rows[r]);
		if (k == 0x04) return keymap[r*3];
		if (k == 0x02) return keymap[r*3 + 1];
		if (k == 0x01) return keymap[r*3 + 2];
	}
	return 0;
}

static void Calc_Press(char k) {
	if (k >= '0' && k <= '9') {
		if (state == 2) { a = 0; len = 0; state = 0; }
		int *x = (state == 0) ? &a : &b;
		if (len < 4) { *x = *x * 10 + (k - '0'); len++; }
	} else if (k == 'O') {
		if (state == 1 && len == 0) {
			op = (op == '+') ? '-' : (op == '-') ? '*' : '+';
		} else if (state != 1) {
			if (state == 2) a = res;
			op = '+'; b = 0; len = 0; state = 1;
		}
	} else if (k == '=') {
		if (state == 1 && len > 0) {
			res = (op == '+') ? a + b : (op == '-') ? a - b : a * b;
			state = 2;
		} else {
			a = b = res = len = state = 0;
		}
	}
}

static void Print(int y, char *s) {
	oled_SetCursor(0, y);
	oled_WriteString(s, Font_7x10, White);
}

static void Show_Help(void) {
	oled_Fill(Black);
	Print(0,  "Help:");
	Print(14, "1 2 3  0-9: digit");
	Print(26, "4 5 6  f: + - *");
	Print(38, "7 8 9  =: result");
	Print(50, "f 0 =  =: reset");
	oled_UpdateScreen();
	HAL_Delay(5000);
}

static void Calc_Show(void) {
	char s[24];
	oled_Fill(Black);
	Print(0, "Calculator");
	sprintf(s, "%d", a);
	Print(14, s);
	if (state == 1) {
		if (len > 0) sprintf(s, "%c %d", op, b);
		else sprintf(s, "%c", op);
		Print(26, s);
	}
	if (state == 2) {
		sprintf(s, "%c %d", op, b);
		Print(26, s);
		sprintf(s, "= %d", res);
		Print(40, s);
	}
	oled_UpdateScreen();
}

int main(void)
{
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_I2C1_Init();
	MX_USART6_UART_Init();
	oled_Init();
	Show_Help();
	Calc_Show();

	char last = 0;
	while (1) {
		char k = Get_Key();
		if (k && k != last) {
			Calc_Press(k);
			Calc_Show();
		}
		last = k;
		HAL_Delay(20);
	}
}

void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
	                            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
}

void Error_Handler(void)
{
}
