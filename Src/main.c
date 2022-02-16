/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
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

#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "printf.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "stm32_adafruit_lcd.h"
#include "string.h"
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
void Tetris_StartGame(void);
void Tetris_Loop(void);
bool Tetris_RotatePiece(bool cw);
bool Tetris_Move(int8_t dx, int8_t dy);
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
char usb_tx_buf[256] = {0};

char usb_rx_buf[64];

bool GPIO_pressed(GPIO_TypeDef *GPIOx, uint16_t pin, int i, bool das) {
    static bool button_active[8] = {false};
    static bool button_das[8] = {false};
    static uint32_t last_active[8] = {0};
    static uint32_t last_das[8] = {0};

    uint32_t now = HAL_GetTick();
    if (HAL_GPIO_ReadPin(GPIOx, pin) == 0) {
        uint32_t last = last_active[i];
        bool active = button_active[i];
        button_active[i] = true;
        last_active[i] = now;

        if (now - last > 50 && !active) {
            button_das[i] = false;
            last_das[i] = now;
            return true;
        }

        if (!button_das[i] && now - last_das[i] > 300) {
            button_das[i] = true;
            last_das[i] = now;
            return true;
        }
        if (button_das[i] && now - last_das[i] > 50) {
            last_das[i] = now;
            return das;
        }

    } else {
        button_active[i] = false;
        button_das[i] = false;
    }
    return false;
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
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
    MX_USB_DEVICE_Init();
/* USER CODE BEGIN 2 */
#ifdef DEBUG
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, (uint8_t *)usb_rx_buf);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    memset(usb_rx_buf, 0, sizeof(usb_rx_buf));
#endif
    PRINTF("start\n");
    srand48(HAL_GetTick());
    BSP_LCD_Init();
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    Tetris_StartGame();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    uint32_t last_active = HAL_GetTick();
    while (1) {
        uint32_t now = HAL_GetTick();

        if (GPIO_pressed(GPIOC, GPIO_PIN_14, 6, true)) {
            PRINTF("C14\n");
            Tetris_RotatePiece(true);
        }
        if (GPIO_pressed(GPIOC, GPIO_PIN_15, 7, true)) {
            PRINTF("C15\n");
            Tetris_RotatePiece(false);
        }
        if (GPIO_pressed(GPIOA, GPIO_PIN_0, 0, false)) {
            PRINTF("A0\n");
            while (Tetris_Move(0, 1))
                ;
        }
        if (GPIO_pressed(GPIOA, GPIO_PIN_1, 1, false)) {
            PRINTF("A1\n");
            Tetris_StartGame();
        }
        if (GPIO_pressed(GPIOA, GPIO_PIN_2, 2, true)) {
            PRINTF("A2\n");
            Tetris_Move(1, 0);
        }
        if (GPIO_pressed(GPIOA, GPIO_PIN_3, 3, false)) {
            PRINTF("A3\n");
            Tetris_RotatePiece(true);
            Tetris_RotatePiece(true);
        }
        if (GPIO_pressed(GPIOA, GPIO_PIN_4, 4, true)) {
            PRINTF("A4\n");
            Tetris_Move(-1, 0);
        }
        if (GPIO_pressed(GPIOA, GPIO_PIN_5, 5, true)) {
            PRINTF("A5\n");
            Tetris_Move(0, 1);
        }

#ifdef DEBUG
        if (usb_rx_buf[0] != 0) {
            char key = usb_rx_buf[0];
            usb_rx_buf[0] = 0;
            PRINTF("r: %c\n", key);
            switch (key) {
                case 'z':
                case '/':
                    Tetris_RotatePiece(false);
                    break;
                // case 'w':
                case 'x':
                case 'A':
                    Tetris_RotatePiece(true);
                    break;
                // case 'a':
                case 'D':
                    Tetris_Move(-1, 0);
                    break;
                // case 's':
                case 'B':
                    Tetris_Move(0, 1);
                    break;
                // case 'd':
                case 'C':
                    Tetris_Move(1, 0);
                    break;
            }
        }
#endif

        Tetris_Loop();
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

    /*Configure GPIO pin : PC14, PC15 */
    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pin : PA0, PA1, PA2, PA3, PA4, PA5 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pin : PB2 */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
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
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
