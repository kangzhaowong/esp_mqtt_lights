#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include <stdatomic.h>
#include <cJSON.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "dv8_mqtt.h"

static const char *_TAG = "mqtt_esp";

_Atomic(float) linear_x;
_Atomic(float) angular_z;
_Atomic(float) battery_percentage;
_Atomic(int) brush_speed;
_Atomic(int) battery_is_charging;
_Atomic(int) e_stop;
_Atomic(int) handbrake;
_Atomic(int) direct_status;
_Atomic(int) robot_mode;
_Atomic(int) safety_mode;

int led_light_rgb[3];
int led_panel_rgb[3];
int led_panel_face[98];

static void save_value_float(cJSON *json, _Atomic(float) *parameter, char *parameter_name)
{
	cJSON *temp = cJSON_GetObjectItem(json,parameter_name);
	if (temp) {
		atomic_store(parameter,cJSON_GetNumberValue(temp));
		ESP_LOGI(_TAG,"RECIEVE: %s: %f",parameter_name,atomic_load(parameter));
	}
}

static void save_value_int(cJSON *json, _Atomic(int) *parameter, char *parameter_name)                                                              {
	cJSON *temp = cJSON_GetObjectItem(json,parameter_name);
	if (temp) {
		atomic_store(parameter,cJSON_GetNumberValue(temp));
		ESP_LOGI(_TAG,"RECIEVE: %s: %d",parameter_name,atomic_load(parameter));
	}
}

void parse_int_array_inplace(int* parameter, char* input, size_t len) {
	size_t index = 0;
	char* end = input + len;
	char* start = input;

	while (start < end) {
		char* comma = memchr(start, ',', end - start);
		if (comma) *comma = '\0';
		parameter[index++] = atoi(start);
		if (!comma) break;
		start = comma + 1;
	}
}

static void save_value_int_arr(cJSON *json,int *parameter, char *parameter_name) {
        cJSON *temp = cJSON_GetObjectItem(json,parameter_name);
        if (temp) {
                size_t input_len = strlen(cJSON_GetStringValue(temp));
                parse_int_array_inplace(parameter,cJSON_GetStringValue(temp),input_len);
        }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;
	switch ((esp_mqtt_event_id_t)event_id) {
		case MQTT_EVENT_CONNECTED:
			esp_mqtt_client_subscribe(client, "/robot/control/cmd_vel/linear_x", 0);
			esp_mqtt_client_subscribe(client, "/robot/control/cmd_vel/angular_z", 0);
			esp_mqtt_client_subscribe(client, "/robot/state/battery_percentage", 0);
			esp_mqtt_client_subscribe(client, "/robot/state/battery_is_charging", 0);
			esp_mqtt_client_subscribe(client, "/robot/state/e_stop", 0);
			esp_mqtt_client_subscribe(client, "/robot/state/handbrake", 0);
			esp_mqtt_client_subscribe(client, "/robot/state/direct_status", 0);
			esp_mqtt_client_subscribe(client, "/robot/state/robot_mode", 0);
			esp_mqtt_client_subscribe(client, "/robot/control/brush_speed", 0);
			esp_mqtt_client_subscribe(client, "/robot/state/safety_mode", 0);
			esp_mqtt_client_subscribe(client, "/debug/led_panel",0);
			esp_mqtt_client_subscribe(client, "/debug/led_light",0);
			ESP_LOGI(_TAG, "MQTT_EVENT_CONNECTED");
			break;
		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(_TAG, "MQTT_EVENT_DISCONNECTED");
			break;
		case MQTT_EVENT_DATA: 
			{
				int topic_len = event->topic_len;
				char topic[topic_len + 1];
				int data_len = event->data_len;
				char data[data_len + 1];

				if (topic_len == 0 || data_len == 0) {
					break;
				}

				strncpy(topic, event->topic, topic_len);
				topic[topic_len] = '\0';
				strncpy(data, event->data, data_len);
				data[data_len] = '\0';	

				ESP_LOGI(_TAG,"ON MQTT TOPIC: %s", topic);

				cJSON *json = cJSON_Parse(data);

				if (json == NULL) {
					ESP_LOGI(_TAG,"RECIEVE ERROR DATA: %s", data);
					cJSON_Delete(json);
					break;		    
				}

				if (strcmp(topic,"/robot/control/cmd_vel/linear_x") == 0) {
					save_value_float(json,&linear_x,"linear_x");
				} else if  (strcmp(topic,"/robot/control/cmd_vel/angular_z") == 0) {
					save_value_float(json,&angular_z,"angular_z");
				} else if (strcmp(topic,"/robot/state/battery_percentage") == 0) {
					save_value_float(json,&battery_percentage,"battery_percentage");    
				} else if (strcmp(topic,"/robot/state/battery_is_charging") == 0) {
					save_value_int(json,&battery_is_charging,"battery_is_charging");
				} else if (strcmp(topic,"/robot/state/e_stop") == 0) {
					save_value_int(json,&e_stop,"e_stop");    
				} else if (strcmp(topic,"/robot/state/handbrake") == 0) {
					save_value_int(json,&handbrake,"handbrake");
				} else if (strcmp(topic,"/robot/state/direct_status") == 0) {
					save_value_int(json,&direct_status,"direct_status");
				} else if (strcmp(topic,"/robot/state/robot_mode") == 0) {
					save_value_int(json,&robot_mode,"robot_mode");
				} else if (strcmp(topic,"/robot/control/brush_speed") == 0) {
					save_value_int(json,&brush_speed,"brush_speed");
				} else if (strcmp(topic,"/robot/state/safety_mode") == 0) {
					save_value_int(json,&safety_mode,"safety_mode");
				} else if (strcmp(topic,"/debug/led_panel") == 0) {
					save_value_int_arr(json,led_panel_rgb,"rgb");
					save_value_int_arr(json,led_panel_face,"face");
				} else if (strcmp(topic,"/debug/led_light") == 0) {
					save_value_int_arr(json,led_light_rgb,"rgb");
				}
				cJSON_Delete(json);
				break;
			}
		case MQTT_EVENT_ERROR:
			ESP_LOGI(_TAG, "MQTT_EVENT_ERROR");
			break;
		default:
			break;
	}
}

void mqtt_app_start(void)
{
	ESP_LOGI(_TAG, "[APP] Startup..");
	ESP_LOGI(_TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
	ESP_LOGI(_TAG, "[APP] IDF version: %s", esp_get_idf_version());

	esp_log_level_set("*", ESP_LOG_VERBOSE);
	esp_log_level_set("wifi_init", ESP_LOG_ERROR);
	esp_log_level_set("wifi", ESP_LOG_ERROR);
	esp_log_level_set("transport_base", ESP_LOG_ERROR);
	esp_log_level_set("esp-tls", ESP_LOG_ERROR);
	esp_log_level_set("transport", ESP_LOG_ERROR);
	esp_log_level_set("outbox", ESP_LOG_ERROR);
	esp_log_level_set("mqtt_esp", ESP_LOG_ERROR);

	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_err_t err;
	do {
		err = example_connect();
		if (err != ESP_OK) {
			ESP_LOGW("WIFI", "Failed to connect to Wi-Fi: %s. Retrying in 5 seconds...", esp_err_to_name(err));
			vTaskDelay(pdMS_TO_TICKS(5000));
		}
	} while (err != ESP_OK);


	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = CONFIG_BROKER_URL,
	};

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
	esp_mqtt_client_start(client);
}
