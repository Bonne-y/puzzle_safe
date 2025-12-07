--#include <TM1637Display.h>
#include <Keypad.h>

// ----------------- Пины TM1637 -----------------
#define CLK 20
#define DIO 21
TM1637Display display(CLK, DIO);

// ----------------- Пины светодиодов -----------------
const uint8_t LED_RED_WIRE   = 42;
const uint8_t LED_GREEN_WIRE = 44;
const uint8_t LED_RED_KEYPAD   = 17;
const uint8_t LED_GREEN_KEYPAD = 4;

// ----------------- Пины проводов -----------------
const uint8_t PIN_CORRECT = 30;
const uint8_t PIN_WRONG1  = 34;
const uint8_t PIN_WRONG2  = 37;
const uint8_t PIN_WRONG3  = 40;

// ----------------- Настройки -----------------
const unsigned long GAME_TIME_MS = 120000UL;   // <<< 2 МИНУТЫ !!!
const unsigned long DEBOUNCE_MS = 50;
const unsigned long START_DELAY_MS = 300;
const int MAX_ERRORS = 3;

// ----------------- Переменные -----------------
unsigned long startMillis;
unsigned long fixedElapsed;
bool solvedWire = false;
bool solvedKeypad = false;
bool timeUp = false;
bool timerStopped = false;
bool gameOver = false;

int prevState[4];
unsigned long lastChangeTime[4] = {0,0,0,0};
bool wirePulled[4] = {false,false,false,false};

int errorCount = 0;

// ----------------- Клавиатура -----------------
const byte ROWS = 4;
const byte COLS = 4;

byte rowPins[ROWS] = {22, 23, 24, 25};
byte colPins[COLS] = {26, 27, 28, 29};

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

char correctSequence[4] = {'2', '6', 'C', 'D'};
char inputSequence[4];
int inputIndex = 0;

// ----------------- Setup -----------------
void setup() {

  // LED
  pinMode(LED_RED_WIRE, OUTPUT);     digitalWrite(LED_RED_WIRE, LOW);
  pinMode(LED_GREEN_WIRE, OUTPUT);   digitalWrite(LED_GREEN_WIRE, LOW);
  pinMode(LED_RED_KEYPAD, OUTPUT);   digitalWrite(LED_RED_KEYPAD, LOW);
  pinMode(LED_GREEN_KEYPAD, OUTPUT); digitalWrite(LED_GREEN_KEYPAD, LOW);

  // Провода
  pinMode(PIN_CORRECT, INPUT);
  pinMode(PIN_WRONG1, INPUT);
  pinMode(PIN_WRONG2, INPUT);
  pinMode(PIN_WRONG3, INPUT);

  // Клавиатура
  for (int i = 0; i < ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }
  for (int i = 0; i < COLS; i++) {
    pinMode(colPins[i], INPUT_PULLUP);
  }

  display.setBrightness(0x0f);
  display.clear();

  startMillis = millis();

  prevState[0] = digitalRead(PIN_CORRECT);
  prevState[1] = digitalRead(PIN_WRONG1);
  prevState[2] = digitalRead(PIN_WRONG2);
  prevState[3] = digitalRead(PIN_WRONG3);
}

// ----------------- Loop -----------------
void loop() {

  unsigned long now = millis();
  if (now - startMillis < START_DELAY_MS) return;

  // --- Клавиатура ---
  if (!timerStopped && !gameOver && !solvedKeypad && !timeUp) {
    handleKeypad();
  }

  // --- Таймер ---
  unsigned long elapsed;
  if (timerStopped) {
    elapsed = fixedElapsed;
  } else {
    elapsed = now - startMillis;
    if (elapsed >= GAME_TIME_MS) {
      timeUp = true;
      elapsed = GAME_TIME_MS;
      stopTimer();
    }
  }

  // Показываем оставшееся время если игра НЕ закончена успехом
  if (!(solvedWire && solvedKeypad)) {
    unsigned long remaining = (GAME_TIME_MS > elapsed) ? (GAME_TIME_MS - elapsed) : 0;
    showTime(remaining / 1000);
  }

  // --- Провода ---
  if (!solvedWire && !timeUp && !timerStopped && !gameOver) {
    checkWire(PIN_CORRECT, true, 0);
    checkWire(PIN_WRONG1, false, 1);
    checkWire(PIN_WRONG2, false, 2);
    checkWire(PIN_WRONG3, false, 3);
  }

  if (!solvedWire && timeUp) digitalWrite(LED_RED_WIRE, HIGH);
  if (!solvedKeypad && timeUp) digitalWrite(LED_RED_KEYPAD, HIGH);

  // --- Победа ---
  if (!timerStopped && solvedWire && solvedKeypad) {
    stopTimer();
    showWinCode();
  }

  // --- Game Over ---
  if (!gameOver && errorCount >= MAX_ERRORS) {
    gameOver = true;
    stopTimer();
    digitalWrite(LED_RED_WIRE, HIGH);
    digitalWrite(LED_RED_KEYPAD, HIGH);
    digitalWrite(LED_GREEN_WIRE, LOW);
    digitalWrite(LED_GREEN_KEYPAD, LOW);
  }
}
// -------- Провода --------
void checkWire(uint8_t pin, bool isCorrect, int index) {
  int reading = digitalRead(pin);
  unsigned long now = millis();

  if (reading != prevState[index]) {
    lastChangeTime[index] = now;
    prevState[index] = reading;
  }

  if ((now - lastChangeTime[index]) > DEBOUNCE_MS && reading == LOW) {
    if (!wirePulled[index]) {
      handleWirePulled(isCorrect, index);
      wirePulled[index] = true;
    }
  }
}

void handleWirePulled(bool isCorrect, int index) {
  if (solvedWire || timeUp || timerStopped || gameOver) return;

  if (isCorrect) {
    solvedWire = true;
    digitalWrite(LED_RED_WIRE, LOW);
    digitalWrite(LED_GREEN_WIRE, HIGH);
  } else {
    errorCount++;
    for (int i = 0; i < 2; i++) {
      digitalWrite(LED_RED_WIRE, HIGH); delay(200);
      digitalWrite(LED_RED_WIRE, LOW);  delay(150);
    }
  }
}

// -------- Клавиатура --------
void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;

  inputSequence[inputIndex++] = key;

  if (inputIndex == 4) {
    bool ok = true;
    for (int i = 0; i < 4; i++) {
      if (inputSequence[i] != correctSequence[i]) ok = false;
    }

    if (ok) {
      solvedKeypad = true;
      digitalWrite(LED_GREEN_KEYPAD, HIGH);
      digitalWrite(LED_RED_KEYPAD, LOW);
    } else {
      errorCount++;
      for (int i = 0; i < 2; i++) {
        digitalWrite(LED_RED_KEYPAD, HIGH); delay(200);
        digitalWrite(LED_RED_KEYPAD, LOW);  delay(150);
      }
    }

    inputIndex = 0;
  }
}

// -------- Победа: вывод 1748 --------
void showWinCode() {
  display.showNumberDecEx(478, 0, true);
}

// -------- Таймер --------
void stopTimer() {
  timerStopped = true;
  fixedElapsed = millis() - startMillis;
}

void showTime(long secondsLeft) {
  if (secondsLeft < 0) secondsLeft = 0;
  int mm = secondsLeft / 60;
  int ss = secondsLeft % 60;
  int value = mm*100 + ss;
  uint8_t dots = 0b01000000;
  display.showNumberDecEx(value, dots, true, 4, 0);
}