#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dv8_mqtt.h"
#include "led_strip.h"
#include "led_expressions.h"
#include "esp_timer.h"

#define LED_LIGHT_GPIO 11
#define LED_LIGHT_COUNT 120
#define LED_PANEL_GPIO 0
#define LED_PANEL_COUNT 98
#define LED_LIGHT_BRIGHTNESS 0.5
#define LED_PANEL_BRIGHTNESS 1

static const char *TAG = "esp_lights";

led_strip_handle_t led_panel;
led_strip_handle_t led_strip;

int current_rgb[3];

void set_led_lights() {
	for (int i = 0; i < LED_LIGHT_COUNT; i++) {
		if (i > 35 || i < 15) {
			if (led_light_rgb[0] != 0 || led_light_rgb[1] != 0 || led_light_rgb[2] != 0) {
				led_strip_set_pixel_rgbw(led_strip, i, led_light_rgb[1]*LED_LIGHT_BRIGHTNESS, led_light_rgb[0]*LED_LIGHT_BRIGHTNESS, led_light_rgb[2]*LED_LIGHT_BRIGHTNESS,0);
			} else {
				led_strip_set_pixel_rgbw(led_strip, i, current_rgb[1]*LED_LIGHT_BRIGHTNESS, current_rgb[0]*LED_LIGHT_BRIGHTNESS, current_rgb[2]*LED_LIGHT_BRIGHTNESS, 0);
			}
		} else {
			led_strip_set_pixel_rgbw(led_strip, i,0,0,0,0);
		}
	}
	led_strip_refresh(led_strip);
	vTaskDelay(pdMS_TO_TICKS(1000));
}

void clear_led_light() {
	for (int i = 0; i < LED_LIGHT_COUNT; i++) {
		led_strip_set_pixel_rgbw(led_strip, i,0,0,0,0);	
	}
	led_strip_refresh(led_strip);
	vTaskDelay(pdMS_TO_TICKS(1000));
}

void set_led_panel(const int light_val_arr[LED_PANEL_COUNT]) {
	for (int i = 0; i < LED_PANEL_COUNT; i++) {
		if (led_panel_rgb[0] != 0 || led_panel_rgb[1] != 0 || led_panel_rgb[2] != 0) {
			if (led_panel_face[i] == 1) {
				led_strip_set_pixel(led_panel,i,led_panel_rgb[0]*LED_PANEL_BRIGHTNESS,led_panel_rgb[1]*LED_PANEL_BRIGHTNESS,led_panel_rgb[2]*LED_PANEL_BRIGHTNESS);
			} else {
				led_strip_set_pixel(led_panel,i,0,0,0);
			}
		} else {
			if (light_val_arr[i] == 1) {
				led_strip_set_pixel(led_panel,i,current_rgb[0]*LED_PANEL_BRIGHTNESS,current_rgb[1]*LED_PANEL_BRIGHTNESS,current_rgb[2]*LED_PANEL_BRIGHTNESS);
			} else {
				led_strip_set_pixel(led_panel,i,0,0,0);
			}
		}
	}
	led_strip_refresh(led_panel);
}

void clear_led_panel() {
	for (int i = 0; i < LED_PANEL_COUNT; i++) {
		led_strip_set_pixel(led_panel,i,0,0,0);
	}
	led_strip_refresh(led_panel);
	vTaskDelay(pdMS_TO_TICKS(1000));
}

void set_rgb(int r, int g, int b) {
	current_rgb[0] = r;
	current_rgb[1] = g;
	current_rgb[2] = b;
}

void led_lights_task(void * pvParameters) {
	ESP_LOGI(TAG,"LED LIGHTS INITIALIZED");
	while (1) {
		if (e_stop == 1) {
			set_rgb(255,0,0); // Red
		} else if (battery_is_charging == 1) {
			clear_led_light();
			set_rgb(40,255,30); // Blinking Green
		} else if (direct_status == 1) {
			set_rgb(0,0,255); // Blue
		} else {
			switch (robot_mode) {
				case 0:// Clear
					set_rgb(0,0,0);
					break;
				case 7: // ERROR MODE - Yellow
					set_rgb(255,120,0);
					break;
				case 3: // LITTER PICKING - Blinking Blue
					clear_led_light();
					set_rgb(0,0,255);
					break;
				case 4: // TRANSIENT
				case 1: // IDLE
				case 2: // COVERAGE MODE
				case 6: // IDLE LATCH - Green
					set_rgb(40,255,30);
					break;
			}
		}
		set_led_lights();
	}
}

void led_panel_task(void * pvParameters) {
	uint64_t end_time = esp_timer_get_time();
	int face = 0; //0 -> Happy, 1 -> Idle, 2 -> look left, 3 -> look right, 4 -> blink
	ESP_LOGI(TAG,"LED PANEL INITIALIZED");	
	while (1) {
		uint64_t current_time = esp_timer_get_time();
		if (end_time<current_time) {
			int rand_val = rand()%4;
			if ((face == 2 && rand_val == 3) || (face == 3 && rand_val == 2)) {
				rand_val = 1;
			}
			face = rand_val;
			end_time = current_time + rand()%10000000 + 10000000;
		}
		if (angular_z>0.1 || angular_z<-0.1) {
			if (angular_z>0) {
				set_led_panel(very_right_face);
			} else {
				set_led_panel(very_left_face);
			}
		} else if (angular_z>0.01 || angular_z<-0.01) {
			if (angular_z>0) {
				set_led_panel(right_face);
			} else {
				set_led_panel(left_face);
			}
		} else {
			switch (robot_mode) {
				case 0:// Clear
					set_led_panel(empty_face);
					break;
				case 7: // ERROR MODE
					set_led_panel(dead_face);
					break;
				case 3: // LITTER PICKING
					set_led_panel(angry_face);
					break;
				case 4: // TRANSIENT
				case 2: // COVERAGE MODE
				case 1: // IDLE
				case 6: // IDLE LATCH
					if (face == 0) {
						set_led_panel(happy_face);
					} else if (face == 1) {
						set_led_panel(idle_face);
					} else if (face == 2) {
						set_led_panel(left_face);
					} else if (face == 3) {
						set_led_panel(right_face);
					} else {
						set_led_panel(empty_face);
					}
					break;
			}
		}
	}
}

void app_main(void)
{	
	// Initiallize MQTT
	mqtt_app_start();
	ESP_LOGI(TAG,"MQTT INITIALIZED");

	// Setup LED Lights
	led_strip_rmt_config_t rmt_config_1 = {
		.clk_src = RMT_CLK_SRC_DEFAULT,
		.resolution_hz = 10 * 1000 * 1000,
		.mem_block_symbols = 48,
		.flags = {
			.with_dma = false,
		}
	}; 
	led_strip_config_t strip_config = {
		.strip_gpio_num = LED_LIGHT_GPIO,
		.max_leds = LED_LIGHT_COUNT,       
		.led_model = LED_MODEL_SK6812,
		.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGBW
	};
	ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config_1, &led_strip));

	// Setup LED Panel
	led_strip_rmt_config_t rmt_config_2 = {
		.clk_src = RMT_CLK_SRC_DEFAULT,
		.resolution_hz = 10 * 1000 * 1000,
		.mem_block_symbols = 48,
		.flags = {
			.with_dma = false,
		}
	};
	led_strip_config_t panel_config = {
		.strip_gpio_num = LED_PANEL_GPIO,
		.max_leds = LED_PANEL_COUNT,
		.led_model = LED_MODEL_WS2812,
		.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
		.flags = {
			.invert_out = false,
		}
	};
	ESP_ERROR_CHECK(led_strip_new_rmt_device(&panel_config, &rmt_config_2, &led_panel));

	// Create Tasks
	xTaskCreate(led_lights_task,"LED_Task",4096,NULL,5,NULL);
	xTaskCreate(led_panel_task, "LED_Panel",4096,NULL,5,NULL);

	// Main Loop
	while (1) {
		ESP_LOGI(TAG,"Free heap: %d", esp_get_free_heap_size());
		ESP_LOGI(TAG,"linear_x: %f, angular_z: %f",linear_x,angular_z);
		ESP_LOGI(TAG,"battery_percentage: %f",battery_percentage);
		ESP_LOGI(TAG,"battery_is_charging: %d",battery_is_charging);
		ESP_LOGI(TAG,"e_stop: %d",e_stop);
		ESP_LOGI(TAG,"handbrake: %d",handbrake);
		ESP_LOGI(TAG,"direct_status: %d",direct_status);
		ESP_LOGI(TAG,"safety_mode: %d",safety_mode);
		ESP_LOGI(TAG,"robot_mode: %d",robot_mode);
		ESP_LOGI(TAG,"brush_speed: %d",brush_speed);
		ESP_LOGI(TAG,"current_rgb: %d %d %d",current_rgb[0],current_rgb[1],current_rgb[2]);		

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
