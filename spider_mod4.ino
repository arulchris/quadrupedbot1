/* -----------------------------------------------------------------------------
  - Project: Remote control Crawling robot
  - Initial Author:  panerqiang@sunfounder.com
   -----------------------------------------------------------------------------
  - Overview
  - This project was written for the Crawling robot desigened by Sunfounder.
    This version of the robot has 4 legs, and each leg is driven by 3 servos.
  This robot is driven by a Ardunio Nano Board with an expansion Board.
  We recommend that you view the product documentation before using.
  - Request
  - This project requires some library files, which you can find in the head of
    this file. Make sure you have installed these files.
  - How to
  - Before use,you must to adjust the robot,in order to make it more accurate.
    - Adjustment operation
    1.uncomment ADJUST, make and run
    2.comment ADJUST, uncomment VERIFY
    3.measure real sites and set to real_site[4][3], make and run
    4.comment VERIFY, make and run
  The document describes in detail how to operate.
   ---------------------------------------------------------------------------*/

// modified by Sparklers for smartphone controlled Crawling robot, 3/30/2021
// Further modified by Arul Christopher, 15th Feb 2025

/* Includes ------------------------------------------------------------------*/
#include <Servo.h>    //to define and control servos
#include "FlexiTimer2.h"//to set a timer to manage all servos
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display
Adafruit_SSD1306* display;
const uint8_t SCREEN_WIDTH = 128;
const uint8_t SCREEN_HEIGHT = 32;
const uint8_t EYE_WIDTH = 16;
const uint8_t EYE_HEIGHT = 16;
const uint8_t EYE_RADIUS = 4;
const uint8_t EYE_SPACING = 12;
int16_t eyeY;

// Animation
enum class EyeState : uint8_t {
  NORMAL,
  BLINK,
  SLEEP,
  HAPPY,
  LOOK_LEFT,
  LOOK_RIGHT
};

EyeState currentState = EyeState::NORMAL;
uint8_t animStep = 0;
unsigned long lastEyeUpdate = 0;
const uint8_t ANIM_STEPS = 8;
const uint8_t ANIM_INTERVAL = 33;
//const int8_t EYE_OFFSET = 24;  // Offset for looking left/right

// Automatic blinking
unsigned long lastBlink = 0;
const unsigned long BLINK_INTERVAL = 3000;

const int8_t EYE_OFFSET = 24;  // Make this more noticeable
int16_t currentEyeOffset = 0;
int16_t targetEyeOffset = 0;
const uint8_t EYE_MOVE_SPEED = 3; // Speed up the movement slightly

unsigned long eyeReturnTime = 0;
const unsigned long EYE_RETURN_DELAY = 4000; // 3 seconds before returning to center

// Constants for idle behavior
const unsigned long IDLE_TIMEOUT = 5000;  // 10 seconds before idle activities start
const unsigned long IDLE_ACTION_INTERVAL = 5000; // 5 seconds between idle actions
unsigned long lastActivityTime = 0;
unsigned long lastIdleActionTime = 0;
bool isIdle = false;



void setupEyes() {
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
  if(!display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  display->clearDisplay();
  display->display();
  eyeY = SCREEN_HEIGHT/2 - EYE_HEIGHT/2;
}

void drawEyes(uint8_t height = EYE_HEIGHT, int8_t extraOffset = 0) {
  display->clearDisplay();
  
  // Use currentEyeOffset for the actual position
  int16_t leftEyeX = SCREEN_WIDTH/2 - EYE_SPACING - EYE_WIDTH + currentEyeOffset;
  int16_t rightEyeX = SCREEN_WIDTH/2 + EYE_SPACING + currentEyeOffset;
  
  // Left eye
  display->fillRoundRect(
    leftEyeX,
    eyeY,
    EYE_WIDTH,
    height,
    EYE_RADIUS,
    SSD1306_WHITE
  );
  
  // Right eye
  display->fillRoundRect(
    rightEyeX,
    eyeY,
    EYE_WIDTH,
    height,
    EYE_RADIUS,
    SSD1306_WHITE
  );
  
  //  // Draw curved smile for HAPPY expression
  // if (currentState == EyeState::HAPPY) {
  //   // Calculate smile position (centered between eyes, slightly below)
  //   int16_t smileWidth = EYE_SPACING+5;  // Width of the smile
  //   int16_t smileX = SCREEN_WIDTH/2 - smileWidth/2 + currentEyeOffset;
  //   int16_t smileY = eyeY + EYE_HEIGHT + 2;  // 4 pixels below eyes
    
  //   // Draw curved smile using multiple small lines
  //   for (int i = 0; i < smileWidth/2; i++) {
  //     // Calculate y-offset using a simple parabola
  //     int8_t yOffset = i * i / 8;  // Adjust divisor to change curve steepness
  //     display->drawPixel(smileX + i, smileY + yOffset, SSD1306_WHITE);
  //     display->drawPixel(smileX + smileWidth - i - 1, smileY + yOffset, SSD1306_WHITE);
  //   }
  // }

  display->display();
}



void updateEyes() {
  if (millis() - lastEyeUpdate < ANIM_INTERVAL) return;
  lastEyeUpdate = millis();

  // Check if we need to auto-return eyes to center
  if ((currentState == EyeState::LOOK_LEFT || currentState == EyeState::LOOK_RIGHT) && 
      millis() > eyeReturnTime) {
    currentState = EyeState::NORMAL;
    targetEyeOffset = 0;
  }

  // Handle eye position animation
  if (currentEyeOffset < targetEyeOffset) {
    currentEyeOffset += EYE_MOVE_SPEED;
    if (currentEyeOffset > targetEyeOffset) currentEyeOffset = targetEyeOffset;
  } else if (currentEyeOffset > targetEyeOffset) {
    currentEyeOffset -= EYE_MOVE_SPEED;
    if (currentEyeOffset < targetEyeOffset) currentEyeOffset = targetEyeOffset;
  }
  
  // Auto blink
  if (currentState == EyeState::NORMAL && 
      millis() - lastBlink >= BLINK_INTERVAL) {
    currentState = EyeState::BLINK;
    animStep = 0;
    lastBlink = millis();
  }
  
  switch(currentState) {
    case EyeState::NORMAL:
      drawEyes();
      break;
      
    case EyeState::BLINK:
      if (animStep < ANIM_STEPS/2) {
        drawEyes(map(animStep, 0, ANIM_STEPS/2, EYE_HEIGHT, 2));
      } else {
        drawEyes(map(animStep - ANIM_STEPS/2, 0, ANIM_STEPS/2, 2, EYE_HEIGHT));
      }
      if (++animStep >= ANIM_STEPS) {
        currentState = EyeState::NORMAL;
        animStep = 0;
      }
      break;
      
    case EyeState::SLEEP:
      if (animStep < ANIM_STEPS/2) {
        drawEyes(map(animStep, 0, ANIM_STEPS/2, EYE_HEIGHT, 2));
      } else {
        display->clearDisplay();
        int16_t leftX = SCREEN_WIDTH/2 - EYE_SPACING - EYE_WIDTH + currentEyeOffset;
        int16_t rightX = SCREEN_WIDTH/2 + EYE_SPACING + currentEyeOffset;
        display->drawLine(leftX, eyeY + EYE_HEIGHT, leftX + EYE_WIDTH, eyeY + EYE_HEIGHT, SSD1306_WHITE);
        display->drawLine(rightX, eyeY + EYE_HEIGHT, rightX + EYE_WIDTH, eyeY + EYE_HEIGHT, SSD1306_WHITE);
        display->display();
      }
      if (++animStep >= ANIM_STEPS) animStep = ANIM_STEPS;
      break;
    
    case EyeState::HAPPY:
      drawEyes(EYE_HEIGHT/3);
      break;

    case EyeState::LOOK_LEFT:
    case EyeState::LOOK_RIGHT:
      drawEyes();
      break;
  }
}

void setEyeState(EyeState state) {
  currentState = state;
  animStep = 0;
  
  switch(state) {
    case EyeState::LOOK_RIGHT:  //EyeState::LOOK_LEFT:
      targetEyeOffset = -EYE_OFFSET;
      eyeReturnTime = millis() + EYE_RETURN_DELAY;
      break;
    case EyeState::LOOK_LEFT: //EyeState::LOOK_RIGHT:
      targetEyeOffset = EYE_OFFSET;
      eyeReturnTime = millis() + EYE_RETURN_DELAY;
      break;
    default:
      targetEyeOffset = 0;
      break;
  }
  
  if (state == EyeState::BLINK) {
    lastBlink = millis();
  }
}


/* Servos --------------------------------------------------------------------*/
//define 12 servos for 4 legs
char data = 0;
Servo servo[4][3];
//define servos' ports
const int servo_pin[4][3] = { {2, 3, 4}, {5, 6, 7}, {8, 9, 10}, {11, 12, 13} };
/* Size of the robot ---------------------------------------------------------*/
 
 //WORKING OK
//  const float length_a = 58; //femur
//  const float length_b = 80; //tibia

// //const float length_a = 55;
// //const float length_b = 77.5;
// const float length_c = 35; //coxa
// const float length_side = 74;


  const float length_a = 58; //femur
  const float length_b = 80; //tibia
 const float length_c = 32; //coxa
 const float length_side = 72; //distance between legs

const float z_absolute = -28+2;
/* Constants for movement ----------------------------------------------------*/
const float z_default = -50, z_up = -30, z_boot = z_absolute;
const float x_default = 62, x_offset = 0;
const float y_start = 0, y_step = 40;
const float y_default = x_default;
/* variables for movement ----------------------------------------------------*/
volatile float site_now[4][3];    //real-time coordinates of the end of each leg
volatile float site_expect[4][3]; //expected coordinates of the end of each leg
float temp_speed[4][3];   //each axis' speed, needs to be recalculated before each movement
float move_speed;     //movement speed
float speed_multiple = 1; //movement speed multiple
const float spot_turn_speed = 4;
const float leg_move_speed = 8;
const float body_move_speed = 3;
const float stand_seat_speed = 1;
volatile int rest_counter;      //+1/0.02s, for automatic rest
//functions' parameter
const float KEEP = 255;
//define PI for calculation
const float pi = 3.1415926;
/* Constants for turn --------------------------------------------------------*/
//temp length
const float temp_a = sqrt(pow(2 * x_default + length_side, 2) + pow(y_step, 2));
const float temp_b = 2 * (y_start + y_step) + length_side;
const float temp_c = sqrt(pow(2 * x_default + length_side, 2) + pow(2 * y_start + y_step + length_side, 2));
const float temp_alpha = acos((pow(temp_a, 2) + pow(temp_b, 2) - pow(temp_c, 2)) / 2 / temp_a / temp_b);
//site for turn
const float turn_x1 = (temp_a - length_side) / 2;
const float turn_y1 = y_start + y_step / 2;
const float turn_x0 = turn_x1 - temp_b * cos(temp_alpha);
const float turn_y0 = temp_b * sin(temp_alpha) - turn_y1 - length_side;

int n_step = 2;
int s_flag=1;

void performCuteWiggle(int cycles) {
  move_speed = body_move_speed;
  
  // Start from standing position
  if (site_now[0][2] != z_default) {
    stand();
    delay(500);
  }
  
  setEyeState(EyeState::HAPPY);
  
  for (int i = 0; i < cycles; i++) {
    // Wiggle left
    body_left(15);
    delay(300);
    
    // Wiggle right
    body_right(30);
    delay(300);
    
    // Back to center
    body_left(15);
    delay(300);
  }
  
  setEyeState(EyeState::NORMAL);
  //updateEyes();
}

void performPeekaboo(int cycles) {
  move_speed = body_move_speed;
  
  // Start from standing position
  if (site_now[0][2] != z_default) {
    stand();
    delay(500);
  }
  
  for (int i = 0; i < cycles; i++) {
    // Hide (crouch down)
    setEyeState(EyeState::SLEEP);
   // Multiple updates to ensure the state change is visible
    for(int j = 0; j < 5; j++) {
      updateEyes();
      delay(50);
    }
        
    for (int leg = 0; leg < 4; leg++) {
      set_site(leg, KEEP, KEEP, z_boot + 20);
    }
    wait_all_reach();
    delay(700);
    
    // Pop up and say peek-a-boo!
    setEyeState(EyeState::NORMAL);
    updateEyes();  // Force immediate update
    for (int leg = 0; leg < 4; leg++) {
      set_site(leg, KEEP, KEEP, z_default - 10);
    }
    wait_all_reach();
    delay(500);
    
    // Return to normal height
    for (int leg = 0; leg < 4; leg++) {
      set_site(leg, KEEP, KEEP, z_default);
    }
    wait_all_reach();
    delay(500);
  }
  setEyeState(EyeState::NORMAL);
}

void performPushUps(int count) {
  move_speed = stand_seat_speed;
  // Start from standing position
  if (site_now[0][2] != z_default) {
    stand();
    delay(500);
  }
  setEyeState(EyeState::HAPPY);
  for (int i = 0; i < count; i++) {
    // Go down slowly
    for (int leg = 0; leg < 4; leg++) {
      set_site(leg, KEEP, KEEP, z_boot + 20);  // Not fully down
    }
    wait_all_reach();
    delay(300);
    // Push up with slight forward tilt
    head_up(15);
    for (int leg = 0; leg < 4; leg++) {
      set_site(leg, KEEP, KEEP, z_default - 10);  // Slightly higher than normal
    }
    wait_all_reach();
    delay(300);
    // Return to normal height
    head_down(15);
    for (int leg = 0; leg < 4; leg++) {
      set_site(leg, KEEP, KEEP, z_default);
    }
    wait_all_reach();
    delay(200);
  }
  setEyeState(EyeState::NORMAL);

}

void performRandomIdleAction() {
  // Skip if robot is sitting or already performing an action
  if (site_now[0][2] == z_boot) return;  // Check if sitting

  uint8_t action = random(0, 9);  // Random number between 0-7
 
  switch(action) {
    case 0:
      // Look right then left
      setEyeState(EyeState::LOOK_RIGHT);
      turn_right(1);
      delay(300);
      setEyeState(EyeState::LOOK_LEFT);
      delay(300);
      setEyeState(EyeState::NORMAL);
      break;     

    case 1:
      // Look left then right
      setEyeState(EyeState::LOOK_LEFT);
      turn_left(1);
      delay(300);
      setEyeState(EyeState::LOOK_RIGHT);
      break;

    case 2:
      // Quick head movements
      Serial.println(F("head movements"));
      head_up(10);
      delay(500);
      head_down(10);
      break;

    case 3:
      // Diagonal head movement
      Serial.println(F("Diagonal head movement"));
      head_up(10);
      body_right(10);
      delay(300);
      head_down(10);
      body_left(10);
      break;

    case 4:
      // Wave with head tilt
      Serial.println(F("Wave with head tilt"));
      head_up(10);
      hand_wave(1);
      head_down(10);
      break;

    case 5:
      // Push-ups
      Serial.println(F("Doing push-ups"));
      performPushUps(3);  // Do 2 push-ups
      break;

    case 6:
      // Shake with side lean
      Serial.println(F("Shake with side lean"));
      body_right(15);
      hand_shake(1);
      body_left(15);
      break;
    case 7:
    // Cute wiggle
      Serial.println(F("Doing cute wiggle"));
      performCuteWiggle(2);
      break;
    case 8:
      // Peekaboo
      Serial.println(F("Playing peekaboo"));
      performPeekaboo(1);
      break;
    //case 9:
    
    default:
      // Simple look around
      setEyeState(EyeState::LOOK_LEFT);
      delay(400);
      setEyeState(EyeState::LOOK_RIGHT);
      delay(400);
      setEyeState(EyeState::NORMAL);
      break;
  }
}

void updateIdleState() {
  // Reset idle timer when receiving commands
  if (Serial.available() > 0) {
    lastActivityTime = millis();
    isIdle = false;
    return;
  }

  // Check if we should enter idle state
  if (!isIdle && (millis() - lastActivityTime >= IDLE_TIMEOUT)) {
    isIdle = true;
    lastIdleActionTime = millis();
  }

  // Perform random actions when idle
  if (isIdle && (millis() - lastIdleActionTime >= IDLE_ACTION_INTERVAL)) {
    performRandomIdleAction();
    lastIdleActionTime = millis();
  }
}

/* ---------------------------------------------------------------------------*/

/*
  - setup function
   ---------------------------------------------------------------------------*/
void setup()
{
  //start serial for debug
  Serial.begin(9600);
  Serial.println(F("Robot starts initialization"));
  //initialize default parameter
  pinMode(14, OUTPUT);
  set_site(0, x_default - x_offset, y_start + y_step, z_boot);
  set_site(1, x_default - x_offset, y_start + y_step, z_boot);
  set_site(2, x_default + x_offset, y_start, z_boot);
  set_site(3, x_default + x_offset, y_start, z_boot);
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      site_now[i][j] = site_expect[i][j];
    }
  }
  //start servo service
  FlexiTimer2::set(20, servo_service);
  FlexiTimer2::start();
  Serial.println(F("Servo service started"));
  //initialize servos
  servo_attach();
  Serial.println(F("Servos initialized"));
  // //Eyes Initialized
  // if (!eyes.begin()) {
  //   while (1); // Failed to initialize
  // }
  // // Optional: customize eye shape
  
  // eyes.setEyeShape(18, 18, 4);  // width, height, radius
  // eyes.setExpression(curExp);

  Serial.println(F("Robot initialization Complete"));
   setupEyes();
  lastActivityTime = millis();
}

void servo_attach(void)
{
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      servo[i][j].attach(servo_pin[i][j]);
      delay(100);
    }
  }
}

void servo_detach(void)
{
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      servo[i][j].detach();
      delay(100);
    }
  }
}
/*
  - loop function
   ---------------------------------------------------------------------------*/
void loop()
{
  updateEyes();
  updateIdleState();
  // Example: Blink every 3 seconds
  // static unsigned long lastBlink = 0;
  // if (millis() - lastBlink >= 3000 && eyes.isAnimationComplete()) {
  //   eyes.setExpression(curExp);
  //   //curExp=EyeExpression::BLINK;
  //   lastBlink = millis();
  // }

  if(Serial.available() > 0)      
   {
    lastActivityTime = millis();  // Reset idle timer on any command
    isIdle = false;
    
      data = Serial.read();        
      Serial.print(data);          
      Serial.print("\n");        
      if(data == 'F') 
        { 
         Serial.println(F("Step forward"));
         setEyeState(EyeState::HAPPY);    
         step_forward(2);
          //setEyeState(EyeState::NORMAL);
        }
      else if(data == 'B')        
         { 
          Serial.println(F("Step back"));
          //eyes.setExpression(EyeExpression::BORING,5000);
          step_back(2);
          
        }    
      else if(data == 'L')        
         { 
          setEyeState(EyeState::LOOK_LEFT);
         Serial.println(F("Turn left"));
         turn_left(2);
        }
      else if(data == 'R')        
        { 
          setEyeState(EyeState::LOOK_RIGHT);
          Serial.println(F("Turn right"));
          turn_right(2);
        } 
        else if(data == 'X')
        {
          setEyeState(EyeState::NORMAL);
         Serial.println(F("Stand"));
         stand();
        }
         else if(data == 'x')
        {
          setEyeState(EyeState::SLEEP);
          Serial.println(F("Sit"));
          sit();   
        }
       else if(data == 'S' ||data == 'D' )        
        { 
          
        }
         else if(data == 'W')        
        { 
          digitalWrite(14, HIGH);
        } 
          else if(data == 'w')        
        { 
         digitalWrite(14, LOW);
        }
        else if(data == 'V')        
        { 
          Serial.println(F("Hand wave"));
          hand_shake(3);
        } 
          else if(data == 'v')        
        { 
         Serial.println(F("Hand wave"));
         //hand_shake(3);
          hand_wave(3);
        }
         else if(data == 'U')        
        { 
          Serial.println(F("Body dance"));
          //body_dance(10);
          performCuteWiggle(2);
        } 
          else if(data == 'u')        
        { 
          Serial.println(F("Body dance"));
          //body_dance(10);
           performPeekaboo(1);
        }
        else if(data == '1')        
        { 
         performRandomIdleAction();
         //performCuteWiggle(2);
        }
         else if(data == '2')        
        { 
        performPeekaboo(1);
        }
        while(Serial.available()) {Serial.read();}
   }
}

/*
  - sit
  - blocking function
   ---------------------------------------------------------------------------*/
void sit(void)
{
  move_speed = stand_seat_speed;
  for (int leg = 0; leg < 4; leg++)
  {
    set_site(leg, KEEP, KEEP, z_boot);
  }
  wait_all_reach();
}

/*
  - stand
  - blocking function
   ---------------------------------------------------------------------------*/
void stand(void)
{
  move_speed = stand_seat_speed;
  for (int leg = 0; leg < 4; leg++)
  {
    set_site(leg, KEEP, KEEP, z_default);
  }
  wait_all_reach();
}


/*
  - spot turn to left
  - blocking function
  - parameter step steps wanted to turn
   ---------------------------------------------------------------------------*/
void turn_left(unsigned int step)
{
    move_speed = spot_turn_speed;
    while (step-- >0){
    if (site_now[3][1] == y_start)
    {
      //leg 3&1 move
      set_site(3, x_default + x_offset, y_start, z_up);
      wait_all_reach();

      set_site(0, turn_x1 - x_offset, turn_y1, z_default);
      set_site(1, turn_x0 - x_offset, turn_y0, z_default);
      set_site(2, turn_x1 + x_offset, turn_y1, z_default);
      set_site(3, turn_x0 + x_offset, turn_y0, z_up);
      wait_all_reach();

      set_site(3, turn_x0 + x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(0, turn_x1 + x_offset, turn_y1, z_default);
      set_site(1, turn_x0 + x_offset, turn_y0, z_default);
      set_site(2, turn_x1 - x_offset, turn_y1, z_default);
      set_site(3, turn_x0 - x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(1, turn_x0 + x_offset, turn_y0, z_up);
      wait_all_reach();

      set_site(0, x_default + x_offset, y_start, z_default);
      set_site(1, x_default + x_offset, y_start, z_up);
      set_site(2, x_default - x_offset, y_start + y_step, z_default);
      set_site(3, x_default - x_offset, y_start + y_step, z_default);
      wait_all_reach();

      set_site(1, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
    else
    {
      //leg 0&2 move
      set_site(0, x_default + x_offset, y_start, z_up);
      wait_all_reach();

      set_site(0, turn_x0 + x_offset, turn_y0, z_up);
      set_site(1, turn_x1 + x_offset, turn_y1, z_default);
      set_site(2, turn_x0 - x_offset, turn_y0, z_default);
      set_site(3, turn_x1 - x_offset, turn_y1, z_default);
      wait_all_reach();

      set_site(0, turn_x0 + x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(0, turn_x0 - x_offset, turn_y0, z_default);
      set_site(1, turn_x1 - x_offset, turn_y1, z_default);
      set_site(2, turn_x0 + x_offset, turn_y0, z_default);
      set_site(3, turn_x1 + x_offset, turn_y1, z_default);
      wait_all_reach();

      set_site(2, turn_x0 + x_offset, turn_y0, z_up);
      wait_all_reach();

      set_site(0, x_default - x_offset, y_start + y_step, z_default);
      set_site(1, x_default - x_offset, y_start + y_step, z_default);
      set_site(2, x_default + x_offset, y_start, z_up);
      set_site(3, x_default + x_offset, y_start, z_default);
      wait_all_reach();

      set_site(2, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
  }
}

/*
  - spot turn to right
  - blocking function
  - parameter step steps wanted to turn
   ---------------------------------------------------------------------------*/
void turn_right(unsigned int step)
{
    move_speed = spot_turn_speed;
while (step-- >0)
{
    if (site_now[2][1] == y_start)
    {
      //leg 2&0 move
      set_site(2, x_default + x_offset, y_start, z_up);
      wait_all_reach();

      set_site(0, turn_x0 - x_offset, turn_y0, z_default);
      set_site(1, turn_x1 - x_offset, turn_y1, z_default);
      set_site(2, turn_x0 + x_offset, turn_y0, z_up);
      set_site(3, turn_x1 + x_offset, turn_y1, z_default);
      wait_all_reach();

      set_site(2, turn_x0 + x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(0, turn_x0 + x_offset, turn_y0, z_default);
      set_site(1, turn_x1 + x_offset, turn_y1, z_default);
      set_site(2, turn_x0 - x_offset, turn_y0, z_default);
      set_site(3, turn_x1 - x_offset, turn_y1, z_default);
      wait_all_reach();

      set_site(0, turn_x0 + x_offset, turn_y0, z_up);
      wait_all_reach();

      set_site(0, x_default + x_offset, y_start, z_up);
      set_site(1, x_default + x_offset, y_start, z_default);
      set_site(2, x_default - x_offset, y_start + y_step, z_default);
      set_site(3, x_default - x_offset, y_start + y_step, z_default);
      wait_all_reach();

      set_site(0, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
    else
    {
      //leg 1&3 move
      set_site(1, x_default + x_offset, y_start, z_up);
      wait_all_reach();

      set_site(0, turn_x1 + x_offset, turn_y1, z_default);
      set_site(1, turn_x0 + x_offset, turn_y0, z_up);
      set_site(2, turn_x1 - x_offset, turn_y1, z_default);
      set_site(3, turn_x0 - x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(1, turn_x0 + x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(0, turn_x1 - x_offset, turn_y1, z_default);
      set_site(1, turn_x0 - x_offset, turn_y0, z_default);
      set_site(2, turn_x1 + x_offset, turn_y1, z_default);
      set_site(3, turn_x0 + x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(3, turn_x0 + x_offset, turn_y0, z_up);
      wait_all_reach();

      set_site(0, x_default - x_offset, y_start + y_step, z_default);
      set_site(1, x_default - x_offset, y_start + y_step, z_default);
      set_site(2, x_default + x_offset, y_start, z_default);
      set_site(3, x_default + x_offset, y_start, z_up);
      wait_all_reach();

      set_site(3, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
  }
}

/*
  - go forward
  - blocking function
  - parameter step steps wanted to go
   ---------------------------------------------------------------------------*/
void step_forward(unsigned int step)
{
    move_speed = leg_move_speed;
  while (step-- >0){

    if (site_now[2][1] == y_start)
    {
      //leg 2&1 move
      set_site(2, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(2, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(2, x_default + x_offset, y_start + 2 * y_step, z_default);
      wait_all_reach();

      move_speed = body_move_speed;

      set_site(0, x_default + x_offset, y_start, z_default);
      set_site(1, x_default + x_offset, y_start + 2 * y_step, z_default);
      set_site(2, x_default - x_offset, y_start + y_step, z_default);
      set_site(3, x_default - x_offset, y_start + y_step, z_default);
      wait_all_reach();

      move_speed = leg_move_speed;

      set_site(1, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(1, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(1, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
    else
    {
      //leg 0&3 move
      set_site(0, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(0, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(0, x_default + x_offset, y_start + 2 * y_step, z_default);
      wait_all_reach();

      move_speed = body_move_speed;

      set_site(0, x_default - x_offset, y_start + y_step, z_default);
      set_site(1, x_default - x_offset, y_start + y_step, z_default);
      set_site(2, x_default + x_offset, y_start, z_default);
      set_site(3, x_default + x_offset, y_start + 2 * y_step, z_default);
      wait_all_reach();

      move_speed = leg_move_speed;

      set_site(3, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(3, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(3, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
  }
}

/*
  - go back
  - blocking function
  - parameter step steps wanted to go
   ---------------------------------------------------------------------------*/
void step_back(unsigned int step)
{
    move_speed = leg_move_speed;
    while (step-- >0 ){
    if (site_now[3][1] == y_start)
    {
      //leg 3&0 move
      set_site(3, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(3, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(3, x_default + x_offset, y_start + 2 * y_step, z_default);
      wait_all_reach();

      move_speed = body_move_speed;

      set_site(0, x_default + x_offset, y_start + 2 * y_step, z_default);
      set_site(1, x_default + x_offset, y_start, z_default);
      set_site(2, x_default - x_offset, y_start + y_step, z_default);
      set_site(3, x_default - x_offset, y_start + y_step, z_default);
      wait_all_reach();

      move_speed = leg_move_speed;

      set_site(0, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(0, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(0, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
    else
    {
      //leg 1&2 move
      set_site(1, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(1, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(1, x_default + x_offset, y_start + 2 * y_step, z_default);
      wait_all_reach();

      move_speed = body_move_speed;

      set_site(0, x_default - x_offset, y_start + y_step, z_default);
      set_site(1, x_default - x_offset, y_start + y_step, z_default);
      set_site(2, x_default + x_offset, y_start + 2 * y_step, z_default);
      set_site(3, x_default + x_offset, y_start, z_default);
      wait_all_reach();

      move_speed = leg_move_speed;

      set_site(2, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(2, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(2, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
  }
}

// add by RegisHsu

void body_left(int i)
{
  set_site(0, site_now[0][0] + i, KEEP, KEEP);
  set_site(1, site_now[1][0] + i, KEEP, KEEP);
  set_site(2, site_now[2][0] - i, KEEP, KEEP);
  set_site(3, site_now[3][0] - i, KEEP, KEEP);
  wait_all_reach();
}

void body_right(int i)
{
  set_site(0, site_now[0][0] - i, KEEP, KEEP);
  set_site(1, site_now[1][0] - i, KEEP, KEEP);
  set_site(2, site_now[2][0] + i, KEEP, KEEP);
  set_site(3, site_now[3][0] + i, KEEP, KEEP);
  wait_all_reach();
}

void hand_wave(int i)
{
  float x_tmp;
  float y_tmp;
  float z_tmp;
  move_speed = 1;
  if (site_now[3][1] == y_start)
  {
    body_right(15);
    x_tmp = site_now[2][0];
    y_tmp = site_now[2][1];
    z_tmp = site_now[2][2];
    move_speed = body_move_speed;
    for (int j = 0; j < i; j++)
    {
      set_site(2, turn_x1, turn_y1, 50);
      wait_all_reach();
      set_site(2, turn_x0, turn_y0, 50);
      wait_all_reach();
    }
    set_site(2, x_tmp, y_tmp, z_tmp);
    wait_all_reach();
    move_speed = 1;
    body_left(15);
  }
  else
  {
    body_left(15);
    x_tmp = site_now[0][0];
    y_tmp = site_now[0][1];
    z_tmp = site_now[0][2];
    move_speed = body_move_speed;
    for (int j = 0; j < i; j++)
    {
      set_site(0, turn_x1, turn_y1, 50);
      wait_all_reach();
      set_site(0, turn_x0, turn_y0, 50);
      wait_all_reach();
    }
    set_site(0, x_tmp, y_tmp, z_tmp);
    wait_all_reach();
    move_speed = 1;
    body_right(15);
  }
}

void hand_shake(int i)
{
  float x_tmp;
  float y_tmp;
  float z_tmp;
  move_speed = 1;
  if (site_now[3][1] == y_start)
  {
    body_right(15);
    x_tmp = site_now[2][0];
    y_tmp = site_now[2][1];
    z_tmp = site_now[2][2];
    move_speed = body_move_speed;
    for (int j = 0; j < i; j++)
    {
      set_site(2, x_default - 30, y_start + 2 * y_step, 55);
      wait_all_reach();
      set_site(2, x_default - 30, y_start + 2 * y_step, 10);
      wait_all_reach();
    }
    set_site(2, x_tmp, y_tmp, z_tmp);
    wait_all_reach();
    move_speed = 1;
    body_left(15);
  }
  else
  {
    body_left(15);
    x_tmp = site_now[0][0];
    y_tmp = site_now[0][1];
    z_tmp = site_now[0][2];
    move_speed = body_move_speed;
    for (int j = 0; j < i; j++)
    {
      set_site(0, x_default - 30, y_start + 2 * y_step, 55);
      wait_all_reach();
      set_site(0, x_default - 30, y_start + 2 * y_step, 10);
      wait_all_reach();
    }
    set_site(0, x_tmp, y_tmp, z_tmp);
    wait_all_reach();
    move_speed = 1;
    body_right(15);
  }
}

void head_up(int i)
{
  set_site(0, KEEP, KEEP, site_now[0][2] - i);
  set_site(1, KEEP, KEEP, site_now[1][2] + i);
  set_site(2, KEEP, KEEP, site_now[2][2] - i);
  set_site(3, KEEP, KEEP, site_now[3][2] + i);
  wait_all_reach();
}

void head_down(int i)
{
  set_site(0, KEEP, KEEP, site_now[0][2] + i);
  set_site(1, KEEP, KEEP, site_now[1][2] - i);
  set_site(2, KEEP, KEEP, site_now[2][2] + i);
  set_site(3, KEEP, KEEP, site_now[3][2] - i);
  wait_all_reach();
}

// void body_dance(int i)
// {
//   float x_tmp;
//   float y_tmp;
//   float z_tmp;
//   float body_dance_speed = 2;
//   sit();
//   move_speed = 1;
//   set_site(0, x_default, y_default, KEEP);
//   set_site(1, x_default, y_default, KEEP);
//   set_site(2, x_default, y_default, KEEP);
//   set_site(3, x_default, y_default, KEEP);
//   wait_all_reach();
//   stand();
//   set_site(0, x_default, y_default, z_default - 20);
//   set_site(1, x_default, y_default, z_default - 20);
//   set_site(2, x_default, y_default, z_default - 20);
//   set_site(3, x_default, y_default, z_default - 20);
//   wait_all_reach();
//   move_speed = body_dance_speed;
//   head_up(30);
//   for (int j = 0; j < i; j++)
//   {
//     if (j > i / 4)
//       move_speed = body_dance_speed * 2;
//     if (j > i / 2)
//       move_speed = body_dance_speed * 3;
//     set_site(0, KEEP, y_default - 20, KEEP);
//     set_site(1, KEEP, y_default + 20, KEEP);
//     set_site(2, KEEP, y_default - 20, KEEP);
//     set_site(3, KEEP, y_default + 20, KEEP);
//     wait_all_reach();
//     set_site(0, KEEP, y_default + 20, KEEP);
//     set_site(1, KEEP, y_default - 20, KEEP);
//     set_site(2, KEEP, y_default + 20, KEEP);
//     set_site(3, KEEP, y_default - 20, KEEP);
//     wait_all_reach();
//   }
//   move_speed = body_dance_speed;
//   head_down(30);
// }


/*
  - microservos service /timer interrupt function/50Hz
  - when set site expected,this function move the end point to it in a straight line
  - temp_speed[4][3] should be set before set expect site,it make sure the end point
   move in a straight line,and decide move speed.
   ---------------------------------------------------------------------------*/
void servo_service(void)
{
  sei();
  static float alpha, beta, gamma;

  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (abs(site_now[i][j] - site_expect[i][j]) >= abs(temp_speed[i][j]))
        site_now[i][j] += temp_speed[i][j];
      else
        site_now[i][j] = site_expect[i][j];
    }

    cartesian_to_polar(alpha, beta, gamma, site_now[i][0], site_now[i][1], site_now[i][2]);
    polar_to_servo(i, alpha, beta, gamma);
  }

  rest_counter++;
}

/*
  - set one of end points' expect site
  - this founction will set temp_speed[4][3] at same time
  - non - blocking function
   ---------------------------------------------------------------------------*/
void set_site(int leg, float x, float y, float z)
{
  float length_x = 0, length_y = 0, length_z = 0;

  if (x != KEEP)
    length_x = x - site_now[leg][0];
  if (y != KEEP)
    length_y = y - site_now[leg][1];
  if (z != KEEP)
    length_z = z - site_now[leg][2];

  float length = sqrt(pow(length_x, 2) + pow(length_y, 2) + pow(length_z, 2));

  temp_speed[leg][0] = length_x / length * move_speed * speed_multiple;
  temp_speed[leg][1] = length_y / length * move_speed * speed_multiple;
  temp_speed[leg][2] = length_z / length * move_speed * speed_multiple;

  if (x != KEEP)
    site_expect[leg][0] = x;
  if (y != KEEP)
    site_expect[leg][1] = y;
  if (z != KEEP)
    site_expect[leg][2] = z;
}

/*
  - wait one of end points move to expect site
  - blocking function
   ---------------------------------------------------------------------------*/
void wait_reach(int leg)
{
  while (1)
    if (site_now[leg][0] == site_expect[leg][0])
      if (site_now[leg][1] == site_expect[leg][1])
        if (site_now[leg][2] == site_expect[leg][2])
          break;
}

/*
  - wait all of end points move to expect site
  - blocking function
   ---------------------------------------------------------------------------*/
void wait_all_reach(void)
{
  updateEyes();  // Update eyes while waiting
  for (int i = 0; i < 4; i++)
    wait_reach(i);
}

/*
  - trans site from cartesian to polar
  - mathematical model 2/2
   ---------------------------------------------------------------------------*/
void cartesian_to_polar(volatile float &alpha, volatile float &beta, volatile float &gamma, volatile float x, volatile float y, volatile float z)
{
  //calculate w-z degree
  float v, w;
  w = (x >= 0 ? 1 : -1) * (sqrt(pow(x, 2) + pow(y, 2)));
  v = w - length_c;
  alpha = atan2(z, v) + acos((pow(length_a, 2) - pow(length_b, 2) + pow(v, 2) + pow(z, 2)) / 2 / length_a / sqrt(pow(v, 2) + pow(z, 2)));
  beta = acos((pow(length_a, 2) + pow(length_b, 2) - pow(v, 2) - pow(z, 2)) / 2 / length_a / length_b);
  //calculate x-y-z degree
  gamma = (w >= 0) ? atan2(y, x) : atan2(-y, -x);
  //trans degree pi->180
  alpha = alpha / pi * 180;
  beta = beta / pi * 180;
  gamma = gamma / pi * 180;
}

/*
  - trans site from polar to microservos
  - mathematical model map to fact
  - the errors saved in eeprom will be add
   ---------------------------------------------------------------------------*/
void polar_to_servo(int leg, float alpha, float beta, float gamma)
{
  if (leg == 0)
  {
    alpha = 90 - alpha;
    beta = beta;
    gamma += 90;
  }
  else if (leg == 1)
  {
    alpha += 90;
    beta = 180 - beta;
    gamma = 90 - gamma;
  }
  else if (leg == 2)
  {
    alpha += 90;
    beta = 180 - beta;
    gamma = 90 - gamma;
  }
  else if (leg == 3)
  {
    alpha = 90 - alpha;
    beta = beta;
    gamma += 90;
  }

  servo[leg][0].write(alpha);
  servo[leg][1].write(beta);
  servo[leg][2].write(gamma);
}
