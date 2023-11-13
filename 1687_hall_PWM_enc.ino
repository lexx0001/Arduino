/*
  Скетч, считающий сигналы с датчика Холла с выводом на дисплей 1637
  библиотека https://github.com/avishorp/TM1637
  +
  Скетч,обрабатываюий энкодер и регулирующий
  PWM
*/
/*
    Sketch, counting signals from the Hall sensor with output to the 1637 display
  library https://github.com/avishorp/TM1637
  +
    Sketch, processing the encoder and adjusting
    PWM
*/
#include <Arduino.h>
#include <TM1637Display.h> // display library//библиотека дисплея
// Module connection pins (Digital Pins)
#define CLK 4
#define DIO 5
TM1637Display display(CLK, DIO);
#include "GyverEncoder.h" // encoder library//библиотека энкодера

#define CLK 6           // S2   designate// S2   обозначаем
#define DT 7            // S1    pins// S1    пины
#define SW 8            // KEY  encoder// KEY  энкодера
#define step_pwm 3      // setting the encoder step// установка шага энкодера
#define big_step_pwm 13 // setting the fast encoder step// установка быстрого шага энкодера

Encoder enc1(CLK, DT, SW);
byte enc;

#define pwm_max 220 // setting the maximum for PWM// установка максимума для PWM
#define pwm_min 55  // setting the minimum for PWM// установка минимума для PWM

#define ml_correct 0.142857 // correction coefficient in milliliters// коэффициент коррекции в миллилитры

void encWork();
void dviglo();
void pwm_limit();

void sensHoll();
void outputToDisp();
void dot_and_ml();
void ml_counting();
byte dotPosition;

unsigned long ml;
volatile unsigned long turns;
unsigned int norm_ml;

void setup()
{

  display.setBrightness(0x0f);      // turn on the screen and set the brightness 1687// включение экрана и установка яркости 1687
  display.showNumberDec(ml, false); // output to the display 1687// вывод на дисплей 1687

  attachInterrupt(0, sensHoll, RISING); // connect the interrupt to pin 2 when the Hall signal rises// подключить прерывание на 2 пин при повышении сигнала Hall

  Serial.begin(9600);

  enc1.setType(TYPE2);     // encoder type TYPE1 single-step, TYPE2 two-step. If your encoder works strangely, change the type// тип энкодера TYPE1 одношаговый, TYPE2 двухшаговый. Если ваш энкодер работает странно, смените тип
  enc1.setFastTimeout(30); // timeout for isFastR speed. Default 50 ENCODER// таймаут на скорость isFastR. По умолч. 50 ENCODER
}

void loop()
{
  enc1.tick();
  encWork();
  dviglo();

  outputToDisp();

  //  delay(50); // delay for stability //задержка для стабильности
}

void sensHoll()
{
  turns++; // count turns// считаем обороты
}

void outputToDisp()
{
  static unsigned long ml_old = 0;

  ml_counting(); // count milliliters// считаем миллилитры

  if (ml != ml_old)
  {

    dot_and_ml();

    display.showNumberDecEx(norm_ml, (0x80 >> dotPosition), true); // output to display// вывод на дисплей

    Serial.print("ml=");
    Serial.println(ml); // output to port in milliliters// вывод в порт в миллилитрах
    Serial.print("turns=");
    Serial.println(turns); // output to port in turns// вывод в порт в оборотах
    Serial.print("ml_old=");
    Serial.println(ml_old);
    Serial.print("norm_ml=");
    Serial.println(norm_ml);
    Serial.print("dotPosition=");
    Serial.println(dotPosition);
    Serial.println("--------------");

    ml_old = ml;
  }
}

void dot_and_ml()
{

  const uint8_t FULL[] = {0x71, 0x3e, 0x38, 0x38}; // "FULL"
  //  Serial.println("dot_and_ml going");
  if (ml <= 9999)
  {
    norm_ml = ml;
    dotPosition = 0; // dot position// позиция точки
  }
  else if (ml <= 99999)
  {
    dotPosition = 1;
    norm_ml = ml * 0.1; // norm_ml - global four-digit variable for display// norm_ml - глобальная четырёхзначная переменная для вывода на дисплей
  }
  else if (ml <= 999999)
  {
    dotPosition = 2;
    norm_ml = ml * 0.01;
  }
  else
  {
    display.setSegments(FULL); //  display "FULL"//  выводим на дисплей "FULL"
    delay(3000);

    ml = 0;
    norm_ml = 0;
    dotPosition = 0; //(dot position) returns to its original value - 0.000//(положение точки) возвращается в изначальное значение - 0,000

    //    while (1) {
  }
}

void ml_counting()
{
  ml = (turns * ml_correct); // count milliliters// считаем миллилитры
}

void encWork()
{

  if (enc1.isRight())
  {
    if (enc == 0)
    { // make a jump in PWM voltage from 0 to the lower limit - so as not to twist for a long time// делаем скачок напряжения PWM от 0 до нижнего предела - что бы долго не крутить
      enc = pwm_min;
    }
    enc += step_pwm; // if there was a turn to the right, we increase by step_pwm// если был поворот направо, увеличиваем на step_pwm
  }
  if (enc1.isLeft() && enc != 0)
    enc -= step_pwm; // if there was a turn to the left, we decrease by step_pwm// если был поворот налево, уменьшаем на step_pwm

  if (enc1.isFastR())
    enc += big_step_pwm; // if there was a quick turn to the right, we increase by big_step_pwm// если был быстрый поворот направо, увеличиваем на big_step_pwm
  if (enc1.isFastL() && enc != 0)
    enc -= big_step_pwm; // if there was a quick turn to the left, we decrease by big_step_pwm// если был быстрый поворот налево, уменьшаем на big_step_pwm

  //  enc = constrain (enc, 20, pwm_max);    // limit the voltage on PWM //ограничиваем напряжение на PWM

  if (enc1.isPress())
  { // if there was a press, reset the flag to "0"// если было нажатие сброс флага  на "0"
    enc = 0;
    Serial.println(enc);
  }

  if (enc1.isTurn())
  {                      // if a turn was made (turn indicator in any direction)// если был совершён поворот (индикатор поворота в любую сторону)
    Serial.println(enc); // output the value when turning// выводим значение при повороте
  }

  pwm_limit(); // check the PWM voltage limits// проверяем пределы напряжения PWM
}

void dviglo()
{
  analogWrite(10, enc);
}

void pwm_limit()
{
  // here we limit the voltage values to prevent overload or excessively low voltage for the motor// здесь ограничиваем значения напряжения чтобы не допустить перегрузки или излишне малого напряжения для двигателя

  if (enc < pwm_min)
  {
    enc = 0;
  }
  else if (enc > pwm_max)
  {
    enc = pwm_max;
  }
}
