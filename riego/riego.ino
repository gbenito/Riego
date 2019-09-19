/******************************************************************************/
/* RIEGO.INO                                                                  */
/******************************************************************************/
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ssl_client.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <DHTesp.h>
/******************************************************************************/
#define DEBUG_NIVEL  0
#define DEBUG_TIEMPO 0
#define MI_DEBUG     0
#define LIMITES      1
/******************************************************************************/
#if MI_DEBUG
  #define miPrint(a)   Serial.print(a)
  #define miPrintln(a) Serial.println(a)
#else
  #define miPrint(a)   
  #define miPrintln(a) 
#endif
/******************************************************************************/
#define DHTPIN     14
#define SUELO_PIN1 A4 // 32
#define SUELO_PIN2 A5 // 33
#define LED_PIN1   21
#define LED_PIN2   19
#define NIVEL      18
/******************************************************************************/
#if LIMITES
  #define ALARMA_1   2500
  #define LIMITE_1   3000
  #define LIMITE_2   3500
  #define ALARMA_2   4050
  #define HISTERESIS 30
  #define TXT_A1     "Encharcado"
  #define TXT_L1_A1  "Muy Humedo"
  #define TXT_L2_L1  "Humedo"
  #define TXT_A2_L2  "Medio"
  #define TXT_A2     "Seco"
#else
  #define ALARMA_1   1000
  #define LIMITE_1   1850
  #define LIMITE_2   2450
  #define ALARMA_2   3100
  #define HISTERESIS 50
  #define TXT_A1     "Humedo *"
  #define TXT_L1_A1  "Humedo"
  #define TXT_L2_L1  "Medio"
  #define TXT_A2_L2  "Seco"
  #define TXT_A2     "Seco *"
#endif
/******************************************************************************/
DHTesp dht;
static int estadoSuelo1 = 0;
static int estadoSuelo2 = 0;
/******************************************************************************/
typedef struct
{
  char ssid[256];
  char password[256];
  char clientId[256];
}
wifiStd;
/******************************************************************************/
#define NUM_RED (2)
static const wifiStd wifiList[NUM_RED] = {
                                     {"WIFI_NAME_1", "WIFI_KEY_1", "riego1"},
                                     {"WIFI_NAME_2", "WIFI_KEY_2", "riego2"}
                                   };
/******************************************************************************/
const char* mqtt_server1 = "server1.com";
const char* mqtt_server2 = "server2.com";
struct TempAndHumidity tempHumi;
WiFiClientSecure espClient;
String clientId;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "hora.roa.es",7200,6000);
/******************************************************************************/
/* topics */
char * pTopic1;
char * pTopic2;
char * pTopic3;
char * pTopic4;
char * pTopic5;
char * pTopic6;
char * pTopic7;
char * pTopic8;
char * pTopic9;
char * pTopic10;
char * pTopic11;
char * pTopic12;
char * pTopic13;
char * pTopic14;

#define TEMP_TOPICA1    "arteche/hora"
#define TEMP_TOPICA2    "arteche/temp"
#define TEMP_TOPICA3    "arteche/humedad"
#define TEMP_TOPICA4    "arteche/led1"
#define TEMP_TOPICA5    "arteche/suelo1"
#define TEMP_TOPICA6    "arteche/suelo2"
#define TEMP_TOPICA7    "arteche/led2"
#define TEMP_TOPICA8    "arteche/Tsuelo1"
#define TEMP_TOPICA9    "arteche/Tsuelo2"
#define TEMP_TOPICA10   "arteche/Nivel"
#define TEMP_TOPICA11   "arteche/stdb1"
#define TEMP_TOPICA12   "arteche/stdb2"
#define TEMP_TOPICA13    "arteche/led1r"
#define TEMP_TOPICA14    "arteche/led2r"
#define TEMP_TOPICM1    "casam/hora"
#define TEMP_TOPICM2    "casam/temp"
#define TEMP_TOPICM3    "casam/humedad"
#define TEMP_TOPICM4    "casam/led1"
#define TEMP_TOPICM5    "casam/suelo1"
#define TEMP_TOPICM6    "casam/suelo2"
#define TEMP_TOPICM7    "casam/led2"
#define TEMP_TOPICM8    "casam/Tsuelo1"
#define TEMP_TOPICM9    "casam/Tsuelo2"
#define TEMP_TOPICM10   "casam/Nivel"
#define TEMP_TOPICM11   "casam/stdb1"
#define TEMP_TOPICM12   "casam/stdb2"
#define TEMP_TOPICM13   "casam/led1r"
#define TEMP_TOPICM14   "casam/led2r"
static long lastMsg;
static long bomba1Time;
static long bomba2Time;
static char msg[20];
/******************************************************************************/
#define WDT_TIMEOUT 90000000  // 90 s
static hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule()
{
  ets_printf("reboot\n");
  esp_restart();
}
/******************************************************************************/
#define STD_OFF     0
#define STD_ON      1
#define STD_BLOCKED 3
#define PIN_ON      1
#define PIN_OFF     0
static int stdBomba1;
static int stdBomba2;
static int nivel;
void callback(char* topic, byte* payload, unsigned int length)
{
#if MI_DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
#endif

  if(0 == strcmp(topic, pTopic4))
  {
    if(0 < length)
    {
      if ((char)payload[0] == '1')
      {
        miPrintln("Bomba 1 arrancar");
        if(STD_OFF == stdBomba1)
        {
          if(0 != nivel)
          {
            miPrintln("Bomba 1 NO hay agua");
            client.publish(pTopic13, "0");
          }
          else
          {
            digitalWrite(LED_PIN1, PIN_ON);
            stdBomba1 = STD_ON;
            client.publish(pTopic11, "ON");
            client.publish(pTopic13, "1");
            miPrintln("Bomba 1 on");
            bomba1Time = millis();
          }
        }
        else if(STD_BLOCKED == stdBomba1)
        {
          client.publish(pTopic13, "0");
           miPrintln("Bomba 1 blocked");
        }
      }
      else if((char)payload[0] == '0')
      {
        if(STD_ON == stdBomba1)
        {
          miPrintln("Bomba 1 No parar");
          client.publish(pTopic13, "1");
        }
      }
    }
  }
  else if(0 == strcmp(topic, pTopic7))
  {
    if(0 < length)
    {
      if ((char)payload[0] == '1')
      {
        if(STD_OFF == stdBomba2)
        {
          if(0 != nivel)
          {
            miPrintln("Bomba 2 NO hay agua");
            client.publish(pTopic14, "0");
          }
          else
          {
            digitalWrite(LED_PIN2, PIN_ON);
            stdBomba2 = STD_ON;
            client.publish(pTopic12, "ON");
            client.publish(pTopic14, "1");
            miPrintln("Bomba 2 on");
            bomba2Time = millis();
          }
        }
        else if(STD_BLOCKED == stdBomba2)
        {
          client.publish(pTopic14, "0");
          miPrintln("Bomba 2 blocked");
        }
      }
      else if((char)payload[0] == '0')
      {
        if(STD_ON == stdBomba2)
        {
          miPrintln("Bomba 2 No parar");
          client.publish(pTopic14, "1");
        }
      }
    }
  }
}
/******************************************************************************/
void mqttconnect() {
  /* Loop until reconnected */
  if(!client.connected())
  {
    Serial.println("MQTT connecting ...");
    /* client ID */
    /* connect now */
    if (client.connect(clientId.c_str(), "USER", "PASSWORD"))
    {
      Serial.println("connected");
      client.subscribe(pTopic4);
      client.subscribe(pTopic7);
    }else
    {
      Serial.print("failed, status code =");
      Serial.println(client.state());
      delay(5000);
    }
  }
}
/******************************************************************************/
int scanAndConect()
{
    int retorno = 0;
    int cont = 0;
    int red = 0;
    for(red = 0; NUM_RED > red; red++)
    {
      Serial.println(wifiList[red].ssid);
      WiFi.begin(wifiList[red].ssid, wifiList[red].password);
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(500);
        Serial.print("#");
        cont++;
        if(20 < cont )
        {
          Serial.println("");
          cont = -1;
          break;
        }
      }
      if(0 <= cont)
      {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        timeClient.begin();
        retorno = 0;
        if(0 == red)
        {
          pTopic1  = (char*)TEMP_TOPICM1;
          pTopic2  = (char*)TEMP_TOPICM2;
          pTopic3  = (char*)TEMP_TOPICM3;
          pTopic4  = (char*)TEMP_TOPICM4;
          pTopic5  = (char*)TEMP_TOPICM5;
          pTopic6  = (char*)TEMP_TOPICM6;
          pTopic7  = (char*)TEMP_TOPICM7;
          pTopic8  = (char*)TEMP_TOPICM8;
          pTopic9  = (char*)TEMP_TOPICM9;
          pTopic10 = (char*)TEMP_TOPICM10;
          pTopic11 = (char*)TEMP_TOPICM11;
          pTopic12 = (char*)TEMP_TOPICM12;
          pTopic13 = (char*)TEMP_TOPICM13;
          pTopic14 = (char*)TEMP_TOPICM14;
          client.setServer(mqtt_server1, 8883);
        }
        else
        {
          pTopic1  = (char*)TEMP_TOPICA1;
          pTopic2  = (char*)TEMP_TOPICA2;
          pTopic3  = (char*)TEMP_TOPICA3;
          pTopic4  = (char*)TEMP_TOPICA4;
          pTopic5  = (char*)TEMP_TOPICA5;
          pTopic6  = (char*)TEMP_TOPICA6;
          pTopic7  = (char*)TEMP_TOPICA7;
          pTopic8  = (char*)TEMP_TOPICA8;
          pTopic9  = (char*)TEMP_TOPICA9;
          pTopic10 = (char*)TEMP_TOPICA10;
          pTopic11 = (char*)TEMP_TOPICA11;
          pTopic12 = (char*)TEMP_TOPICA12;
          pTopic13 = (char*)TEMP_TOPICA13;
          pTopic14 = (char*)TEMP_TOPICA14;
          client.setServer(mqtt_server2, 8883);
        }
        clientId = wifiList[red].clientId;
        break;
      }
      else
      {
        Serial.println("WiFi NO connected");
        retorno = -1;
      }
    }
    return retorno;
}
/******************************************************************************/
void setup()
{
  int n;
  Serial.begin(115200);
  delay(1000);
  pinMode (LED_PIN1, OUTPUT);
  pinMode (LED_PIN2, OUTPUT);
  pinMode(NIVEL, INPUT_PULLUP);
  client.setCallback(callback);
  dht.setup(DHTPIN, DHTesp::AUTO_DETECT);
  stdBomba1 = STD_OFF;
  stdBomba2 = STD_OFF;
  nivel     = 1;
  lastMsg   = 0;
  bomba1Time = 0;
  bomba2Time = 0;
  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, WDT_TIMEOUT, false);       //set time in us
  timerAlarmEnable(timer);   
}
/******************************************************************************/
#define STD_ALARMA_SECO   4
#define STD_SECO          3
#define STD_MEDIO         2
#define STD_HUMEDO        1
#define STD_ALARMA_HUMEDO 0

void sendTiestoHumedad (int pin, char * localTopic1, char * localTopic2, int * estado)
{
    static int suelo;
    suelo = analogRead(pin);
    
    snprintf (msg, 20, "%d", suelo);
    client.publish(localTopic1, msg);
    
    switch(*estado)
    {
/******************************************************************************/
      case STD_ALARMA_HUMEDO:
        if((ALARMA_2 + HISTERESIS) < suelo)
        {
          *estado = STD_ALARMA_SECO;
          client.publish(localTopic2, TXT_A2);
        }
        else if((LIMITE_2 + HISTERESIS) < suelo)
        {
          *estado = STD_SECO;
          client.publish(localTopic2, TXT_A2_L2);
        }
        else if((LIMITE_1 + HISTERESIS) < suelo)
        {
          *estado = STD_MEDIO;
          client.publish(localTopic2, TXT_L2_L1);
        }
        else if((ALARMA_1 + HISTERESIS) < suelo)
        {
          *estado = STD_HUMEDO;
          client.publish(localTopic2, TXT_L1_A1);
        }
        else
        {
          client.publish(localTopic2, TXT_A1);
        }
        break;
/******************************************************************************/
      case STD_HUMEDO:
        if((ALARMA_2 + HISTERESIS) < suelo)
        {
          *estado = STD_ALARMA_SECO;
          client.publish(localTopic2, TXT_A2);
        }
        else if((LIMITE_2 + HISTERESIS) < suelo)
        {
          *estado = STD_SECO;
          client.publish(localTopic2, TXT_A2_L2);
        }
        else if((LIMITE_1 + HISTERESIS) < suelo)
        {
          *estado = STD_MEDIO;
          client.publish(localTopic2, TXT_L2_L1);
        }
/***************/
        else if((ALARMA_1 - HISTERESIS) > suelo)
        {
          *estado = STD_ALARMA_HUMEDO;
          client.publish(localTopic2, TXT_A1);
        }
        else
        {
          client.publish(localTopic2, TXT_L1_A1);
        }
        break;
/******************************************************************************/
      case STD_MEDIO:
        if((ALARMA_2 + HISTERESIS) < suelo)
        {
          *estado = STD_ALARMA_SECO;
          client.publish(localTopic2, TXT_A2);
        }
        else if((LIMITE_2 + HISTERESIS) < suelo)
        {
          *estado = STD_SECO;
          client.publish(localTopic2, TXT_A2_L2);
        }
/***************/
        else if((ALARMA_1 - HISTERESIS) > suelo)
        {
          *estado = STD_ALARMA_HUMEDO;
          client.publish(localTopic2, TXT_A1);
        }
        else if((LIMITE_1 - HISTERESIS) > suelo)
        {
          *estado = STD_HUMEDO;
          client.publish(localTopic2, TXT_L1_A1);
        }
        else
        {
          client.publish(localTopic2, TXT_L2_L1);
        }
        break;
/******************************************************************************/
      case STD_SECO:
        if((ALARMA_2 + HISTERESIS) < suelo)
        {
          *estado = STD_ALARMA_SECO;
          client.publish(localTopic2, TXT_A2);
        }
/***************/
        else if((ALARMA_1 - HISTERESIS) > suelo)
        {
          *estado = STD_ALARMA_HUMEDO;
          client.publish(localTopic2, TXT_A1);
        }
        else if((LIMITE_1 - HISTERESIS) > suelo)
        {
          *estado = STD_HUMEDO;
          client.publish(localTopic2, TXT_L1_A1);
        }
        else if((LIMITE_2 - HISTERESIS) > suelo)
        {
          *estado = STD_MEDIO;
          client.publish(localTopic2, "Madio");
        }
        else
        {
          client.publish(localTopic2, TXT_A2_L2);
        }
        break;
/******************************************************************************/
      case STD_ALARMA_SECO:
        if((ALARMA_1 - HISTERESIS) > suelo)
        {
          *estado = STD_ALARMA_HUMEDO;
          client.publish(localTopic2, TXT_A1);
        }
        else if((LIMITE_1 - HISTERESIS) > suelo)
        {
          *estado = STD_HUMEDO;
          client.publish(localTopic2, TXT_L1_A1);
        }
        else if((LIMITE_2 - HISTERESIS) > suelo)
        {
          *estado = STD_MEDIO;
          client.publish(localTopic2, "Madio");
        }
        if((ALARMA_2 - HISTERESIS) > suelo)
        {
          *estado = STD_SECO;
          client.publish(localTopic2, TXT_A2_L2);
        }
        else
        {
          client.publish(localTopic2, TXT_A2);
        }
        break;
/******************************************************************************/
      default:
        *estado = STD_ALARMA_HUMEDO;
        client.publish(localTopic2, "Error");
        break;
    }
}
/******************************************************************************/
#if DEBUG_TIEMPO
  #define TIEMPO_MSG        5000 //  5 s
  #define TIEMPO_BOMBA      4000 //  4 s
  #define TIEMPO_BLOCKED   30000 // 30 s
#else
  #define TIEMPO_MSG        5000 //  5 s
  #define TIEMPO_BOMBA      4000 //  4 s
  #define TIEMPO_BLOCKED 3600000 //  1 h
#endif
void loop()
{
  timerWrite(timer, 0); //reset timer (feed watchdog);
  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    scanAndConect();
    stdBomba1  = STD_OFF;
    digitalWrite(LED_PIN1, PIN_OFF);
    stdBomba2  = STD_OFF;
    digitalWrite(LED_PIN2, PIN_OFF);
  }
  else
  {
    if(!client.connected())
    {
      Serial.println("");
      mqttconnect();
      delay(10000);
      stdBomba1  = STD_OFF;
      digitalWrite(LED_PIN1, PIN_OFF);
      stdBomba2  = STD_OFF;
      digitalWrite(LED_PIN2, PIN_OFF);
    }
    else
    {
      client.loop();
      long now = millis();
      if((now - lastMsg) > TIEMPO_MSG)
      {
        Serial.print(".");
        timeClient.update();
        lastMsg = now;
        snprintf (msg, 20, "%s", timeClient.getFormattedTime().c_str());
        tempHumi = dht.getTempAndHumidity();
        client.publish(pTopic1, msg);
  
        if(!isnan(tempHumi.temperature)) 
        {
          snprintf (msg, 20, "%.1f", tempHumi.temperature);
        }
        else
        {
           snprintf (msg, 20, "--");
        }
        client.publish(pTopic2, msg);
        if(!isnan(tempHumi.humidity)) 
        {
          snprintf (msg, 20, "%.0f", tempHumi.humidity);
        }
        else
        {
           snprintf (msg, 20, "--");
        }
        client.publish(pTopic3, msg);
        
        sendTiestoHumedad (SUELO_PIN1, pTopic5, pTopic8, &estadoSuelo1);
        sendTiestoHumedad (SUELO_PIN2, pTopic6, pTopic9, &estadoSuelo2);
        
#if DEBUG_NIVEL
        nivel = 0;
#else
        nivel = digitalRead(NIVEL);
#endif
        if(0 == nivel)
        {
          client.publish(pTopic10, "NORMAL");
        }
        else
        {
          client.publish(pTopic10, "BAJO");
        }
/******************************************************************************/
        switch(stdBomba1)
        {
          case STD_OFF:
            client.publish(pTopic11, "OFF");
            break;
          case STD_ON:
            client.publish(pTopic11, "ON");
            break;
          case STD_BLOCKED:
            client.publish(pTopic11, "BLOCKED");
            break;
          default:
            client.publish(pTopic11, "ERROR");
            break;
        }
/******************************************************************************/
        switch(stdBomba2)
        {
          case STD_OFF:
            client.publish(pTopic12, "OFF");
            break;
          case STD_ON:
            client.publish(pTopic12, "ON");
            break;
          case STD_BLOCKED:
            client.publish(pTopic12, "BLOCKED");
            break;
          default:
            client.publish(pTopic12, "ERROR");
            break;
        }
      }
/******************************************************************************/
      switch(stdBomba1)
      {
        case STD_OFF:
          if((now - bomba1Time) > TIEMPO_MSG)
          {
            bomba1Time = now;
            client.publish(pTopic13, "0");
          }
          break;
        case STD_ON:
          if((now - bomba1Time) > TIEMPO_BOMBA)
          {
            bomba1Time = now;
            stdBomba1  = STD_BLOCKED;
            client.publish(pTopic11, "BLOCKED");
            digitalWrite(LED_PIN1, PIN_OFF);
            client.publish(pTopic13, "0");
            miPrintln("Bomba 1 STD_BLOCKED");
          }
          break;
        case STD_BLOCKED:
          if((now - bomba1Time) > TIEMPO_BLOCKED)
          {
            bomba1Time = now;
            stdBomba1  = STD_OFF;
            client.publish(pTopic11, "STD_OFF");
            miPrintln("Bomba 1 STD_OFF");
          }
          break;
        default:
          stdBomba1 = STD_OFF;
          client.publish(pTopic11, "STD_OFF");
          break;
      }
/******************************************************************************/
      switch(stdBomba2)
      {
        case STD_OFF:
          if((now - bomba2Time) > TIEMPO_MSG)
          {
            bomba2Time = now;
            client.publish(pTopic14, "0");
          }
          break;
        case STD_ON:
          if((now - bomba2Time) > TIEMPO_BOMBA)
          {
            bomba2Time = now;
            stdBomba2  = STD_BLOCKED;
            client.publish(pTopic12, "BLOCKED");
            digitalWrite(LED_PIN2, PIN_OFF);
            client.publish(pTopic14, "0");
            miPrintln("Bomba 2 STD_BLOCKED");
          }
          break;
        case STD_BLOCKED:
          if((now - bomba2Time) > TIEMPO_BLOCKED)
          {
            bomba2Time = now;
            stdBomba2  = STD_OFF;
            client.publish(pTopic12, "STD_OFF");
            miPrintln("Bomba 2 STD_OFF");
          }
          break;
        default:
          stdBomba2 = STD_OFF;
          client.publish(pTopic12, "STD_OFF");
          break;
      }
          
    }
  }
}
/******************************************************************************/
