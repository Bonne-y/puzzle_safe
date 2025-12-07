#include <TM1637Display.h>

// ----------------- Пины -----------------
#define CLK 4
#define DIO 3
TM1637Display display(CLK, DIO);

// Светодиоды
const uint8_t LED_RED   = 11;
const uint8_t LED_GREEN = 12;

// Провода головоломки
const uint8_t PIN_CORRECT = 5;
const uint8_t PIN_WRONG1  = 6;
const uint8_t PIN_WRONG2  = 7;
const uint8_t PIN_WRONG3  = 8;

// ----------------- Настройки -----------------
const unsigned long GAME_TIME_MS = 60000UL; // 60 секунд
const unsigned long DEBOUNCE_MS = 50;

// ----------------- Переменные -----------------
unsigned long startMillis;    // время старта таймера
unsigned long fixedElapsed;   // фиксируем время при правильном проводе
bool solved = false;          // правильный провод вытащен
bool timeUp = false;          // время вышло

// Для антидребезга проводов
int prevState[4] = {HIGH,HIGH,HIGH,HIGH};
unsigned long lastChangeTime[4] = {0,0,0,0};

// ----------------- setup -----------------
void setup() {
  // LED
  pinMode(LED_RED, OUTPUT);   digitalWrite(LED_RED, LOW);
  pinMode(LED_GREEN, OUTPUT); digitalWrite(LED_GREEN, LOW);

  // Провода с внешними pull-down 10k к GND
  pinMode(PIN_CORRECT, INPUT);
  pinMode(PIN_WRONG1, INPUT);
  pinMode(PIN_WRONG2, INPUT);
  pinMode(PIN_WRONG3, INPUT);

  // TM1637
  display.setBrightness(0x0f);
  display.clear();

  // Старт таймера сразу
  startMillis = millis();
}

// ----------------- loop -----------------
void loop() {
  unsigned long now = millis();
  unsigned long elapsed;

  // Таймер останавливается при решении
  if (solved) {
    elapsed = fixedElapsed;
  } else {
    elapsed = now - startMillis;
    // Проверка истечения времени
    if (!solved && elapsed >= GAME_TIME_MS) {
      timeUp = true;
      elapsed = GAME_TIME_MS;
    }
  }

  unsigned long remaining = (GAME_TIME_MS > elapsed) ? (GAME_TIME_MS - elapsed) : 0;
  showTime(remaining / 1000);

  // Проверка проводов, только если игра не завершена
  if (!solved && !timeUp) {
    checkWire(PIN_CORRECT, true, 0);
    checkWire(PIN_WRONG1, false, 1);
    checkWire(PIN_WRONG2, false, 2);
    checkWire(PIN_WRONG3, false, 3);
  }

  // Время вышло, правильный провод не вытянули
  if (timeUp && !solved) {
    digitalWrite(LED_RED, HIGH);
  }
}

// ----------------- Проверка одного провода -----------------
void checkWire(uint8_t pin, bool isCorrect, int index) {
  int reading = digitalRead(pin);
  unsigned long now = millis();

  if (reading != prevState[index]) {
    lastChangeTime[index] = now;
    prevState[index] = reading;
  }

  // LOW = провод вытащен
  if ((now - lastChangeTime[index]) > DEBOUNCE_MS && reading == LOW) {
    handleWirePulled(isCorrect);
  }
}

// ----------------- Обработка выдернутого провода -----------------
void handleWirePulled(bool isCorrect) {
  if (solved || timeUp) return;

  if (isCorrect) {
    solved = true;
    fixedElapsed = millis() - startMillis; // фиксируем время
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH); // зелёный LED горит, таймер останавливается
  } else {
    // неправильный провод → красный LED мигает, таймер продолжает
    for(int i=0;i<2;i++){
      digitalWrite(LED_RED,HIGH); delay(200);
      digitalWrite(LED_RED,LOW);  delay(150);
    }
  }
}

// ----------------- Отображение таймера -----------------
void showTime(long secondsLeft) {
  if (secondsLeft < 0) secondsLeft = 0;
  int mm = secondsLeft / 60;
  int ss = secondsLeft % 60;
  int value = mm*100 + ss;
  uint8_t dots = 0b01000000; // двоеточие
  display.showNumberDecEx(value,dots,true,4,0);
}