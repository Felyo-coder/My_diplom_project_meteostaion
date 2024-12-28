#if defined(__AVR__)
#define OPERATING_VOLTAGE   5.0
#define ZERO_SHIFT          0
#else
#define OPERATING_VOLTAGE   3.3
#define ZERO_SHIFT          1.1
#endif

// Коэффициент перевода напряжения в концентрацию pH
#define CALIBRATION_FACTOR  3.5

#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>          // библиотека для работы с шиной SPI
#include "nRF24L01.h"     // библиотека радиомодуля
#include "RF24.h"

#define soil_m_pin A0 // пин для подключения датчика влажности почвы
#define water_lvl_pin A1 // пин для подключения датчика уровня воды

#define MY_PERIOD 100  // период в мс
uint32_t tmr1;         // переменная таймера

LiquidCrystal_I2C lcd(0x27, 20, 4);

Adafruit_AHTX0 aht;

Adafruit_BMP280 bmp;

RF24 radio(9, 10); // "создать" модуль на пинах 9 и 10 Для Уно

// Назначаем пин для подключения датчика pH
constexpr auto pinSensor = A2;

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб
const uint64_t pipe00 = 0xE8E8F0F0E1LL;

void setup() {
  Serial.begin(115200);         // открываем порт для связи с ПК

  // Настройки LCD дисплея
  lcd.init();                   // Инициализируем LCD дисплей
  lcd.backlight();              // Включаем подсветку (Сделать включение подсветки по кнопке?)
  // Выводим значения, которые не будут изменяться в процессе работы, т.е. статичные надписи
  lcd.setCursor(2, 0);          // Во 2 столбце 0 строки вывести:
  lcd.print(char(223));         // Значок градуса
  lcd.print(F("C"));            // Букву С
  lcd.setCursor(0, 1);          // На первой строке вывести:
  lcd.print("Humi ");           // Humi (от англ. Humidity - влажность)
  lcd.setCursor(4, 2);          // На 6 столбце строки 2 вывести:
  lcd.print("mmHg");            // mmHg (мм.рт.ст.)
  lcd.setCursor(6, 3);
  lcd.print("pH");
  lcd.setCursor(12, 0);
  lcd.print("% water");
  lcd.setCursor(12, 1);
  lcd.print("% moist");
  
  // Инициализация NRF24L01
  radio.begin();                // активировать модуль
  radio.setAutoAck(1);          // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);      // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();     // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     // размер пакета, в байтах

  radio.openWritingPipe(pipe00);  // мы - труба 0, открываем канал для передачи данных
  radio.setChannel(0x70);             // выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_HIGH);   // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); // скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  radio.powerUp();
  radio.stopListening();

  //Инициализация AHT20
  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  delay(100);

  //Инициализация BMP280
  while ( !Serial ) delay(100);   // wait for native usb
  Serial.println(F("BMP280 test"));
  unsigned status;
  //status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  status = bmp.begin();
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                     "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(), 16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  delay(1000);
}
void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  // Считываем аналоговое значение с датчика кислотности жидкости
  int adcSensor = analogRead(pinSensor);
  // Переводим данные сенсора в напряжение
  float voltageSensor = adcSensor * OPERATING_VOLTAGE / 1023;
  // Конвертируем напряжение в концентрацию pH
  float pHSensor = CALIBRATION_FACTOR * (voltageSensor + ZERO_SHIFT);

  float temper = temp.temperature;

  float humi = humidity.relative_humidity;

  float water_lvl = map(analogRead(water_lvl_pin), 0, 1023, 0, 100);

  float soil_m = map(analogRead(soil_m_pin), 0, 1023, 0, 100);

  float massive[6] = {temper, humi, bmp.readPressure()/133.3, pHSensor, water_lvl, soil_m};
  radio.write(&massive, sizeof(massive));

  if (millis() - tmr1 >= MY_PERIOD) {  // каждые 50 мс
    tmr1 = millis();                   // перезагружаем NRF
    //(без этого данные не будут отправляться)
    radio.powerDown();
    radio.powerUp();
  }

  lcd.setCursor(0, 0);                  // На 0 строке 0 столбца вывести:
  lcd.print(int(massive[0]));           // Температуру
  lcd.setCursor(5, 1);                  // На 1 строке 5 столбца вывести:
  lcd.print(int(massive[1]));           // Влажность
  lcd.print("%");                       // Значок %
  lcd.setCursor(0, 2);                  // На 2 строке 0 столбца вывести:
  lcd.print(uint16_t(massive[2]));      // Атм. давление
  lcd.setCursor(0, 3);                  // На 3 строке вывести:
  lcd.print(massive[3]);                // уровень pH
  lcd.setCursor(9, 0);                  // На 9 столбце 0 строки вывести:
  lcd.print(uint16_t(massive[4]));      // уровень жидкости
  lcd.setCursor(9, 1);                  // На 9 столбце 1 строки вывести:
  lcd.print(uint16_t(massive[5]));      // влажность почвы
  
  // Отладочные данные
  Serial.print(massive[0]);
  Serial.print(',');
  Serial.print(massive[1]);
  Serial.print(',');
  Serial.print(massive[2]);
  Serial.print(',');
  Serial.print(massive[3]);
  Serial.print(',');
  Serial.print(massive[4]);
  Serial.print(',');
  Serial.print(massive[5]);
  Serial.print(',');
  Serial.println(massive[6]);
}
