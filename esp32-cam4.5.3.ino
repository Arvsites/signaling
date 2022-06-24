// GPIO0 должен быть подключен к GND для загрузки кода

// Подключение необходимых библиотек
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

String token = "5061372116:AAFt_Aeb56jHQCaYRK2bWxzQf3ZVC91hzeE";   // Токен бота
String chat_id = "";    // ID Бота
int pinPIR = 13;        // Выход PIR lfnxbrf
int t = 60000;
bool sendPhoto = false;
bool flag = 0;



unsigned long botRequestDelay = 1000, time2 = 0, time3 = 30000;
unsigned long lastTimeBotRan;

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(token, clientTCP);

const char* ssid = "An";//"iPhone (Иван)";    // Имя сети
const char* password = "87878787";//"0864297531";  // Пароль сети



// контакты для модуля камеры AI-THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define ldrPin 14


//=============это для wifi подключения=============
String str = "";
boolean conf = false;
String html_header = "<html>\
 <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
 <head>\
   <title>ESP8266 Settings</title>\
   <style>\
     body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
   </style>\
 </head>";
WebServer server(80);
void handleRoot() {
  String str = "";
  str += html_header;
  str += 
  "<style>\
  .outer {\
    text-align: center;\
    border: 1px solid blue;\
  }\
</style>\
  <body>\
  <div class=\"outer\">\
   <form method=\"POST\" action=\"ok\">\
     <h1><input name=\"ssid\"></br> Имя точки доступа<\h1></br>\
     <input name=\"pswd\"> </br>Пароль точки доступа</br></br></br>\
     <input name=\"telegramid\"></br> id telegram</br></br></br>\
     <input type=SUBMIT value=\"Сохранить\">\
   </form>\
   </div>\
 </body>\
</html>";
  server.send ( 200, "text/html", str );
}
void handleOk() {
  String ssid_ap;
  String pass_ap;
  String telegramid_ap;
  unsigned char* buf = new unsigned char[64];

  String str = "";
  str += html_header;
  str += "<body>";

  EEPROM.begin(98);

  ssid_ap = server.arg(0);
  pass_ap = server.arg(1);
  telegramid_ap = server.arg(2);

  if (ssid_ap != "") {
    EEPROM.write(96, ssid_ap.length());
    EEPROM.write(97, pass_ap.length());
    if (telegramid_ap != "") {
      EEPROM.write(95, telegramid_ap.length());
    }
    ssid_ap.getBytes(buf, ssid_ap.length() + 1);
    for (byte i = 0; i < ssid_ap.length(); i++)
      EEPROM.write(i, buf[i]);

    pass_ap.getBytes(buf, pass_ap.length() + 1);
    for (byte i = 0; i < pass_ap.length(); i++)
      EEPROM.write(i + 32, buf[i]);

    if (telegramid_ap != "") {
      telegramid_ap.getBytes(buf, telegramid_ap.length() + 1);
      for (byte i = 0; i < telegramid_ap.length(); i++)
        EEPROM.write(i + 64, buf[i]);
    }

    EEPROM.commit();
    EEPROM.end();

    str += "Configuration saved in FLASH</br>\
   Changes applied after reboot</p></br></br>\
   <a href=\"/\">Перезагрузите</a> устройство</br>";

  }
  else {
    str += "Не получилось подключиться</br>\
   <a href=\"/\">Перезагрузите</a> to устройство</br>";
  }
  str += "</body></html>";
  server.send ( 200, "text/html", str );
}
//=============конец блока=============

void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);
  alarm();
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id2 = String(bot.messages[i].chat_id);
    if (chat_id2 != chat_id) {
      //bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;
    if (text == "/help") {
      String welcome = "Здравствуй , " + from_name + "\n";
      welcome += "Для использования бота можно использовать:\n";
      welcome += "/on : поставить на сигнализацию\n";
      welcome += "/off : снять с сигнализации\n";
      welcome += "/photo : сделать фотографию\n";
      welcome += "при при подключении прибор 5 раз моргнет\n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/on") {
      flag = 1;
      Serial.println("on");
      String welcome = "Сигнализация включится через 30 секунд";
      //delay(30000);
      bot.sendMessage(chat_id, welcome, "");
      time2 = millis() + time3;
    }

    if (text == "/off") {
      flag = 0;
      Serial.println("off");
      String welcome = "Сигнализация выключена";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/photo") {
      flag = 0;
      Serial.println("photo");
      String welcome = "Фотография: ";
      bot.sendMessage(chat_id, welcome, "");
      sendCapturedImage2Telegram(token, chat_id);                 // Если обнаружено движение
    }
  }
}

void setup()
{
  pinMode(ldrPin, INPUT);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Serial.begin(115200);
  delay(1000);

  //WiFi.mode(WIFI_STA);

  //==============wifi=================
  byte len_ssid, len_pass, len_telegramid;
  EEPROM.begin(98);
  len_ssid = EEPROM.read(96);
  len_pass = EEPROM.read(97);
  len_telegramid = EEPROM.read(95);
  if (len_pass > 64) len_pass = 0;
  if ((len_ssid < 33) && (len_ssid != 0)) {
    // Режим STATION
    WiFi.mode( WIFI_STA);

    unsigned char* buf_ssid = new unsigned char[32];
    unsigned char* buf_pass = new unsigned char[64];
    unsigned char* buf_telegramid = new unsigned char[96];

    for (byte i = 0; i < len_ssid; i++) buf_ssid[i] = char(EEPROM.read(i));
    buf_ssid[len_ssid] = '\x0';
    const char *ssid  = (const char*)buf_ssid;

    for (byte i = 0; i < len_pass; i++) buf_pass[i] = char(EEPROM.read(i + 32));
    const char *pass  = (const char*)buf_pass;
    buf_pass[len_pass] = '\x0';

    for (byte i = 0; i < len_telegramid; i++) buf_telegramid[i] = char(EEPROM.read(i + 64));
    const char *telegramid  = (const char*)buf_telegramid;
    buf_telegramid[len_telegramid] = '\x0';


    delay(2000);
    Serial.print("SSID: ");
    Serial.print(ssid);
    Serial.print("   ");
    Serial.print("Password: ");
    Serial.print(pass);
    Serial.print("telegramid: ");
    Serial.println(telegramid);
    chat_id = telegramid;

    WiFi.begin(ssid, pass);
    // Wait for connection
    while ( WiFi.status() != WL_CONNECTED ) {
      delay ( 500 );
      Serial.print ( "." );
      if (millis() > 20000) {
        const char *ssid_ap = "AlarmSystemBot";
        WiFi.mode(WIFI_AP);
        Serial.print("Configuring access point...");
        /* You can remove the password parameter if you want the AP to be open. */
        WiFi.softAP(ssid_ap);
        delay(2000);
        Serial.println("done");
        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);
        server.on("/", handleRoot);
        server.on("/ok", handleOk);
        server.begin();
        Serial.println("HTTP server started");
        break;
      }
    }
    Serial.println();
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else // Режим SoftAP
  {
    const char *ssid_ap = "AlarmSystemBot";
    WiFi.mode(WIFI_AP);
    Serial.print("Configuring access point...");
    /* You can remove the password parameter if you want the AP to be open. */
    WiFi.softAP(ssid_ap);
    delay(2000);
    Serial.println("done");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    server.on("/", handleRoot);
    server.on("/ok", handleOk);
    server.begin();
    Serial.println("HTTP server started");
  }
  //===================================
  //
  //  Serial.println("");
  //  Serial.print("Соединяемся с WIFI ");
  //  Serial.println(ssid);
  //WiFi.begin(ssid, password);

  long int StartTime = millis();
  //  while (WiFi.status() != WL_CONNECTED)            // Ждём пока не соединится
  //  {
  //    delay(500);
  //    if ((StartTime+10000) < millis()) break;
  //  }

  Serial.println("");
  Serial.println("Ваш IP адрес: ");
  Serial.println(WiFi.localIP());

  Serial.println("");

  //  if (WiFi.status() != WL_CONNECTED) {
  //    Serial.println("Сброс");
  //
  //    ledcAttachPin(4, 3);
  //    ledcSetup(3, 5000, 8);
  //    ledcWrite(3,10);
  //    delay(200);
  //    ledcWrite(3,0);
  //    delay(200);
  //    ledcDetachPin(3);
  //
  //    delay(1000);
  //    ESP.restart();
  //  }
  if (WiFi.status() == WL_CONNECTED) {
    ledcAttachPin(4, 3);
    ledcSetup(3, 5000, 8);
    for (int i = 0; i < 5; i++) {                   //При подключении к Wi-Fi сети светодиод мигнёт 5 раз
      ledcWrite(3, 10);
      delay(200);
      ledcWrite(3, 0);
      delay(200);
    }
    ledcDetachPin(3);
  }
  // Конфигурациия камеры
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // Если есть psram
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;     // 1600x1200
    config.jpeg_quality = 10;               // Качество сохранения файла от 0 до 63, чем ниже тем лучше
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;           // 800x600
    config.jpeg_quality = 12;                     // Качество сохранения файла от 0 до 63, чем ниже тем лучше
    config.fb_count = 1;
  }

  // Инициализация камеры
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Ошибка инициализации камеры 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  //Выставляем размер кадра для быстрой смены кадров
  sensor_t * s = esp_camera_sensor_get();
  //Разрешение экрана
  s->set_framesize(s, FRAMESIZE_CIF);               // 352 x 288

  // Отправка Токена и ID
  //sendCapturedImage2Telegram(token, chat_id);
  Serial.println();
  String keyboardJson = "[[\"/on\", \"/off\", \"/photo\"],[\"/help\"]]";
  bot.sendMessageWithReplyKeyboard(chat_id, "Выбери один из вариантов", "", keyboardJson, true);
  pinMode(pinPIR, INPUT_PULLUP);  // Объявляем входом и Включаем внутреннюю подтяжку
}

void loop()
{
  server.handleClient();


  //принимаем сообщение
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    alarm();
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      alarm();
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  alarm();
}

int alarm() {
  int v = digitalRead(pinPIR);
  Serial.println(v);
  if (v == 1 && flag == 1 && time2 < millis()) {
    //time2 = millis();
    sendCapturedImage2Telegram(token, chat_id);                 // Если обнаружено движение
    delay(10000);
  }
}

String sendCapturedImage2Telegram(String token, String chat_id) {
  const char* myDomain = "api.telegram.org";
  String getAll = "", getBody = "";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Сбой захвата камеры");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }

  Serial.println("Подключиться к  " + String(myDomain));
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();   //run version 1.0.5 or above

  if (client_tcp.connect(myDomain, 443)) {
    Serial.println("Соединение прошло успешно");

    String head = "--Russia\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chat_id + "\r\n--Russia\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Russia--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;

    client_tcp.println("POST /bot" + token + "/sendPhoto HTTP/1.1");
    client_tcp.println("Host: " + String(myDomain));
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=Russia");
    client_tcp.println();
    client_tcp.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024) {
      if (n + 1024 < fbLen) {
        client_tcp.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        client_tcp.write(fbBuf, remainder);
      }
    }

    client_tcp.print(tail);

    esp_camera_fb_return(fb);

    int waitTime = 10000;   // Ждём 10 секунд
    long startTime = millis();
    boolean state = false;

    while ((startTime + waitTime) > millis())
    {
      Serial.print(".");
      delay(100);
      while (client_tcp.available())
      {
        char c = client_tcp.read();
        if (state == true) getBody += String(c);
        if (c == '\n')
        {
          if (getAll.length() == 0) state = true;
          getAll = "";
        }
        else if (c != '\r')
          getAll += String(c);
        startTime = millis();
      }
      if (getBody.length() > 0) break;
    }
    client_tcp.stop();
    Serial.println(getBody);
  }
  else {
    getBody = "Нет соединения с api.telegram.org.";
    Serial.println("Нет соединения с api.telegram.org.");
  }
  return getBody;
}

String sendMessage2Telegram(String token, String chat_id, String text) {
  const char* myDomain = "api.telegram.org";
  String getAll = "", getBody = "";

  Serial.println("Подключиться к " + String(myDomain));
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();   //run version 1.0.5 or above

  if (client_tcp.connect(myDomain, 443)) {
    Serial.println("Соединение прошло успешно");

    String message = "chat_id=" + chat_id + "&text=" + text;
    client_tcp.println("POST /bot" + token + "/sendMessage HTTP/1.1");
    client_tcp.println("Host: " + String(myDomain));
    client_tcp.println("Content-Length: " + String(message.length()));
    client_tcp.println("Content-Type: application/x-www-form-urlencoded");
    client_tcp.println();
    client_tcp.print(message);

    int waitTime = 10000;   // Ждём 10 секунд
    long startTime = millis();
    boolean state = false;

    while ((startTime + waitTime) > millis())
    {
      Serial.print(".");
      delay(100);
      while (client_tcp.available())
      {
        char c = client_tcp.read();
        if (state == true) getBody += String(c);
        if (c == '\n')
        {
          if (getAll.length() == 0) state = true;
          getAll = "";
        }
        else if (c != '\r')
          getAll += String(c);
        startTime = millis();
      }
      if (getBody.length() > 0) break;
    }
    client_tcp.stop();
    Serial.println(getBody);
  }
  else {
    getBody = "Нет соединения с api.telegram.org.";
    Serial.println("Нет соединения с api.telegram.org.");
  }

  return getBody;
}
