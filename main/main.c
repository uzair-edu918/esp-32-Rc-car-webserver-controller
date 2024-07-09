#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
// #include <driver/temperature_sensor.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_tls.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_check.h"
#include "esp_err.h"
#include "driver/ledc.h"



//For Moter Driver Control Direction (CLock / Anticlock)
#define PIN_5_19_INIT_1 19 //19->1 .. 21->0 = forward
#define PIN_5_21_INIT_2 21 //19->0 .. 21->1 = backward


#define PIN_18_25_INIT_1 25
#define PIN_18_26_INIT_2 26




// LEDC PWM CODE
#define PIN_MOTER_1 5
#define PIN_MOTER_2 18
#define PWM_RES     LEDC_TIMER_13_BIT
int duty_cycle_0 = 7200;
int duty_cycle_1 = 7200;


int duty_cycle_stop_0 = 0;
int duty_cycle_stop_1 = 0;


// LEDC PWM CODE

#define LED_PIN 2

#define ip_protocol 0
#define MAX_FAILURES 10
#define WIFI_SUCCESS 1<<0
#define TCP_SUCCESS 1<<0
#define TCP_FAILURE 1<<1
#define WIFI_FAILURE 1<<1

static EventGroupHandle_t wifi_event_group;
static int s_retry_num = 0;
static const char* TAG = "WIFI";


static void wifi_event_handler(void * arg , esp_event_base_t event_base , int32_t event_id ,void * event_data){

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI( TAG,"connecting to wifi");
        esp_wifi_connect();
    }else if(event_base==WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if(s_retry_num < MAX_FAILURES){
            ESP_LOGI( TAG,"reconecting to WIFI");
            esp_wifi_connect();
            s_retry_num++;
        }else{

            xEventGroupSetBits(wifi_event_group , WIFI_FAILURE);
        }
    }

}


static void ip_event_handler(void * arg , esp_event_base_t event_base , int32_t event_id ,void * event_data){

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI( TAG,"got IP adress");
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG , "STA IP : " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group,WIFI_SUCCESS);
    }

}


int connect_to_wifi(){


    int status  = WIFI_FAILURE;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_event_group = xEventGroupCreate();
    // esp_event_handler_instance_t instance_any_id;
    // esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_t wifi_handler_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&wifi_event_handler,NULL,&wifi_handler_event_instance));

    esp_event_handler_instance_t got_ip_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,ESP_EVENT_ANY_ID,&ip_event_handler,NULL,&got_ip_event_instance));

    wifi_config_t wifi_config = {
        .sta ={
            .ssid = "{yourssid}",
            .password="{yourpassword}",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,

            }
        
        },
    };


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG , "STA Initialization Completed");
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_SUCCESS | WIFI_FAILURE, pdFALSE , pdFALSE ,portMAX_DELAY);



    if(bits && WIFI_SUCCESS){
        ESP_LOGI(TAG, "Connect To Wifi");
        status= WIFI_SUCCESS;
    }else if(bits & WIFI_FAILURE){  

        ESP_LOGI(TAG, "Failed to connect to AP");
        status = WIFI_FAILURE;
    }else{  
        ESP_LOGI(TAG, "Unknown Event");
        status = WIFI_FAILURE;
    }



    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,IP_EVENT_STA_GOT_IP,wifi_handler_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,ESP_EVENT_ANY_ID,got_ip_event_instance));
    vEventGroupDelete(wifi_event_group);

    return status;

}





// ======================WEBSERVER CODE ===================================


static esp_err_t forward_handler(httpd_req_t *req)
{
      httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        //forward
    	gpio_set_level(LED_PIN,1);
    	gpio_set_level(PIN_5_19_INIT_1,1);
    	gpio_set_level(PIN_5_21_INIT_2,0);

        gpio_set_level(PIN_18_25_INIT_1,1);
    	gpio_set_level(PIN_18_26_INIT_2,0);

        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle_0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty_cycle_1);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

    const char *resp_str = "f";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}
static esp_err_t forward_option_handler(httpd_req_t *req)
{
      httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        //forward
    	gpio_set_level(LED_PIN,1);
    	gpio_set_level(PIN_5_19_INIT_1,1);
    	gpio_set_level(PIN_5_21_INIT_2,0);

        gpio_set_level(PIN_18_25_INIT_1,1);
    	gpio_set_level(PIN_18_26_INIT_2,0);

        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle_0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty_cycle_1);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}
httpd_uri_t forward_options = {
    .uri      = "/forward",
    .method   = HTTP_OPTIONS,
    .handler  = forward_option_handler,
    .user_ctx = NULL
};

static esp_err_t stop_handler(httpd_req_t *req)
{
      httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    gpio_set_level(LED_PIN,0);
                    gpio_set_level(PIN_18_25_INIT_1,0);
    	gpio_set_level(PIN_18_26_INIT_2,0);
        	gpio_set_level(PIN_5_19_INIT_1,0);
    	gpio_set_level(PIN_5_21_INIT_2,0);


            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle_stop_0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty_cycle_stop_1);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    const char *resp_str = "s";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}


static esp_err_t right_handler(httpd_req_t *req)
{
        //right
    gpio_set_level(LED_PIN,1);
    gpio_set_level(PIN_5_19_INIT_1,1);
    gpio_set_level(PIN_5_21_INIT_2,0);

    gpio_set_level(PIN_18_25_INIT_1,1);
    gpio_set_level(PIN_18_26_INIT_2,0);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle_0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

    const char *resp_str = "r";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}
static esp_err_t left_handler(httpd_req_t *req)
{
        //right
    gpio_set_level(LED_PIN,1);
    gpio_set_level(PIN_5_19_INIT_1,1);
    gpio_set_level(PIN_5_21_INIT_2,0);

    gpio_set_level(PIN_18_25_INIT_1,1);
    gpio_set_level(PIN_18_26_INIT_2,0);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty_cycle_1);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

    const char *resp_str = "l";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}





static esp_err_t down_handler(httpd_req_t *req)
{
        //forward
    	gpio_set_level(LED_PIN,1);
    	gpio_set_level(PIN_5_19_INIT_1,0);
    	gpio_set_level(PIN_5_21_INIT_2,1);

        gpio_set_level(PIN_18_25_INIT_1,0);
    	gpio_set_level(PIN_18_26_INIT_2,1);

        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle_0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty_cycle_1);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

         const char *resp_str = "d";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}




static const httpd_uri_t forward = {
    .uri       = "/forward",
    .method    = HTTP_POST,
    .handler   = forward_handler,
    .user_ctx = NULL 
};


static const httpd_uri_t stop = {
    .uri       = "/stop",
    .method    = HTTP_GET,
    .handler   = stop_handler,
    .user_ctx = NULL    
    
};

static const httpd_uri_t right = {
    .uri       = "/right",
    .method    = HTTP_GET,
    .handler   = right_handler,
    .user_ctx = NULL
    
    
};
static const httpd_uri_t left = {
    .uri       = "/left",
    .method    = HTTP_GET,
    .handler   = left_handler,
    .user_ctx = NULL
    
    
};


static const httpd_uri_t down = {
    .uri       = "/down",
    .method    = HTTP_GET,
    .handler   = down_handler,
    .user_ctx = NULL
    
    
};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
   
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.task_priority = 5;  // Increase priority as needed
#if CONFIG_IDF_TARGET_LINUX
    // Setting port as 8001 when building for Linux. Port 80 can be used only by a privileged user in linux.
    // So when a unprivileged user tries to run the application, it throws bind error and the server is not started.
    // Port 8001 can be used by an unprivileged user as well. So the application will not throw bind error and the
    // server will be started.
    config.server_port = 8001;
#endif // !CONFIG_IDF_TARGET_LINUX
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &forward);
        httpd_register_uri_handler(server, &stop);
        httpd_register_uri_handler(server, &right);
        httpd_register_uri_handler(server, &left);
        httpd_register_uri_handler(server, &down);
        httpd_register_uri_handler(server, &forward_options);



        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}


static void connect_handler()
{        static httpd_handle_t server = NULL;
    ESP_LOGI(TAG, "Just Before Tcp Client");

    if (server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        start_webserver();
    }
}





// ======================WEBSERVER CODE ===================================



// =======================LEDC PWM CODE =====================================



void moters_init(void)
{

    // TIMER CONFIGURATIONS

    ledc_timer_config_t timer_config = {

        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = (4000),
        .clk_cfg = LEDC_AUTO_CLK,

    };

    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));


    // LEDC PWM channel configuration
   // Configure PWM channel 0
    ledc_channel_config_t ledc_channel_0 = {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_MOTER_1,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel_0);





       // Configure PWM channel 1
    ledc_channel_config_t ledc_channel_1 = {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_MOTER_2,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel_1);


}








// =======================LEDC PWM CODE =====================================

void app_main(void)
{


    gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN , GPIO_MODE_OUTPUT);


    gpio_reset_pin(PIN_5_19_INIT_1);
	gpio_set_direction(PIN_5_19_INIT_1 , GPIO_MODE_OUTPUT);

    gpio_reset_pin(PIN_5_21_INIT_2);
	gpio_set_direction(PIN_5_21_INIT_2 , GPIO_MODE_OUTPUT);


    gpio_reset_pin(PIN_18_25_INIT_1);
	gpio_set_direction(PIN_18_25_INIT_1 , GPIO_MODE_OUTPUT);

    gpio_reset_pin(PIN_18_26_INIT_2);
	gpio_set_direction(PIN_18_26_INIT_2 , GPIO_MODE_OUTPUT);



        static httpd_handle_t server = NULL;



    moters_init();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    connect_to_wifi();

    // ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    // if(WIFI_SUCCESS!= status){
    //     ESP_LOGI( TAG,"Something Went wring");
    //     return;
    // }
    // connect_tcp_server();
    ESP_LOGI(TAG, "Just Before Tcp Client");

    // ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    connect_handler();
    // tcp_client();

    // while (1)
    // {
    //     vTaskDelay(2000);
    // }
    

}
