#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(9, 10);  // "создать" модуль на пинах 9 и 10 Для Уно
LiquidCrystal_I2C lcd(0x27, 20, 4);
const uint64_t pipe00 = 0xE8E8F0F0E1LL;
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

void setup() {
  Serial.begin(9600);         // открываем порт для связи с ПК
  lcd.init();                   // Инициализируем LCD дисплей
  lcd.backlight();              // Включаем подсветку (Сделать включение подсветки по кнопке?)
  
  // Выводим значения, которые не будут изменяться в процессе работы, т.е. статичные надписи
  lcd.setCursor(2, 0);          // Во 2 столбце 0 строки вывести:
  lcd.print(char(223));         // Значок градуса
  lcd.print(F("C"));            // Букву С
  lcd.setCursor(0, 1);          // На первой строке вывести:
  lcd.print("Humi ");           // Humi (от англ. Humidity - влажность)
  lcd.setCursor(4, 2);          // На 4 столбце строки 2 вывести:
  lcd.print("mmHg");            // mmHg (мм.рт.ст.)
  lcd.setCursor(6, 3);
  lcd.print("pH");
  lcd.setCursor(12, 0);
  lcd.print("% water");
  lcd.setCursor(12, 1);
  lcd.print("% moist");
  radio.begin();              // активировать модуль
  radio.setAutoAck(1);        // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();   // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);   // размер пакета, в байтах

  radio.openReadingPipe(1, pipe00);   // хотим слушать трубу 0
  radio.setChannel(0x70);     // выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_HIGH);   // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); // скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp();        // начать работу
  radio.startListening(); // начинаем слушать эфир, мы приёмный модуль
  delay(1000);
}

void loop() {
  float massive[6];
  if( radio.available()){
   radio.read(&massive,sizeof(massive));             // по адресу переменной in функция записывает принятые данные;
}
  lcd.setCursor(0, 0);                  // На 0 строке 0 столбца вывести:
  lcd.print(int(massive[0]));                    // Температуру
  lcd.setCursor(5, 1);                  // На 1 строке 5 столбца вывести:
  lcd.print(int(massive[1]));                      // Влажность
  lcd.print("%");                       // Значок %
  lcd.setCursor(0, 2);                  // На 2 строке 0 столбца вывести:
  lcd.print(uint16_t(massive[2]));        // атм. давление
  lcd.setCursor(0, 3);                  // На 3 строке вывести:
  lcd.print(massive[3]);                // уровень pH
  lcd.setCursor(9, 0);                  // На 9 столбце 0 строки вывести:
  lcd.print(uint16_t(massive[4]));                // уровень жидкости
  lcd.setCursor(9, 1);                  // На 9 столбце 1 строки вывести:
  lcd.print(uint16_t(massive[5]));                // влажность почвы
  
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
  Serial.println(massive[5]);
}
