#include "./super-flash.h"

#define PIN_LED 13
#define PIN_BOOT 16

#if SUPER_FLASH_ENABLED == 1
#include <entry.h>
#include <fpioa.h>
#include <gpiohs.h>
#include <sleep.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sysctl.h>

#include "./drivers/redirect-stdio-standalone.c"
#include "./drivers/w25qxx-standalone.c"

#include "./drivers/output-repeat.c"

#include "./drivers/step1-wait-hello.c"
#include "./drivers/step2-download.c"

static int super_flash_main_start0(void* ctx) {
  core0_step1_wait_hello();
  core0_step2_read_serial();
  exit(0);
  while (1)
    ;
}
static int super_flash_main_start1(void* ctx) {
  plic_init();
  sysctl_enable_irq();

  core1_step1_blink_led();
  core1_step2_write_flash();
  exit(0);
  while (1)
    ;
}

void io_mux_init(void)
{
  // fpioa_set_function(29, FUNC_SPI0_SCLK);
  // fpioa_set_function(30, FUNC_SPI0_D0);
  // fpioa_set_function(31, FUNC_SPI0_D1);
  // fpioa_set_function(32, FUNC_GPIOHS7);

  // fpioa_set_function(24, FUNC_SPI0_SS3);

  // fpioa_set_function(33, FUNC_I2S0_OUT_D0);
  // fpioa_set_function(35, FUNC_I2S0_SCLK);

  // fpioa_set_function(34, FUNC_I2S0_WS);

  fpioa_set_function(27, FUNC_SPI0_SCLK);
  fpioa_set_function(28, FUNC_SPI0_D0);
  fpioa_set_function(26, FUNC_SPI0_D1);
  fpioa_set_function(32, FUNC_GPIOHS7);
  fpioa_set_function(29, FUNC_SPI0_SS3);

}

int main() {
  sysctl_pll_set_freq(SYSCTL_PLL0, 800000000UL);
  // sysctl_pll_set_freq(SYSCTL_PLL1, 160000000UL);
  // sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);

  io_mux_init();
  uint64_t stime = sysctl_get_time_us();
  fpioa_init();
  fpioa_set_function(PIN_BOOT, FUNC_GPIOHS0 + PIN_BOOT);
  gpiohs_set_drive_mode(PIN_BOOT, GPIO_DM_INPUT_PULL_UP);

  while (sysctl_get_time_us() - stime < 1000 * 500) {
    if (gpiohs_get_pin(PIN_BOOT) == GPIO_PV_LOW) {
      dmac_init();
      plic_init();
      sysctl_enable_irq();

      redirect_stdio();
      fpioa_set_function(PIN_LED, FUNC_GPIOHS0 + PIN_LED);
      gpiohs_set_drive_mode(PIN_LED, GPIO_DM_OUTPUT);

      register_core1(super_flash_main_start1, NULL);
      super_flash_main_start0(NULL);
    }
  }

  gpiohs_set_drive_mode(PIN_BOOT, GPIO_DM_INPUT);
}

#elif SUPER_FLASH_ENABLED == 2
#include "./drivers/freertos.c"

#endif
