#include "Arduino.h"
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include "time.h"

const char* ssid     = "xxxxxxxxxx";
const char* password = "xxxxxxxxxx";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = (3600 * 5) + (3600/2);
const int   daylightOffset_sec = 3600;

// Define the display configuration for ESP AMOLED Display S3
#define GFX_DEV_DEVICE LILYGO_T_DISPLAY_S3_AMOLED

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    6 /* cs */, 47 /* sck */, 18 /* d0 */, 7 /* d1 */, 48 /* d2 */, 5 /* d3 */);
Arduino_GFX *gfx = new Arduino_RM67162(bus, 17 /* RST */, 0 /* rotation */);

int values[24] = {0};
int values2[24] = {0};
int values_old[24] = {0};
char timeHour[3] = "00";
char timeMin[3] = "00";
char timeSec[3];

char m[12];
char y[5];
char d[3];
char dw[12];

int gw = 536;
int gy = 210;//144;

int gx = 240;
int gh = 140;


int curent = 0;

#define gray 0x6B6D
#define blue 0x0967
#define orange 0xC260
#define purple 0x604D
#define green 0x1AE9

int deb = 0;
int Mode = 1;

#define RIGHT 21

#define IR_LED          46
#define IR_PHOTOLED     15

uint16_t IR_FB_LED_Off  =   0;
uint16_t IR_FB_LED_On   =   0;

uint8_t  fLED_State     =   0;

void setup(void) 
{
    static uint8_t timeout = 0;

    pinMode(RIGHT, INPUT_PULLUP);

    pinMode(38, OUTPUT);
    digitalWrite(38, 1);

    pinMode(IR_LED,      OUTPUT);
    pinMode(IR_PHOTOLED, INPUT);

    Serial.begin(115200);
    gfx->begin();
    gfx->fillScreen(BLACK);
    gfx->setRotation(1);

    values[23] = gh / 2;
    analogReadResolution(10);

    //Set Brightness
    bus->beginWrite();
    bus->writeC8D8(0x51, 255);
    bus->endWrite();    

    gfx->fillRoundRect(6, 5, 76, 64, 4, green);     //hours
    gfx->fillRoundRect(104, 5, 76, 64, 4, green);   //mins
    gfx->fillRoundRect(6, 80, 176, 24, 4, green);   //date


    gfx->fillRoundRect(6, 110, 176, 24, 4, purple); //fps
    gfx->fillRoundRect(6, 140, 176, 120, 4, purple); //Details

    //Draw X Legend and y Grid lines
    for (int i = 1; i < 12; i++) //12 lines
    {
        //            x1               y1  x2               y2
        gfx->drawLine(gx + (i * 34)-5, gy, gx + (i * 34)-5, gy - gh, gray);
    
        if (i * 17 % 34 == 0)
        {
            if (i * 2 < 10)
            {
                gfx->setTextSize(2 /* x scale */, 2 /* y scale */, 1 /* pixel_margin */);
                gfx->setCursor( gx + (i * 34) - 12, gy + 8);
                gfx->println("0" + String(i * 2));
            }
            else
            {
                gfx->setTextSize(2 /* x scale */, 2 /* y scale */, 1 /* pixel_margin */);
                gfx->setCursor( gx + (i * 34) - 12, gy + 8);
                if( (i * 2) <= 16)
                {
                    gfx->println(String(i * 2));
                }
            }
        }
    }    

    //Draw Y Legend and X Grid lines
    for (int i = 1; i < 6; i++) 
    {
        gfx->drawLine(gx, gy - (i * 26)+3, gx + gw, gy - (i * 26)+3, gray);

        gfx->setCursor(gx - 48, gy - (i * 26) - 5);

        if (i * 26 < 100)
        {
            gfx->println(" " + String(i * 26));
        }
        else
        {
            gfx->println(String(i * 26));
        }
    }

    //Draw Origin
    gfx->drawLine(gx, gy, gx + gw, gy, WHITE);
    gfx->drawLine(gx, gy, gx, gy - gh, WHITE);

   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) 
   {
    delay(500);
    Serial.print(".");
    timeout++;
    if(timeout > 10)
        break; 
   }

   Serial.println("");
   Serial.println("WiFi connected.");
   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);   
}

int Min = gh / 2;
int Max = gh / 2;
int average = 0;

long lastMillis = 0;
int fps = 0;

int counter=0;

void loop() 
{

    if (digitalRead(RIGHT) == 0) 
    {
        if (deb == 0) 
        {
            Mode++;
            if (Mode == 2) 
            {
                Mode = 0;
            }
            deb = 1;
        }
    } 
    else 
    {
        deb = 0;
    }

    fps = 1000 / (millis() - lastMillis);
    lastMillis = millis();

  
    average=0;
    if(counter==0)
    {
        printLocalTime();
    }

    counter++;
    if(counter==50)
    {
        counter=0;
    }

    if (Mode == 0) 
    {
        if (values[23] > 12)
            curent = random(values[23] - 12, values[23] + 12);
        else
            curent = random(1, values[23] + 14);

        if (curent > gh) curent = random(gh - 10, gh);
    }
    if (Mode == 1) {
        
        uint16_t num = get_IR_Sensor();
        
        curent = map(num, 0, 1024, 0, gh);
    }

    for (int i = 0; i < 24; i++) 
    {
        values2[i] = values[i];
    }

    for (int i = 23; i > 0; i--) 
    {   
        values[i - 1] = values2[i];
    }

    values[23] = curent;

    if (values[23] > Max) 
    {
    }

    if (values[23] < Min) 
    {
        Min = values[23];
    }

    for (int i = 0; i < 24; i++) 
    {
        average = average + values[i];
    }

    average = average / 24;

    //gfx->flush();

    //gfx->fillScreen(BLACK);

    //Details    
    gfx->setTextColor(WHITE, purple);
    gfx->setTextSize(3 /* x scale */, 3 /* y scale */, 1 /* pixel_margin */);
    gfx->setCursor(10, 150);
    gfx->println("CURR: " + String(average));
    gfx->setCursor(10, 180);
    gfx->println("MIN:  "  + String(Min));
    gfx->setCursor(10, 210);
    gfx->println("MAX:  "  + String(Max));

    //FPS
    gfx->setTextColor(WHITE, purple);
    gfx->setTextSize(2 /* x scale */, 2 /* y scale */, 1 /* pixel_margin */);
    gfx->setCursor(10, 116);
    gfx->println("SPEED:" + String(fps) + " fps");
    

    //time and date
    gfx->setTextColor(WHITE, green);
    gfx->setTextSize(4 /* x scale */, 4 /* y scale */, 0 /* pixel_margin */);
    gfx->setCursor(20, 24);
    gfx->println(String(timeHour));
    gfx->setCursor(120,24);
    gfx->println(String(timeMin));

    gfx->setTextSize(3 /* x scale */, 3 /* y scale */, 0 /* pixel_margin */);
    gfx->setCursor(gx-40,14);
    gfx->println(String(timeSec));

    gfx->setTextSize(2 /* x scale */, 2 /* y scale */, 1 /* pixel_margin */);
    gfx->setCursor(14,84);
    gfx->println(String(m)+" "+String(d));    

    //Graph heading
    gfx->setTextColor(YELLOW, BLACK);
    gfx->setTextSize(2 /* x scale */, 2 /* y scale */, 1 /* pixel_margin */);
    gfx->setCursor( gx + 10, 16);
    gfx->println("ANALOG READINGS");
    gfx->setCursor( gx + 10, 46);
    if (Mode == 0) 
        gfx->println("RANDOM   ");
    else
        gfx->println("ON PIN 44");




    //Clear Last Update
    for (int i = 0; i < 23; i++) 
    {
         gfx->drawLine(gx + (i * 17), gy - values_old[i], gx + ((i + 1) * 17), gy - values_old[i + 1], BLACK);
         gfx->drawLine(gx + (i * 17), gy - values_old[i] - 1, gx + ((i + 1) * 17), gy - values_old[i + 1] - 1, BLACK);
    }

    //Redraw just the grid
    //Draw X Legend and y Grid lines
    for (int i = 1; i < 12; i++) //12 lines
    {
        //            x1               y1  x2               y2
        gfx->drawLine(gx + (i * 34)-5, gy, gx + (i * 34)-5, gy - gh, gray);
    }    
    //Draw Y Legend and X Grid lines
    for (int i = 1; i < 6; i++) 
    {
        gfx->drawLine(gx, gy - (i * 26)+3, gx + gw, gy - (i * 26)+3, gray);
    }    
    //Draw Origin
    gfx->drawLine(gx, gy, gx + gw, gy, WHITE);
    gfx->drawLine(gx, gy, gx, gy - gh, WHITE);

    //Draw new value
    for (int i = 0; i < 23; i++) 
    {
         gfx->drawLine(gx + (i * 17), gy - values[i], gx + ((i + 1) * 17), gy - values[i + 1], RED);
         gfx->drawLine(gx + (i * 17), gy - values[i] - 1, gx + ((i + 1) * 17), gy - values[i + 1] - 1, RED);
    }

    //Store New values
    memcpy(values_old, values, sizeof(values));

    gfx->setTextColor(WHITE, BLACK);

    gfx->setCursor(gx + 210, 16);
    gfx->println("BAT:" + String(analogRead(4)));

    gfx->setCursor(gx + 210, 46);
    gfx->println("MOD:" + String(Mode));

}


void printLocalTime()
{
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }
  
    strftime(timeHour,3, "%H", &timeinfo);
    strftime(timeMin,3, "%M", &timeinfo);
    strftime(timeSec,3, "%S", &timeinfo);
    strftime(y,5, "%Y", &timeinfo);
    strftime(m,12, "%B", &timeinfo);

    strftime(dw,10, "%A", &timeinfo);
    strftime(d,3, "%d", &timeinfo);

}


uint16_t get_IR_Sensor(void)
{
    static uint16_t wRetValue = 0;

    if(fLED_State == 0)
    {
        IR_FB_LED_Off = analogRead(IR_PHOTOLED);
        fLED_State = 1;
        digitalWrite(IR_LED, fLED_State);
    }   
    else
    {
        IR_FB_LED_On = analogRead(IR_PHOTOLED);
        fLED_State = 0;
        digitalWrite(IR_LED, fLED_State);

        //readout
        wRetValue = IR_FB_LED_On - IR_FB_LED_Off;
    }

    return wRetValue;
}