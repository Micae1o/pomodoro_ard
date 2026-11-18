#include "SevSeg.h"

SevSeg sevseg;

const unsigned long FOCUS_MS = 20UL*1000UL;
const unsigned long BREAK_MS = 10UL*1000UL;
const unsigned long DEBOUNCE_MS = 50UL;

const byte PIN_BTN_START = A0;
const byte PIN_BTN_PAUSE = A1;
const byte PIN_BTN_CONFIRM = A2;
const byte PIN_BTN_RESET = A3;
const byte PIN_LED = A4;

enum class Phase : uint8_t {
  Idle,
  Focus,
  FocusDone,
  Break,
  BreakDone
};

Phase phase = Phase::Idle;
bool paused = false;
unsigned long remainingMs = FOCUS_MS;
unsigned long lastUpdateMs = 0;

struct Button {
  byte pin;
  bool stableDown;
  bool lastRawDown;
  unsigned long lastChangeMs;
  bool pressedEvent;
};

Button btnStart = {PIN_BTN_START, false, false, 0, false};
Button btnPause = {PIN_BTN_PAUSE, false, false, 0, false};
Button btnConfirm = {PIN_BTN_CONFIRM, false, false, 0, false};
Button btnReset = {PIN_BTN_RESET, false, false, 0, false};

void updateButton(Button &btn) {
  const bool rawDown = digitalRead(btn.pin) == LOW;
  const unsigned long now = millis();

  if (rawDown != btn.lastRawDown) {
    btn.lastChangeMs = now;
    btn.lastRawDown = rawDown;
  }

  btn.pressedEvent = false;
  if ((now - btn.lastChangeMs) > DEBOUNCE_MS && rawDown != btn.stableDown) {
    btn.stableDown = rawDown;
    if (btn.stableDown) {
      btn.pressedEvent = true;
    }
  }
}

void showTime(unsigned long ms) {
  const int totalSec = (int)(ms / 1000UL);
  const int mins = totalSec / 60;
  const int secs = totalSec % 60;
  sevseg.setNumber(mins * 100 + secs, 2);
}

void enterIdle() {
  phase = Phase::Idle;
  paused = false;
  remainingMs = FOCUS_MS;
  digitalWrite(PIN_LED, LOW);
}

void startFocus() {
  phase = Phase::Focus;
  paused = false;
  remainingMs = FOCUS_MS;
  lastUpdateMs = millis();
  digitalWrite(PIN_LED, LOW);
}

void startBreak() {
  phase = Phase::Break;
  paused = false;
  remainingMs = BREAK_MS;
  lastUpdateMs = millis();
  digitalWrite(PIN_LED, LOW);
}

void handleButtons() {
  updateButton(btnStart);
  updateButton(btnPause);
  updateButton(btnConfirm);
  updateButton(btnReset);

  if (btnReset.pressedEvent) {
    enterIdle();
    return;
  }

  if (btnStart.pressedEvent) {
    if (phase == Phase::Idle) {
      startFocus();
    } else if ((phase == Phase::Focus || phase == Phase::Break) && paused) {
      paused = false;
      lastUpdateMs = millis();
    }
  }

  if (btnPause.pressedEvent) {
    if ((phase == Phase::Focus || phase == Phase::Break) && !paused) {
      paused = true;
    }
  }

  if (btnConfirm.pressedEvent) {
    if (phase == Phase::FocusDone) {
      startBreak();
    } else if (phase == Phase::BreakDone) {
      startFocus();
    }
  }
}

void updateTimer() {
  if (paused) {
    return;
  }
  if (phase != Phase::Focus && phase != Phase::Break) {
    return;
  }

  const unsigned long now = millis();
  const unsigned long dt = now - lastUpdateMs;
  lastUpdateMs = now;

  if (dt >= remainingMs) {
    remainingMs = 0;
    phase = (phase == Phase::Focus) ? Phase::FocusDone : Phase::BreakDone;
    digitalWrite(PIN_LED, HIGH);
  } else {
    remainingMs -= dt;
  }
}

void updateDisplay() {
  switch (phase) {
    case Phase::Idle:
      showTime(FOCUS_MS);
      break;
    case Phase::Focus:
    case Phase::Break:
    case Phase::FocusDone:
    case Phase::BreakDone:
      showTime(remainingMs);
      break;
  }
  sevseg.refreshDisplay();
}

void setup() {
  byte numDigits = 4;
  byte digitPins[] = {10, 11, 12, 13};
  byte segmentPins[] = {2, 3, 4, 5, 6, 7, 8, 9};

  sevseg.begin(
    COMMON_CATHODE,
    numDigits,
    digitPins,
    segmentPins,
    false,
    false,
    true
  );
  sevseg.setBrightness(60);

  pinMode(PIN_BTN_START, INPUT_PULLUP);
  pinMode(PIN_BTN_PAUSE, INPUT_PULLUP);
  pinMode(PIN_BTN_CONFIRM, INPUT_PULLUP);
  pinMode(PIN_BTN_RESET, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  enterIdle();
}

void loop() {
  handleButtons();
  updateTimer();
  updateDisplay();
}
