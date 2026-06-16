#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "FreeRTOS.h"
#include "task.h"

static void clock_setup(void)
{
	/* Configure clock to 180 MHz using external 8MHz crystal (e.g. Nucleo board) */
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_180MHZ]);
	
	/* Enable GPIOA clock for LED */
	rcc_periph_clock_enable(RCC_GPIOA);
}

static void gpio_setup(void)
{
	/* Setup GPIOA Pin 5 as output (User LED on Nucleo F446RE) */
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
}

static void blink_task(void *args)
{
	(void)args;
	while (1) {
		gpio_toggle(GPIOA, GPIO5);
		vTaskDelay(pdMS_TO_TICKS(500)); /* Block for 500ms */
	}
}

int main(void)
{
	clock_setup();
	gpio_setup();

	/* Create a simple blink task */
	xTaskCreate(blink_task, "Blink", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();

	/* Should never reach here if scheduler started successfully */
	while (1) {
		__asm__("nop");
	}
	return 0;
}
