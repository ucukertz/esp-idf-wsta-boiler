#include <esp_wifi.h>

#include <lwip/err.h>
#include <lwip/sys.h>

#include <sys/param.h>

#include "unifier.h"

static const char* TAG = "APSTA";

/* CONFIGURABLES */
static const u32_t WIFI_STA_RETRY_SEC = 3;
static const u32_t WIFI_STA_MAX_RETRY = 0;

static const char* WIFI_AP_PASS = "";
static u8_t WIFI_AP_MAX_CONN = 2;

static const u8_t WIFI_DEFAULT_CHANNEL = 7;
/* CONFIGURABLES END */

static const bool TEST_WIFI_CRED = true;
static const char* TEST_WIFI_SSID = "TEST_WIFI_SSID";
static const char* TEST_WIFI_PASS = "TEST_WIFI_PASS";

static char wifi_sta_ssid[33];
static char wifi_sta_pass[65];
static char wifi_sta_ip4[16];

static int cnt_wconnect_retry = 0;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    //STA events
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        strcpy(wifi_sta_ip4, "0.0.0.0");

        ESP_LOGW(TAG, "Failed to connect to SSID:%s, password:%s",
                     wifi_sta_ssid, wifi_sta_pass);
        TASK_DELAY_SEC(WIFI_STA_RETRY_SEC);
        esp_wifi_connect();
        ESP_LOGI(TAG, "Retrying to connect to the AP");

        if (cnt_wconnect_retry < WIFI_STA_MAX_RETRY || WIFI_STA_MAX_RETRY == 0) {
            cnt_wconnect_retry++;
        }
        else {
            wifi_to_apsta();
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {

        ESP_LOGI(TAG, "Connected to ap SSID:%s password:%s",
                 wifi_sta_ssid, wifi_sta_pass);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        snprintf(wifi_sta_ip4, sizeof(wifi_sta_ip4), IPSTR, IP2STR(&event->ip_info.ip));
        cnt_wconnect_retry = 0;
    }

    //AP events
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGW(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGW(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

// Set new credentials to be used on Wi-Fi station. Call wifi_to_sta() or wifi_to_apsta to apply it.
void wifi_sta_set_cred(const char* ssid, const char* password)
{
    strcpy(wifi_sta_ssid, ssid);
    strcpy(wifi_sta_pass, password);
}

// Save newest Wi-Fi station credentials setting to NVS. The setting will be loaded on future device start ups.
void wifi_sta_save_cred()
{
    nvs_handle_t nvsh;
    nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsh);

    ESP_LOGI(TAG, "Saving Wi-Fi SSID: %s Pass: %s to NVS", wifi_sta_ssid, wifi_sta_pass);

    nvs_set_str(nvsh, "wssid", wifi_sta_ssid);
    nvs_set_str(nvsh, "wpass", wifi_sta_pass);
    nvs_commit(nvsh);

    nvs_close(nvsh);
}

char* wifi_sta_get_ssid()
{
    return wifi_sta_ssid;
}

char* wifi_sta_get_ip4()
{
    return wifi_sta_ip4;
}

i8_t wifi_sta_get_rssi()
{
    wifi_ap_record_t wifi_record;
    esp_wifi_sta_get_ap_info(&wifi_record);
    return wifi_record.rssi;
}

bool wifi_is_ap_active()
{
    wifi_mode_t wifi_mode;
    esp_wifi_get_mode(&wifi_mode);

    if (wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_APSTA) return 1;
    else return 0;
}

void wifi_config_set_sta(wifi_config_t* wconfig)
{
    strcpy((char *)wconfig->sta.ssid, wifi_sta_ssid);
    strcpy((char *)wconfig->sta.password, wifi_sta_pass);
    wconfig->sta.channel = WIFI_DEFAULT_CHANNEL;
    wconfig->sta.scan_method = WIFI_FAST_SCAN;
    ESP_LOGW(TAG, "Connecting STA to SSID = %s Password = %s", wifi_sta_ssid, wifi_sta_pass);
}

void wifi_config_set_ap(wifi_config_t* wconfig)
{
    u8_t WIFI_AP_SSID[33];

    esp_efuse_mac_get_default(WIFI_AP_SSID);

    snprintf((char*)WIFI_AP_SSID, sizeof(WIFI_AP_SSID), "SmartInside-%02x%02x%02x-apf", WIFI_AP_SSID[3]
                                                                                      , WIFI_AP_SSID[4]
                                                                                      , WIFI_AP_SSID[5]);
    ESP_LOGW(TAG, "Starting AP with SSID: %s", (char*)WIFI_AP_SSID);

    strcpy((char *)wconfig->ap.ssid, (char*)WIFI_AP_SSID);
    strcpy((char *)wconfig->ap.password, WIFI_AP_PASS);
    wconfig->ap.ssid_len = strlen((char*)WIFI_AP_SSID);
    wconfig->ap.channel = WIFI_DEFAULT_CHANNEL;
    wconfig->ap.max_connection = WIFI_AP_MAX_CONN;
    wconfig->ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    if (strlen(WIFI_AP_PASS) == 0) {
        wconfig->ap.authmode = WIFI_AUTH_OPEN;
    }
}

// Switch Wi-Fi mode to STA - Newest STA credential configuration will be applied
void wifi_to_sta()
{
    wifi_config_t sta_config = {0};
    wifi_config_set_sta(&sta_config);
    sta_config.sta.channel = 0; // No need to lock channel when purely on STA mode

    ESP_LOGI(TAG, "Connecting to SSID = %s Password = %s", wifi_sta_ssid, wifi_sta_pass);

    // ESP_LOGI(TAG, "wmode: %d", wifi_mode);
    esp_wifi_stop();
    esp_wifi_restore();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGW(TAG, "Wi-Fi mode switched to STA");
}

// Switch Wi-Fi mode to AP
void wifi_to_ap()
{
    wifi_config_t ap_config = {0};
    wifi_config_set_ap(&ap_config);

    wifi_mode_t wifi_mode;
    esp_wifi_get_mode(&wifi_mode);

    if (!wifi_is_ap_active()) {
        // ESP_LOGI(TAG, "wmode: %d", wifi_mode);
        esp_wifi_stop();
        esp_wifi_restore();
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGW(TAG, "Wi-Fi mode switched to AP");
}

// Switch Wi-Fi mode to AP - Newest STA credential configuration will be applied
void wifi_to_apsta()
{
    wifi_config_t ap_config = {0};
    wifi_config_set_ap(&ap_config);
    wifi_config_t sta_config = {0};
    wifi_config_set_sta(&sta_config);

    wifi_mode_t wifi_mode;
    esp_wifi_get_mode(&wifi_mode);

    if (wifi_mode != WIFI_MODE_APSTA) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();

    ESP_LOGW(TAG, "Wi-Fi mode switched to APSTA");
}

void wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_create_default_wifi_sta();
    esp_netif_t* wifiAP = esp_netif_create_default_wifi_ap();

    // Sets AP's default IP address
    esp_netif_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192,168,4,1);
	IP4_ADDR(&ipInfo.gw, 192,168,4,1);
	IP4_ADDR(&ipInfo.netmask, 255,255,255,0);
	esp_netif_dhcps_stop(wifiAP);
	esp_netif_set_ip_info(wifiAP, &ipInfo);
	esp_netif_dhcps_start(wifiAP);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    strcpy(wifi_sta_ssid, "");
    strcpy(wifi_sta_pass, "");

    nvs_handle_t nvsh;
    nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsh);

    size_t wssid_len;
    size_t wpass_len;

    nvs_get_str(nvsh, "wssid", NULL, &wssid_len);
    nvs_get_str(nvsh, "wpass", NULL, &wpass_len);

    nvs_get_str(nvsh, "wssid", wifi_sta_ssid, &wssid_len);
    nvs_get_str(nvsh, "wpass", wifi_sta_pass, &wpass_len);

    nvs_close(nvsh);

    esp_event_handler_instance_t instance_wifi_evt;
    esp_event_handler_instance_t instance_ip_evt;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_wifi_evt));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_ip_evt));

    // Sets both Wi-Fi AP and STA to use 802.11b/g/n
    u8_t wifi_protocol_bitmap_ap, wifi_protocol_bitmap_sta;
    u8_t wifi_protocol_bgn = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N;
    esp_wifi_set_protocol(WIFI_IF_AP, wifi_protocol_bgn);
    esp_wifi_set_protocol(WIFI_IF_STA, wifi_protocol_bgn);
    esp_wifi_get_protocol(WIFI_IF_AP, &wifi_protocol_bitmap_ap);
    esp_wifi_get_protocol(WIFI_IF_STA, &wifi_protocol_bitmap_sta);
    ESP_LOGW(TAG, "Protocols -> AP: %u STA: %u", wifi_protocol_bitmap_ap, wifi_protocol_bitmap_sta);

    if (TEST_WIFI_CRED) {
        strcpy(wifi_sta_ssid, TEST_WIFI_SSID);
        strcpy(wifi_sta_pass, TEST_WIFI_PASS);
    }

    if (strlen(wifi_sta_ssid) > 0) {
        ESP_LOGW(TAG, "SSID len: %u", strlen(wifi_sta_ssid));
        ESP_LOGW(TAG, "Wi-Fi mode init to STA");
        wifi_to_sta();
    }
    else {
        ESP_LOGW(TAG, "Wi-Fi mode init to APSTA");
        wifi_to_apsta();
    }
}