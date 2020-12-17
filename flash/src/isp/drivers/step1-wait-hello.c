volatile uint8_t lock_step1 = 1;

#define JSON_TYPE_1 ("{\"type\":1,\"hello\":\"this is fast flash isp.\"}\n")

static void core0_step1_wait_hello() {
  char* hello = repeat_malloc(JSON_TYPE_1);
  while (lock_step1) {
    repeat_printf(hello, JSON_TYPE_1);
    msleep(10);
    if (gpiohs_get_pin(PIN_BOOT) == GPIO_PV_HIGH) {
      lock_step1 = 0;
    }
  }
}

static void core1_step1_blink_led() {
  while (lock_step1) {
    gpiohs_set_pin(PIN_LED, GPIO_PV_LOW);
    msleep(120);
    gpiohs_set_pin(PIN_LED, GPIO_PV_HIGH);
    msleep(600);
  }
  gpiohs_set_pin(PIN_LED, GPIO_PV_HIGH);
}
