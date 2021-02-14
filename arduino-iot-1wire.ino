#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

//// BME280

#include <Wire.h>
#include <SPI.h>

unsigned long delayTime;
////////////

ESP8266WiFiMulti WiFiMulti;

String Device_Name = "Radio LED FM";

String inputString = "";     // a String to hold incoming data
bool stringComplete = false; // whether the string is complete

int addrAP = 0;
int addrPASS = 256;

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN 5

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 20

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

//                  _                 _                _____ _
//      /\         (_)               | |              / ____| |
//     /  \   _ __  _ _ __ ___   __ _| |_ ___  _ __  | |    | | __ _ ___ ___
//    / /\ \ | '_ \| | '_ ` _ \ / _` | __/ _ \| '__| | |    | |/ _` / __/ __|
//   / ____ \| | | | | | | | | | (_| | || (_) | |    | |____| | (_| \__ \__ \
//  /_/    \_\_| |_|_|_| |_| |_|\__,_|\__\___/|_|     \_____|_|\__,_|___/___/

class Animator
{
  int startingTime;
  int runningTime;
  int frame;
  int oldFrame;
  int mode;
  int ledCount;
  int tempo;
  float framerate;
  float segments[6];

  // chase
  float pixels[100];
  int c1r, c1g, c1b;
  int c2r, c2g, c2b;

public:
  Animator(int currentTime, int _ledCount)
  {
    ledCount = _ledCount;
    startingTime = currentTime;
    mode = 0;
    tempo = 120;
    oldFrame = 0;
    framerate = 100;
  }

  void update(int currentTime)
  {
    runningTime = abs(currentTime - startingTime);
    frame = floor(runningTime / (1000.0 / framerate));
    bool UPDATE = frame != oldFrame;
    // Serial.printf("frame: %d\n", frame);
    switch (mode)
    {
      // two color chase

    case 5:
      // violet
      if (UPDATE)
      {
        c1r = 255;
        c1g = 0;
        c1b = 100;
        // yellow
        c2r = 255;
        c2g = 100;
        c2b = 0;
        //

        int middle = int(floor(ledCount / 2.0));
        int cr = int(floor(float(frame) / framerate * tempo / 12)) % middle;
        //
        for (int i = 0; i < middle; i++)
        {
          if (i > cr)
          {
            strip.setPixelColor(middle - i, strip.Color(c1g, c1r, c1b));
            strip.setPixelColor(middle + i, strip.Color(c1g, c1r, c1b));
          }
          else
          {
            strip.setPixelColor(middle - i, strip.Color(c2g, c2r, c2b));
            strip.setPixelColor(middle + i, strip.Color(c2g, c2r, c2b));
          }
        }
        strip.show();
      }
      break;
    // KNIGHT RIDER
    case 6:
      if (UPDATE)

      {

        int cr = int(floor(float(frame) / framerate * tempo / 12)) % 10;
        int cursor = cr > 5 ? 10 - cr : cr;
        //Serial.printf("kr %d, %d:%d\n", frame, cr, cursor);
        for (int i = 0; i < 6; i++)
        {
          if (i == cursor)
          {
            segments[i] = 1.0;
          }
          else
          {
            segments[i] = segments[i] * (1.0 - (float(tempo) / 60.0) / framerate);
          }
        }
        for (int i = 0; i < 6; i++)
        {
          for (int j = 0; j < KRSegmentLedCount; j++)
          {
            strip.setPixelColor(
                KROffset + i * KRSegmentLedCount + j,
                strip.Color(0,
                            round((j == (KRMiddle) ? (255.0) : (100.0)) * segments[int(floor(i))]), 0));
          }
        }
      }

      strip.show();
      break;

    // POLICE
    case 7:
      if (UPDATE)
      {

        bool period1 = (frame % int(framerate / 3)) < framerate / 6;
        bool period2 = (frame % int(framerate / 4)) > framerate / 8;
        bool period3 = (frame % int(framerate / 6)) < framerate / 12;
        bool strobe = (frame % 33) < 4;

        for (int i = 0; i < PLedCount; i++)
        {
          if (!strobe)
          {

            if (i < PMiddle)
            {
              strip.setPixelColor(
                  i,
                  strip.Color(0, int(period2 & period3 ? 255 : 0), int(period2 ? 0 : 255)));
            }
            else
            {
              if (period1)
              {
                strip.setPixelColor(
                    i,
                    strip.Color(0, int(period2 & period3 ? 0 : 255), int(period2 ? 255 : 0)));
              }
              else
              {
                strip.setPixelColor(
                    i,
                    strip.Color(0, int(period2 ? 255 : 0), int(period2 ? 0 : 255)));
              }
            }
          }
          else
          {
            strip.setPixelColor(
                i,
                strip.Color(255, 255, 255));
          }
        }
        strip.show();
      }
      break;
    }
    oldFrame = frame;
  }

public:
  void changeMode(int newMode)
  {
    switch (newMode)
    {
    case 6:
      KnightRider();
      break;
    case 7:
      Police();
      break;
    }
    mode = newMode;
    strip.fill((0, 0, 0));
    strip.show();
  }

public:
  void changeTempo(int newTempo)
  {
    tempo = newTempo;
  }
  // ilość diod na jeden segment
  int KRLedCount;
  int KROffset;
  int KRMiddle;
  int KRSegmentLedCount;
  void KnightRider()
  {
    mode = 6;
    KRLedCount = floor(float(ledCount) / 6.0) * 6;
    KROffset = int((ledCount - KRLedCount) / 2.0);
    KRSegmentLedCount = KRLedCount / 6;
    KRMiddle = (floor(float(KRSegmentLedCount) / 2.0));
  }

  int PLedCount;
  int PMiddle;
  void Police()
  {
    mode = 7;
    PLedCount = ledCount;
    PMiddle = int(round(ledCount / 2));

    Serial.printf("Changing to Police mode: %d, PLedCount: %d, PMiddle: %d", mode, PLedCount, PMiddle);
  }
};

//===============================================================================
//   / ____ \| | | | | | | | | | (_| | || (_) | |    | |____| | (_| \__ \__ \
//==================================================================================

Animator Animate(0, LED_COUNT);

void setup()
{

  EEPROM.begin(4096);
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("PA_Goscie", "superplus");
  WiFiMulti.addAP("OneCloos", "biggenerator");
  WiFiMulti.addAP("WLAN1-E39FGM", "Siwuchawcaleniegryzie");
  WiFiMulti.addAP("ojrzanet", "$$$JarokaJaro2137$$$");

  if (EEPROM.read(1023) == 1)
  {
    Serial.println("reading AP from EEPROM...");
    String sAP;
    EEPROM.get(addrAP, sAP);
    char eeAP[sAP.length()];
    sAP.toCharArray(eeAP, sAP.length());
    Serial.println(sAP);

    String sPASS;
    EEPROM.get(addrPASS, sPASS);
    char eePASS[sPASS.length()];
    sPASS.toCharArray(eePASS, sPASS.length());
    Serial.println(sPASS);

    WiFiMulti.addAP(eeAP, eePASS);
  }

  // for (uint8_t t = 4; t > 0; t--)
  // {
  //   Serial.printf("[SETUP] WAIT %d...\n", t);
  //   Serial.flush();
  //   delay(1000);
  // }
  while (not WiFiMulti.run() == WL_CONNECTED)
  {
    continue;
  }
  pinMode(LED_BUILTIN, OUTPUT);

  strip.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();             // Turn OFF all pixels ASAP
  strip.setBrightness(255); // Set BRIGHTNESS to about 1/5 (max = 255)

  //  initialize Animator class
  Animate = Animator(millis(), LED_COUNT);
  //  do the right thing
  Animate.KnightRider();
  //Animate.Police();
}

bool LED;
int oldmil = 0;
int tempoldmil = 0;

// Sterowanie paskiem led
// tryb świecenia
// 1 - statyczny
// 2 - oddychający
// 3 - RGB hue shift
// 4 - karuzela
// 6 - KNIGHT RIDER
int LED_mode;

// color statyczny

int static_red;
int static_green;
int static_blue;

void loop()
{
  int currentTime = millis();

  Animate.update(currentTime);

  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED))
  {

    WiFiClient client;

    HTTPClient http;

    //
    if (currentTime - oldmil > 10000 & false)
    {
      oldmil = currentTime;
      //Serial.print("[HTTP] begin...\n");

      String url = String("http://clooske.y0.pl/connector.php?name=" + Device_Name + "&ip=" + WiFi.localIP().toString() + "&val1=" + String(LED_mode));

      if (http.begin(client, url))
      { // HTTP

        int httpCode = http.GET();

        if (httpCode > 0)
        {

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
          {
            String payload = http.getString();
            //LED = payload.substring(payload.indexOf("val4") + 7, payload.indexOf('\n', payload.indexOf("val4"))).substring(0, 1).toInt() == 1;
            Serial.println(payload);
            //Serial.println(payload.substring(payload.indexOf("val4") + 7, payload.indexOf('\n', payload.indexOf("val4"))).substring(0, 1));
            //Serial.println(LED);
          }
        }
        else
        {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
      }
      else
      {
        Serial.printf("[HTTP} Unable to connect\n");
      }

      //// Fill along the length of the strip in various colors...
      //  colorWipe(strip.Color(255,   0,   0), 50); // Red
      //  colorWipe(strip.Color(  0, 255,   0), 50); // Green
      //  colorWipe(strip.Color(  0,   0, 255), 50); // Blue

      // Do a theater marquee effect in various colors...
      // theaterChase(strip.Color(127, 127, 127), 50); // White, half brightness
      // theaterChase(strip.Color(127, 0, 0), 50);     // Red, half brightness
      // theaterChase(strip.Color(0, 0, 127), 50);     // Blue, half brightness

      // rainbow(10);             // Flowing rainbow cycle along the whole strip
      // theaterChaseRainbow(50); // Rainbow-enhanced theaterChase variant

      //    }
    }
  }
  //// komunikacja szeregowa

  if (stringComplete)
  {
    if (inputString.substring(0, 5) == "WIFI;")
    {

      String app = inputString.substring(5, inputString.indexOf(';', 6) + 1);
      String pp = inputString.substring(inputString.indexOf(';', 6) + 1, inputString.indexOf(';', inputString.indexOf(';', 6) + 1) + 1);
      char ap[app.length()];
      app.toCharArray(ap, app.length());
      char p[pp.length()];
      pp.toCharArray(p, pp.length());
      Serial.println(String("Adding AP:\n" + String(ap) + "\np:\n" + String(p)));
      WiFiMulti.addAP(ap, p);
      for (int i = 0; i < EEPROM.length(); i++)
      {
        EEPROM.write(i, 0);
      }
      EEPROM.write(1023, 1);
      EEPROM.put(addrAP, app);
      EEPROM.put(addrPASS, pp);
      EEPROM.commit();
      WiFi.printDiag(Serial);
    }
    else if (inputString.substring(0, 5).equals("clear"))
    {
      for (int i = 0; i < EEPROM.length(); i++)
      {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      Serial.println("Clearing EEPROM...");
      WiFi.disconnect();
      ESP.reset();
      delay(1000);
    }
    else if (inputString.substring(0, 5).equals("mode;"))
    {
      Serial.println("Changing mode...");
      int newMode = inputString.substring(5, 6).toInt();
      Animate.changeMode(newMode);
    }
    else if (inputString.substring(0, 6).equals("tempo;"))
    {
      int newTempo = inputString.substring(6, inputString.indexOf(';', 7) + 1).toInt();
      Serial.printf("Tempo changed to %d\n", newTempo);
      Animate.changeTempo(newTempo);
    }
    else if (inputString.substring(0, 3).equals("br;"))
    {
      int arg1 = inputString.substring(3, inputString.indexOf(';', 4) + 1).toInt();
      Serial.printf("Brightness changed to %d\n", arg1);
      strip.setBrightness(arg1);
    }

    inputString = "";
    stringComplete = false;
  }

  // SERIAL EVENT
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n')
    {
      stringComplete = true;
      Serial.println(inputString);
    }
  }
}
// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait)
{
  for (int a = 0; a < 10; a++)
  { // Repeat 10 times...
    for (int b = 0; b < 3; b++)
    {                //  'b' counts from 0 to 2...
      strip.clear(); //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for (int c = b; c < strip.numPixels(); c += 3)
      {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait)
{
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256)
  {
    for (int i = 0; i < strip.numPixels(); i++)
    { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait)
{
  int firstPixelHue = 0; // First pixel starts at red (hue 0)
  for (int a = 0; a < 30; a++)
  { // Repeat 30 times...
    for (int b = 0; b < 3; b++)
    {                //  'b' counts from 0 to 2...
      strip.clear(); //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for (int c = b; c < strip.numPixels(); c += 3)
      {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int hue = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color);                       // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
