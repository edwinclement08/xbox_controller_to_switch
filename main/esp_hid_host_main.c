/* This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this software is
   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "esp_hidh.h"
#include "esp_hid_gap.h"

static const char *TAG = "ESP_HIDH_DEMO";
static const char *DEVICE = "98:7a:14:ae:ec:f8"; // Put your device's Bluetooth device address here

void printout(const void *buffer, uint16_t buff_len) {
    static const char *tag = "PRINTOUT";

    if (buff_len != 15) {
        return;
    }

    char temp_buffer1[16 + 3]; //for not-byte-accessible memory
    char hex_buffer1[3 * 16 + 1];
    const char *p;
    int bytes_cur_line = 15;

    if (!esp_ptr_byte_accessible(buffer)) {
        //use memcpy to get around alignment issue
        memcpy(temp_buffer1, buffer, (bytes_cur_line + 3) / 4 * 4);
        p = temp_buffer1;
    } else {
        p = buffer;
    }

    char output[100];
    sprintf(output, "Lx:%3d %3d", p[0], p[1]);
    sprintf(output+10*1, "Ly:%3d %3d ", p[2], p[3]);
    sprintf(output+10*2, "Rx:%3d %3d ", p[4], p[5]);
    sprintf(output+10*3, "Ry:%3d %3d ", p[6], p[7]);
    sprintf(output+10*4, "Lt:%3d %03x ", p[8], p[9]);
    sprintf(output+10*5, "Rt:%3d %03x ", p[10], p[11]);

       

    for (int i = 0; i < bytes_cur_line; i ++) {
        if (i < 12) sprintf(hex_buffer1 + 3 * i, "%02x ", p[i]);
        else sprintf(hex_buffer1 + 3 * i, "%02x ", p[i]);
    }
    ESP_LOG_LEVEL(ESP_LOG_INFO, tag, "%s", output);
}

void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDH_OPEN_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->open.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " OPEN: %s", ESP_BD_ADDR_HEX(bda), esp_hidh_dev_name_get(param->open.dev));
        esp_hidh_dev_dump(param->open.dev, stdout);

        char hex_buffer_1[4 * 16 + 1];
        sprintf(hex_buffer_1, ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda) );
        if(strcmp( hex_buffer_1, DEVICE) == 0) {
            ESP_LOGI("DEBUG", "Device connected");
        }

        break;
    }
    case ESP_HIDH_BATTERY_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->battery.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " BATTERY: %d%%", ESP_BD_ADDR_HEX(bda), param->battery.level);
        break;
    }
    case ESP_HIDH_INPUT_EVENT: {
         const uint8_t *bda = esp_hidh_dev_bda_get(param->input.dev);
        // ESP_LOGI(TAG, ESP_BD_ADDR_STR " INPUT: %8s, MAP: %2u, ID: %3u, Len: %d, Data:", ESP_BD_ADDR_HEX(bda), esp_hid_usage_str(param->input.usage), param->input.map_index, param->input.report_id, param->input.length);
        //ESP_LOG_BUFFER_HEX("ORIGINAL", param->input.data, param->input.length);
        // if(strcmp( ESP_BD_ADDR_HEX(bda), DEVICE) == 0) {
            
        // }
        
          char hex_buffer_2[4 * 16 + 1];
        sprintf(hex_buffer_2, ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda) );
        if(strcmp( hex_buffer_2, DEVICE) == 0) {
           printout(param->input.data, param->input.length) ;
        }

        
        break;
    }
    case ESP_HIDH_FEATURE_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->feature.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " FEATURE: %8s, MAP: %2u, ID: %3u, Len: %d", ESP_BD_ADDR_HEX(bda), esp_hid_usage_str(param->feature.usage), param->feature.map_index, param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDH_CLOSE_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->close.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " CLOSE: '%s' %s", ESP_BD_ADDR_HEX(bda), esp_hidh_dev_name_get(param->close.dev), esp_hid_disconnect_reason_str(esp_hidh_dev_transport_get(param->close.dev), param->close.reason));
        //MUST call this function to free all allocated memory by this device
        esp_hidh_dev_free(param->close.dev);
        break;
    }
    default:
        ESP_LOGI(TAG, "EVENT: %d", event);
        break;
    }
}

#define SCAN_DURATION_SECONDS 5

void hid_demo_task(void *pvParameters)
{
    size_t results_len = 0;
    esp_hid_scan_result_t *results = NULL;
    ESP_LOGI(TAG, "SCAN...");
    //start scan for HID devices
    esp_hid_scan(SCAN_DURATION_SECONDS, &results_len, &results);
    ESP_LOGI(TAG, "SCAN: %u results", results_len);
    if (results_len) {
        esp_hid_scan_result_t *r = results;
        esp_hid_scan_result_t *cr = NULL;
        while (r) {
            printf("  %s: " ESP_BD_ADDR_STR ", ", (r->transport == ESP_HID_TRANSPORT_BLE) ? "BLE" : "BT ", ESP_BD_ADDR_HEX(r->bda));
            printf("RSSI: %d, ", r->rssi);
            printf("USAGE: %s, ", esp_hid_usage_str(r->usage));
            if (r->transport == ESP_HID_TRANSPORT_BLE) {
                cr = r;
                printf("APPEARANCE: 0x%04x, ", r->ble.appearance);
                printf("ADDR_TYPE: '%s', ", ble_addr_type_str(r->ble.addr_type));
            } else {
                cr = r;
                printf("COD: %s[", esp_hid_cod_major_str(r->bt.cod.major));
                esp_hid_cod_minor_print(r->bt.cod.minor, stdout);
                printf("] srv 0x%03x, ", r->bt.cod.service);
                print_uuid(&r->bt.uuid);
                printf(", ");
            }
            printf("NAME: %s ", r->name ? r->name : "");
            printf("\n");
            r = r->next;
        }
        if (cr) {
            //open the last result
            esp_hidh_dev_open(cr->bda, cr->transport, cr->ble.addr_type);
        }
        //free the results
        esp_hid_scan_results_free(results);
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    ESP_ERROR_CHECK( esp_hid_gap_init(ESP_BT_MODE_BTDM) );
    ESP_ERROR_CHECK( esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler) );
    esp_hidh_config_t config = {
        .callback = hidh_callback,
        .event_stack_size = 4096
    };
    ESP_ERROR_CHECK( esp_hidh_init(&config) );

    xTaskCreate(&hid_demo_task, "hid_task", 12 * 1024, NULL, 2, NULL);
}
