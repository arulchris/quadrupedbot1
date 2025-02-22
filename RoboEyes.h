#ifndef ROBO_EYES_H
#define ROBO_EYES_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

enum class EyeExpression : uint8_t {
  NORMAL,
  HAPPY,
  BLINK,
  SLEEP,
  SURPRISED,
  WINK,
  BORING
};

enum class EyePosition : uint8_t {
  CENTER,
  LEFT,
  RIGHT,
  MOVING_LEFT,
  MOVING_RIGHT
};

struct EyeShape {
  uint8_t width;
  uint8_t height;
  uint8_t radius;
};

class RoboEyes {
private:
  Adafruit_SSD1306* display;
  EyeShape baseShape;
  uint8_t screenWidth;
  uint8_t screenHeight;
  uint8_t eyeSpacing;
  int16_t eyeY;
  int16_t baseEyeY;
  
  EyeExpression currentExpr;
  EyeExpression nextExpr;
  EyePosition currentPos;
  EyePosition targetPos;
  uint8_t animStep;
  uint8_t blinkCount;
  uint8_t currentBlink;
  
  bool isAnimating;
  bool shouldReturnToNormal;
  
  float positionProgress;
  
  unsigned long lastUpdate;
  unsigned long lastRandomAction;
  unsigned long lastBlinkTime;
  unsigned long positionTimeout;
  unsigned long expressionTimeout;
  
  static const uint8_t ANIM_INTERVAL = 33;
  static const uint8_t MAX_ANIM_STEPS = 8;
  static const int8_t SIDE_OFFSET = 24;
  static const uint16_t POSITION_TRANSITION_TIME = 300;
  static const uint16_t RANDOM_ACTION_INTERVAL = 3000;
  static const uint16_t BLINK_CYCLE_INTERVAL = 300;
  
  void drawEyePair(const EyeShape& left, const EyeShape& right);
  int8_t getCurrentOffset();
  void updatePositionTransition();
  void resetToNormalAfterDelay();
  void performRandomAction();
  
  void handleNormal();
  void handleHappy();
  void handleBlink();
  void handleSleep();
  void handleSurprised();
  void handleWink();
  void handleBoring();

public:
  RoboEyes(uint8_t w = 128, uint8_t h = 32);
  bool begin(TwoWire *theWire = &Wire);
  void setEyeShape(uint8_t w, uint8_t h, uint8_t r);
  void setExpression(EyeExpression exp, uint32_t duration = 0, uint8_t count = 1);
  void setPosition(EyePosition pos, uint32_t duration = 0);
  void update();
  bool isAnimationComplete() { return !isAnimating; }
};

#endif