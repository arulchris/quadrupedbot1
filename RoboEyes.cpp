#include "RoboEyes.h"

RoboEyes::RoboEyes(uint8_t w, uint8_t h) {
  screenWidth = w;
  screenHeight = h;
  display = new Adafruit_SSD1306(screenWidth, screenHeight, &Wire, -1);
  
  baseShape = {16, 16, 4};
  eyeSpacing = 12;
  eyeY = screenHeight / 2 - 8;
  baseEyeY = eyeY;
  
  currentExpr = EyeExpression::NORMAL;
  nextExpr = EyeExpression::NORMAL;
  currentPos = EyePosition::CENTER;
  targetPos = EyePosition::CENTER;
  
  animStep = 0;
  blinkCount = 1;
  currentBlink = 0;
  
  isAnimating = false;
  shouldReturnToNormal = true;
  
  positionProgress = 0;
  
  lastUpdate = 0;
  lastRandomAction = 0;
  lastBlinkTime = 0;
  positionTimeout = 0;
  expressionTimeout = 0;
}

bool RoboEyes::begin(TwoWire *theWire) {
  if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C, false, theWire)) return false;
  display->clearDisplay();
  display->display();
  return true;
}

void RoboEyes::setExpression(EyeExpression exp, uint32_t duration, uint8_t count) {
  if (exp != currentExpr) {
    eyeY = baseEyeY;
    currentExpr = exp;
    nextExpr = EyeExpression::NORMAL;
    animStep = 0;
    isAnimating = true;
    shouldReturnToNormal = duration > 0;
    
    if (exp == EyeExpression::BLINK) {
      blinkCount = count;
      currentBlink = 0;
      lastBlinkTime = 0;
    }
    
    expressionTimeout = duration > 0 ? millis() + duration : 0;
  }
}

void RoboEyes::setPosition(EyePosition pos, uint32_t duration) {
  if (pos != currentPos) {
    targetPos = pos;
    positionProgress = 0;
    
    // Store current position before starting transition
    if (pos == EyePosition::CENTER) {
      currentPos = currentPos == EyePosition::LEFT ? EyePosition::MOVING_LEFT : EyePosition::MOVING_RIGHT;
    } else {
      currentPos = pos == EyePosition::LEFT ? EyePosition::MOVING_LEFT : EyePosition::MOVING_RIGHT;
    }
    
    positionTimeout = duration > 0 ? millis() + duration : 0;
  }
}

void RoboEyes::performRandomAction() {
  if (currentExpr != EyeExpression::NORMAL || isAnimating || millis() - lastRandomAction < RANDOM_ACTION_INTERVAL) return;
  
  bool isInSidePosition = currentPos >= EyePosition::LEFT && currentPos <= EyePosition::MOVING_RIGHT;
  uint8_t action = random(0, 10);
  
  switch (action) {
    case 0:
    case 1:
      setExpression(EyeExpression::BLINK, 0, 1);
      break;
    case 2:
    case 3:
      setExpression(EyeExpression::BLINK, 0, 2);
      break;
    case 4:
      setExpression(EyeExpression::HAPPY, 500);
      break;
    case 5:
      if (!isInSidePosition) setPosition(EyePosition::LEFT, 1000);
      break;
    case 6:
      if (!isInSidePosition) setPosition(EyePosition::RIGHT, 1000);
      break;
    default:
      setExpression(EyeExpression::BLINK, 0, 1);
  }
  
  lastRandomAction = millis();
}

void RoboEyes::resetToNormalAfterDelay() {
  if (expressionTimeout > 0 && millis() >= expressionTimeout && shouldReturnToNormal) {
    setExpression(EyeExpression::NORMAL);
    expressionTimeout = 0;
  }
}

// int8_t RoboEyes::getCurrentOffset() {
//   if (currentPos == EyePosition::MOVING_LEFT || currentPos == EyePosition::MOVING_RIGHT) {
//     int8_t startOffset;
//     int8_t targetOffset;
    
//     // Determine start offset based on current state
//     if (currentPos == EyePosition::MOVING_LEFT) {
//       startOffset = targetPos == EyePosition::LEFT ? 0 : -SIDE_OFFSET;
//     } else {
//       startOffset = targetPos == EyePosition::RIGHT ? 0 : SIDE_OFFSET;
//     }
    
//     // Determine target offset
//     targetOffset = targetPos == EyePosition::CENTER ? 0 :
//                   targetPos == EyePosition::LEFT ? -SIDE_OFFSET : SIDE_OFFSET;
    
//     // Use eased progress for smooth transition
//     float easedProgress = 0.5f - cos(positionProgress * PI) * 0.5f;
//     return startOffset + (targetOffset - startOffset) * easedProgress;
//   }
  
//   // Return fixed position for non-transitioning states
//   return currentPos == EyePosition::LEFT ? -SIDE_OFFSET : 
//          currentPos == EyePosition::RIGHT ? SIDE_OFFSET : 0;
// }
// Combine position calculations
int8_t  RoboEyes::getCurrentOffset() {
  if (currentPos >= EyePosition::MOVING_LEFT) {
    int8_t start = (currentPos == EyePosition::MOVING_LEFT) ? 
      (targetPos == EyePosition::LEFT ? 0 : -SIDE_OFFSET) :
      (targetPos == EyePosition::RIGHT ? 0 : SIDE_OFFSET);
    int8_t end = targetPos == EyePosition::CENTER ? 0 :
                 targetPos == EyePosition::LEFT ? -SIDE_OFFSET : SIDE_OFFSET;
    return start + (end - start) * (0.5f - cos(positionProgress * PI) * 0.5f);
  }
  return currentPos == EyePosition::LEFT ? -SIDE_OFFSET :
         currentPos == EyePosition::RIGHT ? SIDE_OFFSET : 0;
}


void RoboEyes::updatePositionTransition() {
  if (currentPos == EyePosition::MOVING_LEFT || currentPos == EyePosition::MOVING_RIGHT) {
    positionProgress += (float)ANIM_INTERVAL / POSITION_TRANSITION_TIME;
    
    if (positionProgress >= 1.0) {
      positionProgress = 1.0;
      currentPos = targetPos;
      
      if (positionTimeout > 0 && millis() >= positionTimeout && 
          (targetPos == EyePosition::LEFT || targetPos == EyePosition::RIGHT)) {
        setPosition(EyePosition::CENTER);
        positionTimeout = 0;
      }
    }
  } else if (positionTimeout > 0 && millis() >= positionTimeout) {
    setPosition(EyePosition::CENTER);
    positionTimeout = 0;
  }
}

void RoboEyes::setEyeShape(uint8_t w, uint8_t h, uint8_t r) {
  baseShape = {w, h, r};
}

void RoboEyes::drawEyePair(const EyeShape& left, const EyeShape& right) {
  display->clearDisplay();
  
  int8_t posOffset = getCurrentOffset();
  int16_t leftX = (screenWidth/2) - eyeSpacing - left.width + posOffset;
  int16_t rightX = (screenWidth/2) + eyeSpacing + posOffset;
  
  display->fillRoundRect(leftX, eyeY, left.width, left.height, left.radius, SSD1306_WHITE);
  display->fillRoundRect(rightX, eyeY, right.width, right.height, right.radius, SSD1306_WHITE);
  
  display->display();
}

void RoboEyes::handleNormal() {
  drawEyePair(baseShape, baseShape);
  isAnimating = false;
  performRandomAction();
}

void RoboEyes::handleBoring() {
  static unsigned long lastBoringUpdate = 0;
  static bool isLidded = false;
  
  EyeShape boringEye = baseShape;
  boringEye.height = baseShape.height / 2;
  
  if (millis() - lastBoringUpdate >= 500) {
    isLidded = !isLidded;
    lastBoringUpdate = millis();
  }
  
  if (isLidded) boringEye.height -= 2;
  
  drawEyePair(boringEye, boringEye);
  isAnimating = false;
}

void RoboEyes::handleSleep() {
  if (animStep < MAX_ANIM_STEPS/2) {
    EyeShape sleepEye = baseShape;
    sleepEye.height = map(animStep, 0, MAX_ANIM_STEPS/2, baseShape.height, 2);
    eyeY = baseEyeY + (baseShape.height - sleepEye.height);
    drawEyePair(sleepEye, sleepEye);
  } else {
    display->clearDisplay();
    int8_t posOffset = getCurrentOffset();
    int16_t leftX = (screenWidth/2) - eyeSpacing - baseShape.width + posOffset;
    int16_t rightX = (screenWidth/2) + eyeSpacing + posOffset;
    
    display->drawLine(leftX, eyeY, leftX + baseShape.width, eyeY, SSD1306_WHITE);
    display->drawLine(rightX, eyeY, rightX + baseShape.width, eyeY, SSD1306_WHITE);
    display->display();
  }
  
  isAnimating = animStep < MAX_ANIM_STEPS;
  if (isAnimating) animStep++;
}

void RoboEyes::handleHappy() {
  EyeShape happyEye = baseShape;
  happyEye.height = baseShape.height / 2;
  happyEye.radius = baseShape.radius / 2;
  
  drawEyePair(happyEye, happyEye);
  
  if (++animStep >= MAX_ANIM_STEPS) {
    animStep = 0;
    isAnimating = false;
    if (currentExpr != nextExpr) setExpression(nextExpr);
  }
}

void RoboEyes::handleSurprised() {
  EyeShape surprisedEye = baseShape;
  surprisedEye.width += 4;
  surprisedEye.height += 4;
  surprisedEye.radius += 2;
  
  drawEyePair(surprisedEye, surprisedEye);
  isAnimating = false;
}

void RoboEyes::handleWink() {
  EyeShape normalEye = baseShape;
  EyeShape winkEye = baseShape;
  
  winkEye.height = animStep < MAX_ANIM_STEPS/2 ?
    map(animStep, 0, MAX_ANIM_STEPS/2, baseShape.height, 2) :
    map(animStep - MAX_ANIM_STEPS/2, 0, MAX_ANIM_STEPS/2, 2, baseShape.height);
  
  drawEyePair(winkEye, normalEye);
  
  if (++animStep >= MAX_ANIM_STEPS) {
    animStep = 0;
    isAnimating = false;
  }
}

void RoboEyes::handleBlink() {
  if (animStep == 0 && currentBlink > 0 && millis() - lastBlinkTime < BLINK_CYCLE_INTERVAL) return;
  
  if (animStep == 0) lastBlinkTime = millis();
  
  EyeShape eye = baseShape;
  float progress = animStep < MAX_ANIM_STEPS/2 ?
    (float)animStep / (MAX_ANIM_STEPS/2) :
    (float)(animStep - MAX_ANIM_STEPS/2) / (MAX_ANIM_STEPS/2);
  
  float easedProgress = sin(progress * PI/2);
  eye.height = animStep < MAX_ANIM_STEPS/2 ?
    baseShape.height - (baseShape.height - 2) * easedProgress :
    2 + (baseShape.height - 2) * easedProgress;
  
  drawEyePair(eye, eye);
  
  if (++animStep >= MAX_ANIM_STEPS) {
    animStep = 0;
    isAnimating = currentBlink < blinkCount - 1;
    if (isAnimating) {
      currentBlink++;
    } else if (currentExpr != nextExpr) {
      setExpression(nextExpr);
    }
  }
}

void RoboEyes::update() {
  if (millis() - lastUpdate < ANIM_INTERVAL) return;
  
  lastUpdate = millis();
  updatePositionTransition();
  
  if (shouldReturnToNormal) resetToNormalAfterDelay();
  
  isAnimating |= currentPos == EyePosition::MOVING_LEFT || currentPos == EyePosition::MOVING_RIGHT;
  
  switch (currentExpr) {
    case EyeExpression::NORMAL: handleNormal(); break;
    case EyeExpression::HAPPY: handleHappy(); break;
    case EyeExpression::BLINK: handleBlink(); break;
    case EyeExpression::SLEEP: handleSleep(); break;
    case EyeExpression::SURPRISED: handleSurprised(); break;
    case EyeExpression::WINK: handleWink(); break;
    case EyeExpression::BORING: handleBoring(); break;
  }
}