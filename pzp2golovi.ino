#include <TM1637Display.h>
#include <Keypad.h>

// ----------------- Пины TM1637 -----------------
#define CLK 20
#define DIO 21
TM1637Display display(CLK, DIO);

// ----------------- Пины светодиодов -----------------
const uint8_t LED_RED_WIRE     = 42;
const uint8_t LED_GREEN_WIRE   = 44;
const uint8_t LED_RED_KEYPAD   = 16;
const uint8_t LED_GREEN_KEYPAD = 18;

// ----------------- Пины проводов -----------------
const uint8_t PIN_CORRECT = 30;
const uint8_t PIN_WRONG1  = 34;
const uint8_t PIN_WRONG2  = 37;
const uint8_t PIN_WRONG3  = 40;

// ----------------- Настройки -----------------
const unsigned long GAME_TIME_MS = 120000UL; // <<< 2 МИНУТЫ
const unsigned long DEBOUNCE_MS = 50;
const unsigned long START_DELAY_MS = 200;
const int MAX_ERRORS = 3;

// ----------------- Переменные -----------------
unsigned long startMillis;
unsigned long fixedElapsed;
bool solvedWire = false;
bool solvedKeypad = false;
bool timeUp = false;
bool timerStopped = false;
bool gameOver = false;

int prevState[4] = {HIGH,HIGH,HIGH,HIGH};
unsigned long lastChangeTime[4] = {0,0,0,0};
bool wirePulled[4] = {false,false,false,false};

int errorCount = 0;

// ----------------- Новая раскладка клавиатуры (повёрнута на 90° CCW) -----------------
const byte ROWS = 4;
const byte COLS = 4;

byte rowPins[ROWS] = {22, 23, 24, 25};
byte colPins[COLS] = {26, 27, 28, 29};

// 90° CCW rotation changes key positions like this matrix:
char keys[ROWS][COLS] = {
  {'3','6','9','D'},
  {'2','5','8','0'},
  {'1','4','7','*'},
  {'A','B','C','#'}
};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// новая правильная комбинация с учётом поворота
char correctSequence[4] = {'1','5','9','D'}; 
char inputSequence[4];
int inputIndex = 0;

// ----------------- Системные флаги мигания -----------------
unsigned long blinkTimer = 0;
bool blinkState = false;

// ----------------- Setup -----------------
void setup() {

  pinMode(LED_RED_WIRE, OUTPUT);
  pinMode(LED_GREEN_WIRE, OUTPUT);
  pinMode(LED_RED_KEYPAD, OUTPUT);
  pinMode(LED_GREEN_KEYPAD, OUTPUT);

  digitalWrite(LED_RED_WIRE, LOW);
  digitalWrite(LED_GREEN_WIRE, LOW);
  digitalWrite(LED_RED_KEYPAD, LOW);
  digitalWrite(LED_GREEN_KEYPAD, LOW);

  pinMode(PIN_CORRECT, INPUT);
  pinMode(PIN_WRONG1, INPUT);
  pinMode(PIN_WRONG2, INPUT);
  pinMode(PIN_WRONG3, INPUT);

  display.setBrightness(0x0f);
  display.clear();

  startMillis = millis();

  for (int i=0; i<4; i++)
    prevState[i] = digitalRead(PIN_CORRECT + i);
}

// ----------------- Loop -----------------
void loop() {
  unsigned long now = millis();

  if (now - startMillis < START_DELAY_MS) return;

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

  unsigned long remaining = GAME_TIME_MS - elapsed;
  if (!timerStopped)
    showTime(remaining / 1000);

  // --- Провода ---
  if (!solvedWire && !timeUp && !timerStopped && !gameOver) {
    checkWire(PIN_CORRECT, true, 0);
    checkWire(PIN_WRONG1, false, 1);
    checkWire(PIN_WRONG2, false, 2);
    checkWire(PIN_WRONG3, false, 3);
  }

  // --- Клавиатура ---
  if (!solvedKeypad && !timeUp && !timerStopped && !gameOver) {
    char key = keypad.getKey();
    if (key) processKey(key);
  }

  // --- Проверка условия успешного решения ---
  if (!timerStopped && solvedWire && solvedKeypad) {
    stopTimer();
    display.showNumberDec(1748, true);
  }

  // --- Game Over ---
  if (!gameOver && errorCount >= MAX_ERRORS) {
    gameOver = true;
    stopTimer();
    turnAllRed();
  }
}

// ----------------- Провода -----------------
void checkWire(uint8_t pin, bool isCorrect, int index) {
  int reading = digitalRead(pin);
  unsigned long now = millis();

  if (reading != prevState[index]) {
    lastChangeTime[index] = now;
    prevState[index] = reading;
  }

if ((now - lastChangeTime[index]) > DEBOUNCE_MS && reading == LOW) {
    if (!wirePulled[index]) {
      wirePulled[index] = true;
      handleWirePulled(isCorrect);
    }
  }
}

void handleWirePulled(bool isCorrect) {
  if (solvedWire || timeUp || timerStopped || gameOver) return;

  if (isCorrect) {
    solvedWire = true;
    digitalWrite(LED_GREEN_WIRE, HIGH);
    digitalWrite(LED_RED_WIRE, LOW);
  } else {
    errorCount++;
    blinkRedLED(LED_RED_WIRE);
  }
}

// ----------------- Неблокирующее мигание красных LED -----------------
void blinkRedLED(uint8_t pin) {
  unsigned long now = millis();

  if (now - blinkTimer > 120) {
    blinkTimer = now;
    blinkState = !blinkState;
    digitalWrite(pin, blinkState ? HIGH : LOW);
  }
}

// ----------------- Клавиатура -----------------
void processKey(char key) {
  inputSequence[inputIndex++] = key;

  if (inputIndex == 4) {
    bool correct = true;

    for (int i = 0; i < 4; i++)
      if (inputSequence[i] != correctSequence[i])
        correct = false;

    if (correct) {
      solvedKeypad = true;
      digitalWrite(LED_GREEN_KEYPAD, HIGH);
      digitalWrite(LED_RED_KEYPAD, LOW);
    } else {
      errorCount++;
      blinkRedLED(LED_RED_KEYPAD);
      inputIndex = 0;
    }
  }
}

// ----------------- Game Over -----------------
void turnAllRed() {
  digitalWrite(LED_GREEN_WIRE, LOW);
  digitalWrite(LED_GREEN_KEYPAD, LOW);
  digitalWrite(LED_RED_WIRE, HIGH);
  digitalWrite(LED_RED_KEYPAD, HIGH);
}

// ----------------- Остановка таймера -----------------
void stopTimer() {
  timerStopped = true;
  fixedElapsed = millis() - startMillis;
}

// ----------------- Отображение таймера -----------------
void showTime(long secondsLeft) {
  if (secondsLeft < 0) secondsLeft = 0;
  int mm = secondsLeft / 60;
  int ss = secondsLeft % 60;
  int value = mm * 100 + ss;
  uint8_t dots = 0b01000000;
  display.showNumberDecEx(value, dots, true);
}