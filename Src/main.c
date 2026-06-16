#include "stm32f4xx.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_utils.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_gpio.h"

#include "FreeRTOS.h"
#include "task.h"

void SystemClock_Config(void)
{
  /* Enable HSE (High Speed External Clock) */
  LL_RCC_HSE_Enable();
  while(LL_RCC_HSE_IsReady() != 1) { }

  /* Set FLASH latency */
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_5) {
    while(1);
  }

  /* Enable power clock and set voltage scaling to scale 1 (for max frequency) */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

  /* Configure PLL: HSE(8MHz) / 8 * 360 / 2 = 180MHz */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 360, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_Enable();
  while(LL_RCC_PLL_IsReady() != 1) { }

  /* Activate OverDrive to reach 180MHz */
  LL_PWR_EnableOverDriveMode();
  while(LL_PWR_IsActiveFlag_OD() != 1) { }
  LL_PWR_EnableOverDriveSwitching();
  while(LL_PWR_IsActiveFlag_ODSW() != 1) { }

  /* Set AHB/APB prescalers */
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);

  /* Switch system clock to PLL */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) { }

  /* Update SystemCoreClock variable */
  LL_SetSystemCoreClock(180000000);
}

void GPIO_Init(void)
{
  /* Enable GPIOA clock */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

  /* Configure PA5 as Output (LED on Nucleo F446RE) */
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_5, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_5, LL_GPIO_SPEED_FREQ_LOW);
  LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_5, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_5, LL_GPIO_PULL_NO);
}

static void blink_task(void *args)
{
	(void)args;
	while (1) {
		LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_5);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

int main(void)
{
	/* Configure the system clock to 180 MHz */
	SystemClock_Config();

	/* Initialize GPIO */
	GPIO_Init();

	/* Create blink task */
	xTaskCreate(blink_task, "Blink", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

	/* Start FreeRTOS scheduler */
	vTaskStartScheduler();

	while (1) {
        /* Should never reach here */
	}
	return 0;
}
