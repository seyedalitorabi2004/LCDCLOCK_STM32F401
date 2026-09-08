/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "lcd_1602_driver.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {uint8_t hour; uint8_t minute;} time_struct;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
uint8_t btn_set_state = 0; //Status of the Set Button
uint8_t btn_increment_state = 0; //Status of the Increment Button
uint8_t btn_set_down = 0; //If the button is kept down, this will enable.
uint8_t btn_increment_down = 0;

uint8_t lcd_update_pending = 0; // Whether if we need to Update the Display, Since Our Program basically tuns very fast
                                // Display cannot keep up with it, so we update it only when needed.
uint8_t lcd_clear_enable = 0;   // Only Clear When Needed.

uint8_t cnt = 0; // A Prescaler For the Display, As the tim2 interrupts at around every 20ms, we use this to Slow the Display even further.

// Initializing the LCD Structs
LCD_GPIO rs_pin = {.GPIO_PORT = GPIOB, .pin = GPIO_PIN_5};
LCD_GPIO en_pin = {.GPIO_PORT = GPIOB, .pin = GPIO_PIN_6};
LCD_GPIO D4_pin = {.GPIO_PORT = GPIOA, .pin = GPIO_PIN_9};
LCD_GPIO D5_pin = {.GPIO_PORT = GPIOA, .pin = GPIO_PIN_10};
LCD_GPIO D6_pin = {.GPIO_PORT = GPIOA, .pin = GPIO_PIN_11};
LCD_GPIO D7_pin = {.GPIO_PORT = GPIOA, .pin = GPIO_PIN_12};
lcd_4bit_struct theLCD = {.pin_rs = &rs_pin, .pin_en = &en_pin,
                        .pin_D4 = &D4_pin, .pin_D5 = &D5_pin, .pin_D6 = &D6_pin, .pin_D7 = &D7_pin,
                        .lcd_tim = &htim1,
                        .lcd_col = 16, .lcd_row = 2};

time_struct theTime = {0};
time_struct temp_time = {0};

// The State of the program.
enum {SET_HOUR = 0, SET_MINUTE = 1, CLOCK = 2} program_state;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
void main_loop(void); // The Main Display
void setting_menu(void); // The Setting Menu
void set_time(time_struct time); // Set the time
time_struct get_time(void); //Get the time.
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  // We start the Timers in here
  HAL_TIM_Base_Start(&htim1);
  HAL_TIM_Base_Start_IT(&htim2);

  //Adding a 100ms delay, so that both the Power and all the peripherals Settle down.
  HAL_Delay(100);

  //We Init the Display and Clear it.
  lcd_init(&theLCD);
  lcd_clear(&theLCD);

  //We set the Program state, if the Backup register is empty, we switch to the time set.
  program_state = CLOCK;
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0x2345) // If The RTC is not Set already
  {
    program_state = SET_HOUR;
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (program_state == CLOCK)
      main_loop();
    else
      setting_menu();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 83;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1679999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB12 PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PA9 PA10 PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB5 PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
  //Every 20ms Sample the Button states.
  btn_increment_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);
  btn_set_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);

  //Every 100ms, Issue an Update to the LCD.
  if (cnt == 4)
  {
    lcd_update_pending = 1;
    cnt = 0;
  }
  else
  {
    cnt++;
  }
}

void main_loop(void)
{
  //If LCD Has a pending Update, Update it.
  if (lcd_update_pending == 1)
  {
    //Read the time.
    theTime = get_time();
    //We don't need the clearing, however it's still included.
    if ((theTime.minute < 10) && (lcd_clear_enable == 1))
    {
      lcd_clear(&theLCD);
      lcd_clear_enable = 0;
    }
    if (theTime.minute > 10 && (lcd_clear_enable == 0))
    {
      lcd_clear_enable = 1;
    }

    //Printing the Title
    lcd_set_cursor(&theLCD, 0, 0);
    lcd_print(&theLCD, "Time Now");

    //Printing the Time
    lcd_set_cursor(&theLCD, 0, 1);
    char x[16];
    x[15] = '\0';
    sprintf(x, "%02d:%02d", theTime.hour, theTime.minute);
    lcd_print(&theLCD, x);

    //Set the Update flag to 0, We have Serviced the Incoming LCD Update.
    lcd_update_pending = 0;
  }

  // If the Setting Button is Pressed, but Wasn't Held down before.
  if (btn_set_state == 1 && !btn_set_down)
  {
    lcd_blink_on(&theLCD);
    lcd_clear(&theLCD);
    temp_time = get_time();
    btn_set_down = 1;
    program_state = SET_HOUR;
  }
  if (!btn_set_state)
  {
    btn_set_down = 0;
  }
}

void setting_menu(void)
{
  //If LCD Has a pending Update, Update it.
  if (lcd_update_pending == 1)
  {
    if (lcd_clear_enable)
    {
      lcd_clear(&theLCD);
      lcd_clear_enable = 0;
    }
    //pps is the Cursor Position
    uint8_t pps = 0;
    lcd_set_cursor(&theLCD, 0, 0);
    lcd_print(&theLCD, "Set Clock ");
    if (program_state == SET_HOUR)
    {
      lcd_print(&theLCD, "HR");
      pps = 1;
    }
    else if (program_state == SET_MINUTE)
    {
      lcd_print(&theLCD, "Min");
      pps = 4;
    }

    //Printing the Time
    lcd_set_cursor(&theLCD, 0, 1);
    char x[16];
    x[15] = '\0';
    sprintf(x, "%02d:%02d", temp_time.hour, temp_time.minute);
    lcd_print(&theLCD, x);
    lcd_set_cursor(&theLCD, pps, 1);

    //Set the Update flag to 0, We have Serviced the Incoming LCD Update.
    lcd_update_pending = 0;
  }

  // If the Setting Button is Pressed, but Wasn't Held down before.
  if (btn_set_state == 1 && !btn_set_down)
  {
    btn_set_down = 1;
    //We Switch to the Next state in here
    //If the Program state is SET_MINUTE, and we press the SET BTN,
    //We Go back to the Main Clock and Save the Time that was set in here.
    switch (program_state)
    {
      case SET_HOUR:
        program_state = SET_MINUTE;
        break;
      case SET_MINUTE:
        program_state = CLOCK;
        set_time(temp_time);
        lcd_blink_off(&theLCD);
        lcd_clear(&theLCD);
        break;
      case CLOCK:
        break;
    }
  }
  if (!btn_set_state)
  {
    btn_set_down = 0;
  }

  // If the Increment Button is Pressed, but Wasn't Held down before.
  if (btn_increment_state == 1 && !btn_increment_down)
  {
    btn_increment_down = 1;
    // We Increment the Specified part of the time, with respect to the program state.
    // if State is SET_HOUR, We increment the Hour part
    // if State is SET_MINUTE, We Increment the Minute part
    switch (program_state)
    {
    case SET_HOUR:
      temp_time.hour++;
      if (temp_time.hour > 23)
      {
        temp_time.hour = 0;
        lcd_clear_enable = 1;
      }
      break;
    case SET_MINUTE:
      temp_time.minute++;
      if (temp_time.minute > 59)
      {
        temp_time.minute = 0;
        lcd_clear_enable = 1;
      }
      break;
    case CLOCK:
      break;
    }
  }
  if (!btn_increment_state)
  {
    btn_increment_down = 0;
  }
}

void set_time(time_struct time)
{
  RTC_TimeTypeDef sTime = {0};
  sTime.Hours = time.hour;
  sTime.Minutes = time.minute;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }

  //We write Data to the Backup register of the rtc to indicate that we don't need to Adjust the time at boot.
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x2345);  // backup register
}

time_struct get_time(void)
{
  //We Read the time from RTC.
  //Note that it is very important that You read both the Time and The Date, even if the Date is not needed.
  //As Not Reading Either one of them, causes the RTC to Lock up.
  RTC_DateTypeDef gDate;
  RTC_TimeTypeDef gTime;

  /* Get the RTC current Time */
  HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN);
  /* Get the RTC current Date */
  HAL_RTC_GetDate(&hrtc, &gDate, RTC_FORMAT_BIN);

  time_struct x = {.hour = gTime.Hours, .minute = gTime.Minutes};
  return x;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
