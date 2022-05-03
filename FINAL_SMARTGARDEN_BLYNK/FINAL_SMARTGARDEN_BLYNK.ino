#define BLYNK_PRINT Serial
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
#include <EEPROM.h>
#include <DS3231_Simple.h>
DS3231_Simple Clock;
#define TdsSensorPin A1
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
char ssid [] = "Home";
char pass [] = "home9899";
//char ssid [] = "nextjack";
//char pass [] = "dwl686168";
char auth[] = "w9gKcejlGmde44KVrdu7D-NQD6kRBNGd";


// Hardware Serial on Mega, Leonardo, Micro...
#define EspSerial Serial1



// or Software Serial on Uno, Nano...
//#include <SoftwareSerial.h>
//SoftwareSerial EspSerial(2, 3); // RX, TX

// Your ESP8266 baud rate:
#define ESP8266_BAUD 115200

ESP8266 wifi(&EspSerial);

int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int resval = 0;  // holds the value
int analogBufferIndex = 0, copyIndex = 0;
int button = 8;
int nilaitombol;
int count;
int button2 = 7;
int nilaitombol2;
int count2;
int waktuTanam = 0;
int lampuUv = 13;
int pompaNutrisiAB = 12;
int pompaPupuk = 11;
int pompaAir = 10;
int pompaphdown = 9;
int relayGnd = 6;
String keadaanAir;
String modeSistem;
String sistemTanam;
const int phSensorPin  = A0;
float Po = 0;
byte value;
float averageVoltage = 0, tdsValue = 0, temperature = 25;

void setup()
{
  Serial.begin(9600);
  pinMode (phSensorPin, INPUT);
  pinMode(button, INPUT);
  pinMode(button2, INPUT);
  pinMode(TdsSensorPin, INPUT);
  pinMode(lampuUv, OUTPUT);
  pinMode(pompaNutrisiAB, OUTPUT);
  pinMode(pompaPupuk, OUTPUT);
  pinMode(pompaAir, OUTPUT);
  pinMode(pompaphdown, OUTPUT);
  pinMode(relayGnd, OUTPUT);
  digitalWrite(lampuUv, HIGH);
  digitalWrite(relayGnd, HIGH);
  Clock.begin();
  EspSerial.begin(ESP8266_BAUD);
  delay(10);

  Blynk.begin(auth, wifi, ssid, pass);
}

void loop()
{
  Blynk.run();
  // program rtc
  DateTime waktu;
  waktu = Clock.read();

  Serial.print(waktu.Day);
  Serial.print("/");
  Serial.print(waktu.Month);
  Serial.print("/");
  Serial.print(waktu.Year);
  Serial.print(" ");
  Serial.print(waktu.Hour);
  Serial.print(":");
  Serial.print(waktu.Minute);
  Serial.print(":");
  Serial.println(waktu.Second);
  int waktuDay = waktu.Day;
  int waktuHour = waktu.Hour;

  //program button waktu tanam
  nilaitombol = digitalRead(button);
  if (nilaitombol == 1) {
    count++;
    delay(1000);
    delay(1);
    if (count == 1) {
      sistemTanam = "tanaman sedang dalam pertumbuhan";
      if (waktuDay++)
      {
        value++;
        EEPROM.write(0, value);

      }
    }
    else if (count == 2) {
     sistemTanam = "tanaman telah dipanen" ;
      value = 0;
      count = 0;
    }
  }


  // program mode indoor dan outdor
  nilaitombol2 = digitalRead(button2);

  //Program water level
  resval = analogRead(A2);
  Serial.println(resval);


  //program ppm
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U)  //every 40 milliseconds,read the analog value from the ADC
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U)
  {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0); //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge = averageVoltage / compensationCoefficient; //temperature compensation
    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
    //Serial.print("voltage:");
    //Serial.print(averageVoltage,2);
    //Serial.print("V   ");

  }
  Serial.print("TDS Value:");
  Serial.print(tdsValue, 0);
  Serial.println(" ppm");
  delay(5);

  //program ph air
  int nilaiPengukuranPh = analogRead(phSensorPin);
  double TeganganPh = 5 / 1024.0 * nilaiPengukuranPh;
  //Po = 7.00 + ((teganganPh7 - TeganganPh) / PhStep);
  Po = 8.30 + ((2.6 - TeganganPh) / 0.17);
  Serial.print("Nilai PH cairan: ");
  Serial.println(Po);
  delay(100);
  //kurang pupuk
  if ((tdsValue >= 0) && (tdsValue <= 800))
  {
    digitalWrite(pompaNutrisiAB, LOW);
    digitalWrite(pompaPupuk, HIGH);

  }
  //Batas wajar kualitas pupuk
  if (tdsValue > 800)
  {
    digitalWrite(pompaPupuk, LOW);
    digitalWrite(pompaNutrisiAB, HIGH);
  }
  // air menyala saat kurang dari jarak normal
  if (resval <= 100)
  {
    keadaanAir="Tidak Penuh";
    digitalWrite(pompaAir, HIGH);
  }

  // air berhenti saat melewati batas
  if (resval > 330)
  {
    keadaanAir="Penuh";
    digitalWrite(pompaAir, LOW);
  }

  //ph air basa
  if ( Po > 6.50)
  {
    digitalWrite(pompaphdown, HIGH);
  }
  if ( Po <= 6.50)
  {
    digitalWrite(pompaphdown, LOW);
  }
  if (nilaitombol2 == 1) {

    count2++;
    delay(1000);
    if (count2 == 1)
    {
      modeSistem = "Normal Indoor";
      if ((waktuHour >= 5) && ( waktuHour <= 19))
      {
        digitalWrite(relayGnd, LOW);
        digitalWrite(lampuUv, LOW);
      }
      if ((waktuHour <= 5) || ( waktuHour >= 19))
      {
        digitalWrite(relayGnd, HIGH);
        digitalWrite(lampuUv, HIGH);
      }
    }
    if (count2 == 2)
    {
      modeSistem = "Night Indoor";
      if ((waktuHour < 5) || ( waktuHour > 19))
      {
        digitalWrite(relayGnd, LOW);
        digitalWrite(lampuUv, LOW);
      }
      if ((waktuHour > 5) && ( waktuHour < 19))
      {
        digitalWrite(relayGnd, HIGH);
        digitalWrite(lampuUv, HIGH);
      }
    }
  }
  if (count2 == 3)
  {
    modeSistem = "Outdoor ";
    if ((waktuHour >= 5) && ( waktuHour <= 19))
    {
      digitalWrite(relayGnd, LOW);
      digitalWrite(lampuUv, HIGH);
    }
    if ((waktuHour <= 5) || ( waktuHour >= 19))
    {
      digitalWrite(relayGnd, HIGH);
      digitalWrite(lampuUv, HIGH);
    }
    count2 = 0;
  }

  Serial.println(modeSistem);
  waktuTanam = EEPROM.read(0);
  Serial.print("waktu tanam =" );
  Serial.print(waktuTanam);
  Serial.print(" hari");
  Serial.println (" ");


}
int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
BLYNK_READ(V0) //Blynk app has something on V0
{
  Blynk.virtualWrite(V0, waktuTanam); //sending to Blynk
}
BLYNK_READ(V1) //Blynk app has something on V1
{
  Blynk.virtualWrite(V1, Po); //sending to Blynk
}
BLYNK_READ(V2) //Blynk app has something on V2
{
  Blynk.virtualWrite(V2, tdsValue); //sending to Blynk
}
BLYNK_READ(V3) //Blynk app has something on V3
{
  Blynk.virtualWrite(V3, keadaanAir); //sending to Blynk
}
BLYNK_READ(V4) //Blynk app has something on V4
{
  Blynk.virtualWrite(V4, modeSistem); //sending to Blynk
}
BLYNK_READ(V5) //Blynk app has something on V5
{
  Blynk.virtualWrite(V5, sistemTanam); //sending to Blynk
}

