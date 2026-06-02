#include "app.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
	gh_app_init();

	for (;;) {
		gh_app_tick();
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}