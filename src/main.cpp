#include <Arduino.h>
#include <®.h>
#include <Adafruit_NeoPixel.h>
#include <secrets.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

//
// Simple way
// Run all the things on power up, then deep sleep.
// Button push triggers reset pin
//
// Better way (OR NOT)
// Power hooked up to esp, but not ch_pd/ch_en/en enable pin (somehow)
// Button push triggers enable pin (poss 0/2 also 'for normal boot'?)
// HIGH gpio also hooked to enable pin to keep it alive
// Set low at end, boom off.
//
// NOW: This is only better if the power use for a non-enabled esp is less than deep sleep
// UPDATE: Apparently it's exactly the heckin' same
// https://bbs.espressif.com/viewtopic.php?t=8598#p18557

#define CHAKRAM_DIN 2 // Only D pins 3,5,6,9,10,11 support analogWrite
#define CHAKRAM_LEDS 60
// If colours are wrong, it'll be NEO_RGBW, or any other of the mad fugue of options in the library
Adafruit_NeoPixel chakram = Adafruit_NeoPixel(CHAKRAM_LEDS, CHAKRAM_DIN, NEO_GRBW + NEO_KHZ800);

bool debugging = true;

void chakramFill(int r, int g, int b, int w)
{
  for (uint16_t i = 0; i < CHAKRAM_LEDS; i++)
  {
    chakram.setPixelColor(i, r, g, b, w);
  }
  chakram.show();
}

void setup()
{
  Serial.begin(115200);
  delay(4000); // Always smart to delay, for microcontroller uploadery reasons
  Serial.println("Let's do this!");

  chakram.setBrightness(100); // 0->255
  chakram.begin();

  // Set the ESP8266 to be only WiFi-client, not AP as by default it does both
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int w = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    // Might as well spaz around at this point while waiting
    // chakramFill(random(0, 16), 0, 5, 0);
    chakramFill(constrain(w * 5, 0, 100), 0, 0, 0);
    w++;
  }

  Serial.print("Connected as ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  WiFiClient client;
  if (!client.connect(host, 80))
  {
    Serial.println("Error: Can't connect to API edpoint");
    chakramFill(255, 255, 0, 0);
    delay(5000);
    return;
  }

  Serial.print("Requesting URL: ");
  Serial.print(host);
  Serial.println(apiEndpoint);

  client.print(String("GET ") + apiEndpoint +
               " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  // Wait up to 10 seconds for response
  int c = 0;
  while ((!client.available()) && (c < 100))
  {
    delay(100);
    // Might as well spaz around at this point while waiting
    // chakramFill(0, 5, random(0, 16), 0);
    chakramFill(5, 0, constrain(c * 3, 0, 100), 0);
    c++;
  }

  String result = "";
  while (client.available())
  {
    char ch = static_cast<char>(client.read());
    // Serial.print(ch);
    result += ch;
  }
  client.stop();

  int lastLineIndex = result.lastIndexOf('\n');
  int daysTilHoliday = result.substring(lastLineIndex).toInt();
  Serial.print("daysTilHoliday: ");
  Serial.println(daysTilHoliday);

  // Runs anti-clockwise actually (unlike neopixels one)
  for (int i = CHAKRAM_LEDS - 1; i >= 0; i--)
  {
    // if (i >= CHAKRAM_LEDS - daysTilHoliday)
    if (i < daysTilHoliday)
    {
      chakram.setPixelColor(i, 255, 0, 20, 0);
    }
    else
    {
      chakram.setPixelColor(i, 12, 20, 0, 0);
    }
    delay(20);
    chakram.show();
  }

  // // Chill
  // delay(3000);

  // // Fade out hard
  // for (int i = 100; i >= 0; i--)
  // {
  //   chakram.setBrightness(round(i * 1.5));
  //   chakram.show();
  //   delay(10);
  // }

  // // Off all the things
  // chakramFill(0, 0, 0, 0);
  // ESP.deepSleep(0);

  // We're looping now, wall-mount style so just cycle every hour
  delay(3600000);
}
