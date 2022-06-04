#include <Arduino.h>
#include <MPU6050_tockn.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "BluetoothSerial.h"

#define SDA 21
#define SCL 22
#define OLED_RESET 4

#define BUFFERLEN 128

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

MPU6050 mpu6050(Wire);
MAX30105 particleSensor;
long timer = 0, loctime;
float AngleX, AngleY, AngleZ;
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];    //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute = 60;
int beatAvg = 60;
int COUNT = 0;

long irValue;

long MPU_starttime, MPU_endtime;
double MPU_fps = 30.0;
float MPU_DATA[6][BUFFERLEN];

long MAX_starttime, MAX_endtime;
double MAX_fps = 30.0;
uint32_t MAX_IR_DATA[BUFFERLEN];

long Press_starttime, Press_endtime;
double Press_fps = 30.0;
uint16_t Press_DATA[BUFFERLEN];

BluetoothSerial SerialBT;

String bluetooth_in_buffer = "";

void Draw_Init_Interface(void)
{
    for (size_t i = 0; i < 46; i = i + 5)
    {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(25, 20);
        display.println("Initialize...");
        display.drawRect(38, 38, 51, 6, WHITE);      //以（38，38）为起点绘制一个长度51宽度为6的矩形
        display.drawLine(40, 40, 40 + i, 40, WHITE); //循环绘制线条达到显示进度的效果
        display.drawLine(40, 41, 40 + i, 41, WHITE);
        display.display();
        delay(100);
        display.clearDisplay();
    }
    display.display();
}

void setup()
{
    Wire.begin(SDA, SCL);
    Serial.begin(115200);
    SerialBT.begin("SmartD");

    mpu6050.begin();
    mpu6050.calcGyroOffsets(true);

    if (particleSensor.begin(Wire, I2C_SPEED_FAST) == false) //Use default I2C port, 400kHz speed
    {
        Serial.println("MAX30105 was not found. Please check wiring/power. ");
    }

    //Setup to sense up to 18 inches, max LED brightness
    byte ledBrightness = 0xFF; //Options: 0=Off to 255=50mA
    byte sampleAverage = 1;    //Options: 1, 2, 4, 8, 16, 32
    byte ledMode = 3;          //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
    int sampleRate = 50;       //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
    int pulseWidth = 69;       //Options: 69, 118, 215, 411
    int adcRange = 2048;       //Options: 2048, 4096, 8192, 16384

    //particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
    particleSensor.setup();
    particleSensor.setPulseAmplitudeIR(0x24);

    //Pre-populate the plotter so that the Y scale is close to IR values
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3D (for the 128x64)
    //display.display();                         // 屏幕显示（显示内容为预设画面）
    display.clearDisplay(); //清理屏幕和缓存区（I2C只清理缓存区）
    Draw_Init_Interface();
}

void loop()
{
    int i, j;
    MPU_starttime = millis();
    for (i = 0; i < BUFFERLEN; i++)
    {
        display.clearDisplay();
        display.setTextSize(1);  //设置字体大小
        display.setCursor(0, 5); //设置显示位置
        mpu6050.update();
        MPU_DATA[0][i] = mpu6050.getAngleX();
        MPU_DATA[1][i] = mpu6050.getAngleY();
        MPU_DATA[2][i] = mpu6050.getAngleZ();
        MPU_DATA[3][i] = mpu6050.getAccX();
        MPU_DATA[4][i] = mpu6050.getAccY();
        MPU_DATA[5][i] = mpu6050.getAccZ();
        display.print("X: ");
        display.print(MPU_DATA[0][i]);
        display.print("/");
        display.println(MPU_DATA[3][i]);
        display.print("Y: ");
        display.print(MPU_DATA[1][i]);
        display.print("/");
        display.println(MPU_DATA[4][i]);
        display.print("Z: ");
        display.print(MPU_DATA[2][i]);
        display.print("/");
        display.println(MPU_DATA[5][i]);
        //display.print("fps = ");
        //display.println(MPU_fps);
        display.display();
        Serial.print("X: ");
        Serial.print(MPU_DATA[0][i]);
        Serial.print("/");
        Serial.println(MPU_DATA[3][i]);
        Serial.print("Y: ");
        Serial.print(MPU_DATA[1][i]);
        Serial.print("/");
        Serial.println(MPU_DATA[4][i]);
        Serial.print("Z: ");
        Serial.print(MPU_DATA[2][i]);
        Serial.print("/");
        Serial.println(MPU_DATA[5][i]);
        Serial.print("fps = ");
        Serial.println(MPU_fps);
        display.display();
    }
    MPU_endtime = millis();
    MPU_fps = BUFFERLEN * 1000.0 / (MPU_endtime - MPU_starttime);

    SerialBT.print("MPU ");
    SerialBT.println(MPU_fps);
    for (i = 0; i < BUFFERLEN; i++)
    {
        for (j = 0; j < 6; j++)
        {
            SerialBT.print(MPU_DATA[j][i]);
            SerialBT.print(" ");
        }
        SerialBT.println();
    }
    delay(500);

    MAX_starttime = millis();
    for (i = 0; i < BUFFERLEN; i++) //do we have new data?
    {
        MAX_IR_DATA[i] = particleSensor.getIR();
        display.clearDisplay();
        display.setTextSize(1);  //设置字体大小
        display.setCursor(0, 5); //设置显示位置

        display.print("IR[");
        display.print(MAX_IR_DATA[i]);
        display.println("]");
        display.display();
        Serial.print("IR[");
        Serial.print(MAX_IR_DATA[i]);
        Serial.println("]");

        //Serial.print("Temp[");
        //Serial.print(particleSensor.readTemperature());
        //Serial.print("]");
        //Serial.println();
        //Serial.print("fps = ");
        //Serial.println(MAX_fps);
    }
    MAX_endtime = millis();
    MAX_fps = BUFFERLEN * 1000.0 / (MAX_endtime - MAX_starttime);

    SerialBT.print("MAX ");
    SerialBT.print(MAX_fps);
    for (i = 0; i < BUFFERLEN; i++)
    {
        SerialBT.print(MAX_IR_DATA[i]);
        SerialBT.println();
    }
    delay(500);

    Press_starttime = millis();
    for (i = 0; i < BUFFERLEN; i++)
    {
        Press_DATA[i] = analogRead(4);

        display.clearDisplay();
        display.setTextSize(1);  //设置字体大小
        display.setCursor(0, 5); //设置显示位置
        display.print("Press[");
        display.print(Press_DATA[i]);
        display.println("]");
        display.display();

        Serial.print("Press[");
        Serial.print(Press_DATA[i]);
        Serial.println("]");
    }
    Press_endtime = millis();
    Press_fps = BUFFERLEN * 1000.0 / (Press_endtime - Press_starttime);

    SerialBT.print("Press ");
    SerialBT.println(Press_fps);
    for (i = 0; i < BUFFERLEN; i++)
    {
        SerialBT.print(Press_DATA[i]);
        SerialBT.println();
    }
    //if (SerialBT.available)
    /*

    
    Serial.print("\tangleX : ");
    Serial.print(mpu6050.getAngleX());
    Serial.print("\tangleY : ");
    Serial.print(mpu6050.getAngleY());
    Serial.print("\tangleZ : ");
    Serial.println(mpu6050.getAngleZ());
    */
    /*
    if (checkForBeat(irValue) == true)
    {
        //We sensed a beat!
        Serial.println("Beating");
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute < 255 && beatsPerMinute > 20)
        {
            rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
            rateSpot %= RATE_SIZE;                    //Wrap variable

            //Take average of readings
            beatAvg = 0;
            for (byte x = 0; x < RATE_SIZE; x++)
                beatAvg += rates[x];
            beatAvg /= RATE_SIZE;
        }
    }
    */

    //Serial.println(irValue);
    //display.print("BPM=");
    //display.println(beatsPerMinute);
    //display.print("Avg BPM=");
    //display.println(beatAvg);
}