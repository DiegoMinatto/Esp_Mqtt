#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "mqtt_client.h"


#include <stdlib.h>
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "esp_wifi.h"


#include <stdbool.h>

#include <dht.h>

#include <sys/param.h>


static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
#if defined(CONFIG_IDF_TARGET_ESP8266)
static const gpio_num_t dht_gpio = 5;
#else
static const gpio_num_t dht_gpio = 17;
#endif

/* Definições e Constantes */
#define TRUE  1
#define FALSE 0
#define DEBUG TRUE 
#define LED_BUILDING	GPIO_NUM_2 
#define BUTTON			GPIO_NUM_4 
#define LED_EXTERNA     GPIO_NUM_5
#define GPIO_INPUT_PIN_SEL  	(1ULL<<BUTTON)
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY
#define PORT CONFIG_EXAMPLE_PORT
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define WIFI_CONNECTING_BIT BIT2
#define BUTTON_BIT BIT0
#define READY_BIT  BIT0


static EventGroupHandle_t event_group;
static EventGroupHandle_t s_wifi_event_group;
static EventGroupHandle_t button_event_group;
static int s_retry_num = 0;
static bool btnApertado = false;
esp_mqtt_client_handle_t client;

static const char *TAG = "MQTTS_EXAMPLE";

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t iot_eclipse_org_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t iot_eclipse_org_pem_start[]   asm("_binary_iot_eclipse_org_pem_start");
#endif
extern const uint8_t iot_eclipse_org_pem_end[]   asm("_binary_iot_eclipse_org_pem_end");

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
		    xEventGroupSetBits(event_group, READY_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
		    xEventGroupClearBits(event_group, READY_BIT);
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
		case MQTT_EVENT_SUBSCRIBED:
		    break;
		case MQTT_EVENT_DATA:
		    break;
	    case MQTT_EVENT_UNSUBSCRIBED:
		    break;
        case MQTT_EVENT_ERROR:
		    xEventGroupClearBits(event_group, READY_BIT);
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}


void sensor_task(void *pvParamters)
{ int msg_id;
  int16_t temperature = 0;
    int16_t humidity = 0;
	char integer_string[4];
    while (true)
    {
        xEventGroupWaitBits(event_group, READY_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
		if (dht_read_data(sensor_type, dht_gpio, &humidity, &temperature) == ESP_OK){
		  sprintf(integer_string, "%d", (temperature / 10));		
		  msg_id = esp_mqtt_client_publish(client, "/topic/qos0", integer_string, 0, 0, 0);
        }else{
			//xQueueSend(buffer, "0", pdMS_TO_TICKS(0));
			ESP_LOGE(TAG, "Erro ao ler o sensor!");
		}
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URI,
        .event_handle = mqtt_event_handler,
        .cert_pem = (const char *)iot_eclipse_org_pem_start,
		.username = "cliente",
		.password = "cl13nt3.1",
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}


void task_GPIO_Blink( void *pvParameter )
{
	 /*  Parâmetros de controle da GPIO da função "gpio_set_direction"
		GPIO_MODE_OUTPUT       			//Saída
		GPIO_MODE_INPUT        			//Entrada
		GPIO_MODE_INPUT_OUTPUT 			//Dreno Aberto
    */
	bool inverte = true;
	int delay = 100;
	gpio_set_direction( LED_BUILDING, GPIO_MODE_OUTPUT );
	gpio_set_level( LED_BUILDING, 1 );  //O Led desliga em nível 1;
    bool estado = 0;   

    while ( TRUE ) 
    {	

        //xEventGroupWaitBits(button_event_group, BUTTON_BIT, pdFALSE, pdFALSE, portMAX_DELAY);	
      EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                             WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_CONNECTING_BIT,
                                             pdFALSE,
                                             pdFALSE,
                                             portMAX_DELAY);
										
      /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
       * happened. */
      if (bits & WIFI_CONNECTED_BIT) {
        inverte = false;
		estado = false;
		delay = 100;		 
      } else if (bits & WIFI_FAIL_BIT) {
        inverte = true;
		delay = 100;
      }else if(bits & WIFI_CONNECTING_BIT){
		  inverte = true;
		  delay = 500;
	  }
	  if(inverte){
        estado = !estado;
	  }
      gpio_set_level( LED_BUILDING, estado ); 				
      vTaskDelay( delay / portTICK_RATE_MS ); //Delay de 2000ms liberando scheduler;
	}
}
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		if( DEBUG )
		    ESP_LOGI(TAG, "Tentando conectar ao WiFi...\r\n");
		    
		/*
			O WiFi do ESP8266 foi configurado com sucesso. 
			Agora precisamos conectar a rede WiFi local. Portanto, foi chamado a função esp_wifi_connect();
		*/
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
			xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTING_BIT);
			xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
			/*
			Se chegou aqui foi devido a falha de conexão com a rede WiFi.
			Por esse motivo, haverá uma nova tentativa de conexão WiFi pelo ESP8266.
			*/
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Tentando reconectar ao WiFi...");
        } else {
			/*
				É necessário apagar o bit para avisar as demais Tasks que a 
				conexão WiFi está offline no momento. 
			*/
			xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTING_BIT);
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"Falha ao conectar ao WiFi");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		/*
			Conexão efetuada com sucesso. Busca e imprime o IP atribuido. 
		*/
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado! O IP atribuido é:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
		/*
				Seta o bit indicativo para avisar as demais Tasks que o WiFi foi conectado. 
		*/
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
void wifi_init_sta(void)
{
   

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    /* Setting a password implies station will connect to all security modes including WEP/WPA.
        * However these modes are deprecated and not advisable to be used. Incase your Access point
        * doesn't support WPA2, these mode can be enabled by commenting below line */

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Conectado ao AP SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Falha ao conectar ao AP SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "EVENTO INESPERADO");
    }

   // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
  //  ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    //vEventGroupDelete(s_wifi_event_group);
}

void task_ip( void *pvParameter )
{
    
    if( DEBUG )
      ESP_LOGI( TAG, "Inicializada task_ip...\r\n" );
	
    while (TRUE) 
    {    
		/* o EventGroup bloqueia a task enquanto a rede WiFi não for configurada */
		xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);	
    
		/* Após configurada a rede WiFi recebemos e imprimimos a informação do IP atribuido */
		tcpip_adapter_ip_info_t ip_info;
	    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));  
		
		/* Imprime o IP, bloqueia a task e repete a a impressão a cada 5 segundos */
    	if( DEBUG )
      		ESP_LOGI( TAG, "IP atribuido:  %s\n", ip4addr_ntoa(&ip_info.ip) );
		vTaskDelay( 5000/portTICK_PERIOD_MS );
    }
}

void task_Button(void *pvParameter)
{
	gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BUTTON,GPIO_PULLUP_ONLY);
	while(TRUE){
		if(!gpio_get_level(BUTTON)){
			if(!btnApertado){ 
			  gpio_set_level(LED_EXTERNA, 1); 
			  btnApertado = true;
			  xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
              s_retry_num = 0;
			  gpio_set_level(LED_EXTERNA, 0); 
              esp_wifi_connect();
			  xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			}
		}else{
		  btnApertado = false;
		}
		vTaskDelay(50/portTICK_RATE_MS);
	}
}

void app_main()
{

	
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
	event_group = xEventGroupCreate();
		gpio_set_direction( LED_EXTERNA, GPIO_MODE_OUTPUT );
    button_event_group = xEventGroupCreate();	
	 s_wifi_event_group = xEventGroupCreate();


    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
	
	if( (xTaskCreate( task_GPIO_Blink, "task_GPIO_Blink", 2048, NULL, 2, NULL )) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar task_GPIO_Blink.\r\n" );  
      return;   
    } 

    wifi_init_sta();	

    if(xTaskCreate( task_ip, "task_ip", 2048, NULL, 1, NULL )!= pdTRUE )
	{
		if( DEBUG )
			ESP_LOGI( TAG, "error - nao foi possivel alocar Task_IP.\n" );	
		return;		
	}

	if( (xTaskCreate( task_Button, "task_Button", 2048, NULL, 5, NULL )) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar task_Button.\r\n" );  
      return;   
    }  
	

    mqtt_app_start();
	
	if( (xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL)) != pdTRUE )
    {
     
      ESP_LOGI( TAG, "error - Nao foi possivel alocar sensor_task.\r\n" );  
      return;   
    }
}
