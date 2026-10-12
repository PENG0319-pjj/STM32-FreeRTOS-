/**
 * ESP-01S 固件:串口收 STM32 的 $DP 帧 → 解析 → 上报华为云 IoTDA
 *
 * 链路:STM32(GPS报站) --USART3 115200--> ESP-01S --MQTT 1883--> 华为云
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <bearssl/bearssl_hash.h>
#include <bearssl/bearssl_hmac.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

/* ================= 1. 只需要改 WiFi 这两行 ================= */
#define WIFI_SSID       "PENG"
#define WIFI_PASS       "pengjunjie"

#define DEVICE_ID       "6aa8a881cbb0cf6bb983819f_esp01_node"
#define DEVICE_SECRET   "Aa123456"

#define MQTT_HOST       "f8ffb9510e.st1.iotda-device.cn-north-4.myhuaweicloud.com"
#define MQTT_PORT       1883

#define SERVICE_ID      "BusReport"
#define TOPIC_REPORT    "$oc/devices/" DEVICE_ID "/sys/properties/report"

/* ================= 2. 第一次烧录请打开这个 ================= */
#define DEBUG_ON                        /* 注释掉这一行就关闭调试输出 */
#ifdef DEBUG_ON
  #define DBG(...) Serial1.printf(__VA_ARGS__)
#else
  #define DBG(...) do{}while(0)
#endif

WiFiClient   espClient;
PubSubClient mqtt(espClient);

/* ---------- HMAC-SHA256 → 64 位小写 hex ---------- */
static void hmac_sha256_hex(const char *key, const char *msg, char *out)
{
    unsigned char mac[32];
    br_hmac_key_context kc;
    br_hmac_context     hc;

    br_hmac_key_init(&kc, &br_sha256_vtable, key, strlen(key));
    br_hmac_init(&hc, &kc, 0);                          /* 0 = 完整 32 字节输出 */
    br_hmac_update(&hc, msg, strlen(msg));
    br_hmac_out(&hc, mac);

    for (int i = 0; i < 32; i++) sprintf(out + i * 2, "%02x", mac[i]);
    out[64] = '\0';
}

/* ---------- UTC 时间戳 "YYYYMMDDHH" ---------- */
static void make_timestamp(char *out)
{
    time_t now = time(nullptr);
    struct tm *g = gmtime(&now);
    sprintf(out, "%04d%02d%02d%02d",
            g->tm_year + 1900, g->tm_mon + 1, g->tm_mday, g->tm_hour);
}

/* ---------- 连 WiFi ---------- *
 * WiFi.status() 返回值:
 *   1 = WL_NO_SSID_AVAIL   → 找不到这个 SSID(最常见:路由器是 5GHz,ESP8266 只支持 2.4G)
 *   4 = WL_CONNECT_FAILED  → 密码错
 *   6 = WL_DISCONNECTED    → 掉线 / 信号太弱 / 认证失败
 *   3 = WL_CONNECTED       → 成功
 */
static void wifi_connect(void)
{
    if (WiFi.status() == WL_CONNECTED) return;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    DBG("WiFi connecting to [%s]\n", WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        DBG("  status=%d\n", WiFi.status());
    }
    DBG("WiFi OK ip=%s\n", WiFi.localIP().toString().c_str());
}

/* ---------- 连华为云 ---------- */
static void mqtt_connect(void)
{
    char ts[11], pass[65], clientId[128];

    while (!mqtt.connected())
    {
        wifi_connect();
        make_timestamp(ts);
        hmac_sha256_hex(ts, DEVICE_SECRET, pass);   /* key=时间戳, 内容=密钥 */
        snprintf(clientId, sizeof(clientId), "%s_0_0_%s", DEVICE_ID, ts);

        DBG("MQTT %s\n", clientId);
        if (mqtt.connect(clientId, DEVICE_ID, pass))
            DBG("MQTT OK\n");
        else
        {
            DBG("MQTT fail rc=%d\n", mqtt.state());
            delay(5000);
        }
    }
}

/* ---------- "28.068777" → 28068777(微度,纯整数运算) ---------- */
static long deg_to_udeg(const char *s)
{
    bool neg = (*s == '-');
    if (neg) s++;

    long int_part = 0;
    while (*s >= '0' && *s <= '9') int_part = int_part * 10 + (*s++ - '0');

    long frac = 0, scale = 1;
    if (*s == '.')
    {
        s++;
        while (*s >= '0' && *s <= '9' && scale < 1000000)
        { frac = frac * 10 + (*s++ - '0'); scale *= 10; }
    }
    while (scale < 1000000) { frac *= 10; scale *= 10; }

    long v = int_part * 1000000 + frac;
    return neg ? -v : v;
}

/* ---------- 收到一整行,解析并上报 ---------- */
static void handle_frame(char *p)
{
    char *star = strchr(p, '*');
    if (!star) return;

    uint8_t cs = 0;
    for (char *q = p + 1; q < star; q++) cs ^= (uint8_t)(*q);
    if ((uint8_t)strtoul(star + 1, NULL, 16) != cs) { DBG("CS bad\n"); return; }
    *star = '\0';

    char *f[11];
    uint8_t n = 0;
    f[n++] = p;
    for (char *q = p; *q && n < 11; q++)
        if (*q == ',') { *q = '\0'; f[n++] = q + 1; }
    if (n < 11 || strcmp(f[0], "$DP") != 0) { DBG("fields=%d\n", n); return; }

    long lat = deg_to_udeg(f[1]);
    long lng = deg_to_udeg(f[2]);
    int  fix = atoi(f[3]);
    int  spd = atoi(f[10]);

    char rtc[24];
    snprintf(rtc, sizeof(rtc), "20%02d-%02d-%02d %02d:%02d:%02d",
             atoi(f[4]), atoi(f[5]), atoi(f[6]),
             atoi(f[7]), atoi(f[8]), atoi(f[9]));

    char payload[224];
    snprintf(payload, sizeof(payload),
        "{\"services\":[{\"service_id\":\"%s\",\"properties\":"
        "{\"lat\":%ld,\"lng\":%ld,\"fix\":%d,\"spd\":%d,\"rtc\":\"%s\"}}]}",
        SERVICE_ID, lat, lng, fix, spd, rtc);

    DBG("pub: %s\n", payload);
    DBG(mqtt.publish(TOPIC_REPORT, payload) ? "publish OK\n" : "publish FAIL\n");
}

char    line[128];
uint8_t lineLen = 0;

void setup()
{
#ifdef DEBUG_ON
    Serial1.begin(115200);          /* GPIO2 作调试串口 */
#endif
    Serial.begin(115200);           /* 和 STM32 的链路 */

    wifi_connect();

    /* NTP 对时是【异步】的:configTime() 只是"启动对时"就立刻返回,
     * 必须等 time() 变成合理值,否则时间戳会是 1970 年。
     * 最多等 10 秒;即使没对上,因为 ClientId 用的是 _0_0_(不校验时间戳),也能连上。 */
    configTime(0, 0, "pool.ntp.org", "ntp.aliyun.com");
    DBG("NTP syncing");
    for (int i = 0; i < 20 && time(nullptr) < 1000000000L; i++)
    {
        delay(500);
        DBG(".");
    }
    DBG("\n");

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setKeepAlive(60);
    mqtt_connect();

    DBG("ready\n");
}

void loop()
{
    if (!mqtt.connected()) mqtt_connect();
    mqtt.loop();

    while (Serial.available())
    {
        char c = Serial.read();
        if (c == '$') lineLen = 0;                    /* 帧头重新同步 */
        if (lineLen < sizeof(line) - 1) line[lineLen++] = c;
        if (c == '\n')
        {
            line[lineLen] = '\0';
            handle_frame(line);
            lineLen = 0;
        }
    }
}
