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

#include "arm_math.h"
#include "core_cm4.h"

/**
 * @brief  Initialize ITM/SWO for live variable watching via Cortex-Debug
 * @note   cpuFreq must match SystemCoreClock (180 MHz), swoFreq must match launch.json (2 MHz)
 */
void ITM_Init(uint32_t cpuFreq, uint32_t swoFreq)
{
    /* Enable Debug Exception and Monitor Control Register (DEMCR) */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* Enable DWT cycle counter (required for SWO timing) */
    DWT->CYCCNT = 0;
    DWT->CTRL   |= DWT_CTRL_CYCCNTENA_Msk;

    /* Configure SWO baud rate: SWO_freq = cpu_freq / (prescaler + 1) */
    TPIU->ACPR = (cpuFreq / swoFreq) - 1;

    /* Set SWO to use NRZ (UART) mode */
    TPIU->SPPR = 2;

    /* Disable Formatter (not needed for single-wire SWO) */
    TPIU->FFCR = 0;

    /* Unlock ITM and enable it */
    ITM->LAR  = 0xC5ACCE55;
    ITM->TCR  = ITM_TCR_TRACEBUSID_Msk |
                ITM_TCR_SYNCENA_Msk    |
                ITM_TCR_TSENA_Msk      |
                ITM_TCR_ITMENA_Msk;

    /* Enable ITM stimulus port 0 (used for console output) */
    ITM->TER = 0x1;
}

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

static void dsp_test_task(void *args)
{
    (void)args;

    /* CMSIS-DSP Test Data */
    float32_t a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float32_t b[4] = {2.0f, 2.0f, 2.0f, 2.0f};
    float32_t result[4];

    while (1) {
        /* Test Vector Multiplication (utilizes FPU/SIMD) */
        arm_mult_f32(a, b, result, 4);

        /* Test Fast Math Sine (utilizes FPU) */
        float32_t sin_val = arm_sin_f32(PI/4);
        (void)sin_val; // suppress unused warning

        LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_5);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void)
{
	/* Configure the system clock to 180 MHz */
	SystemClock_Config();

	/* Initialize ITM/SWO for Cortex-Debug live watch (cpu=180MHz, swo=2MHz) */
	ITM_Init(180000000, 2000000);

	/* Initialize GPIO */
	GPIO_Init();

	/* Create DSP test task */
	xTaskCreate(dsp_test_task, "DSP_Test", configMINIMAL_STACK_SIZE + 128, NULL, 2, NULL);

	/* Start FreeRTOS scheduler */
	vTaskStartScheduler();

	while (1) {
        /* Should never reach here */
	}
	return 0;
}
