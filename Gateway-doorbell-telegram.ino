#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <ESP8266WiFi.h>
  BearSSL::WiFiClientSecure client;
  BearSSL::Session   session;
  BearSSL::X509List  certificate(telegram_cert);
#elif defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <WiFi.h>
  #include <WiFiClientSecure.h>
  WiFiClientSecure client;
#endif

#include <AsyncTelegram2.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <time.h>
#define timeZone "UTC+3"

const char* ssid = "SSID";
const char* password = "Password";
const char* token =  "TELEGRAM-BOT-TOKEN";  // Telegram token
int64_t userid = Telegram-UserID;

int buttonState = HIGH;            // the current reading from the input pin
int lastButtonState = HIGH;  // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 100;    // the debounce time; increase if the output flickers

AsyncWebServer server(80);
AsyncTelegram2 myBot(client);

#define doorBell 19
#define onOffSwitch 21

void setup(void) 
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(doorBell, INPUT_PULLUP);
  pinMode(onOffSwitch, INPUT_PULLUP);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

#ifdef ESP8266
  // Sync time with NTP, to check properly Telegram certificate
  configTime(timeZone, "time.google.com", "time.windows.com", "pool.ntp.org");
  //Set certficate, session and some other base client properies
  client.setSession(&session);
  client.setTrustAnchors(&certificate);
  client.setBufferSizes(1024, 1024);
#elif defined(ESP32)
  // Sync time with NTP
  configTzTime(timeZone, "time.google.com", "time.windows.com", "pool.ntp.org");
  client.setCACert(telegram_cert);
#endif

  // Set the Telegram bot properties
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");
  Serial.print("Bot name: @");
  Serial.println(myBot.getBotName());

  time_t now = time(nullptr);
  struct tm t = *localtime(&now);
  char welcome_msg[64];
  strftime(welcome_msg, sizeof(welcome_msg), "Bot started at %X", &t);
  myBot.sendTo(userid, welcome_msg);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
  {
    request->send(200, "text/plain", "Hi! This is a sample response.");
  });

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) 
{
  // In the meantime LED_BUILTIN will blink with a fixed frequency
  // to evaluate async and non-blocking working of library
  int reading;
  static uint32_t ledTime = millis();
  
  if (millis() - ledTime > 200)
  {
    ledTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  
  // Check incoming messages and keep Telegram server connection alive
  TBMessage msg;
  if (myBot.getNewMessage(msg)) {    
    Serial.print("User ");
    Serial.print(msg.sender.username);
    Serial.print(" send this message: ");
    Serial.println(msg.text);

    // echo the received message
    myBot.sendMessage(msg, msg.text);
  }
  
  if(digitalRead(onOffSwitch) == HIGH)
  {
    reading = digitalRead(doorBell);
    if (reading != lastButtonState) 
    {
      // reset the debouncing timer
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) 
    {
      // whatever the reading is at, it's been there for longer than the debounce
      // delay, so take it as the actual current state:
      // if the button state has changed:
      if (reading != buttonState) 
      {
        buttonState = reading;
        if (buttonState == HIGH) 
        {
          Serial.print("¡Tocaron el timbre en ...!");
          time_t now = time(nullptr);
          struct tm t = *localtime(&now);
          char msg_buf[64];
          strftime(msg_buf, sizeof(msg_buf), "%X - ¡Tocaron el timbre en ...!", &t);
          myBot.sendTo(userid, msg_buf);    
       }
      }   
    }
    lastButtonState = reading;
  }
  }
