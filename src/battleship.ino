bool myTurn = true; // set to false for Player 2
bool isPlayerOne = true; // set to false for Player 2 - determines who goes first after reset

// const values corresponding to joystick position
const int LEFT = 0;
const int RIGHT = 1;
const int UP = 2;
const int DOWN = 3;
const int CENTER = 4;

// labeled pins for 7-segment display
const int pinA = 13;
const int pinB = 12;

// pin designation for joystick and button
const int xpotPin = A0;
const int ypotPin = A1;
const int buttonPin = 11;

// threshold value to constitute valid joystick move
const int THRESHOLD = 200;
const int ORIGIN = 512;

// reset confirmation protocol constants
const int RESET_REQUEST = 99;      // request to reset
const int RESET_CONFIRM = 97;      // confirmation that reset is ready
const int GAME_WON = 98;           // game won signal


int joystickPos = CENTER;             // stores the joystick postion
bool wasLastMoveCentered = true;      // bool storing if last move of joystick was center
bool validMovementDetected = false;   // bool storing if a valid move is detected
int buttonVal;                        // initializing button value variable
int newPosition;                      // holds new position of joystick

const int readyPin = A2;   // pin to input of AND gate to confirm selection
const int readyCheck = A3; // pin from output of AND gate to check if both players have selected

// matrix storing the digital pins the LEDs are connected to (in correct order)
const int LEDGRID [3][3] = {{2, 5, 8}, {3, 6, 9}, {4, 7, 10}};

// used to get value in LEDGRID
int GridLocation[2] = {0, 0};

// determine what LED was last selected
int lastOnLED = LEDGRID[0][0];
int myShipPos;
int myScore = 0;

// list storing state of all LEDs ON/OFF
int selectedList[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
int listMap = lastOnLED - 2;

// flag for shot selection phase
bool selectionPhase = true;
bool shootingPhase = false;
bool isInitialSelection = true; // true for game start/reset, false for post-hit selection
bool gameResetPending = false; // flag to indicate a reset is in progress
bool displayPatternActive = false; // flag to disable centralized display during patterns
int hitmiss;

int receivedTarget; // value received from other arduino VIA UART

// variables for handling LED blinking
unsigned long lastBlinkTime = 0;
bool blinkState = true;
const unsigned long BLINK_INTERVAL = 250;
unsigned long currentTime;

// function prototypes
bool isJoystickCentered();
int getJoystickDirection();
void handleMovement(int direction);
void updateLEDs();
void LEDON (int LEDNumber);
void LEDOFF (int LEDNumber);
void XPattern();
void ShotPattern();
void HitPattern();
void display0();
void display1();
void display2();
void displayS();
void displayScore(int score);
void allLEDOFF();
int readTarget();
void sendTarget(int target);
void performReset();


void setup()
{
  // initialize all used pins
  pinMode(xpotPin, INPUT);
  pinMode(ypotPin, INPUT);
  Serial.begin(9600); // baud rate, UART
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, INPUT);
  pinMode(readyPin, OUTPUT);
  pinMode(readyCheck, INPUT);
  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  displayS();
}

void loop()
{
  // check if a reset is pending, execute it immediately and skip everything else
  if(gameResetPending)
  {
    performReset();
    gameResetPending = false;
    return;
  }

  newPosition = getJoystickDirection();

  buttonVal = digitalRead(buttonPin);
  
  if(buttonVal == HIGH)
  {
    // check for 3 second button press for game reset
    unsigned long buttonPressStart = millis();
    bool resetTriggered = false;
    
    while(digitalRead(buttonPin) == HIGH)
    {
      if(millis() - buttonPressStart >= 3000)
      {
        // start reset confirmation process
        sendTarget(RESET_REQUEST);
        
        // wait for confirmation from opponent
        int response = 0;
        unsigned long waitStart = millis();
        while(millis() - waitStart < 5000) // wait up to 5 seconds for confirmation
        {
          if(Serial.available())
          {
            response = readTarget();
            if(response == RESET_CONFIRM)
            {
              // opponent confirmed, set flag to reset on next loop iteration
              gameResetPending = true;
              resetTriggered = true;
              break;
            }
          }
          delay(10);
        }
        
        // if no confirmation received, cancel reset
        if(!resetTriggered)
        {
          // flash LEDs to indicate reset cancelled
          for(int i = 0; i < 3; i++)
          {
            allLEDOFF();
            delay(200);
            for(int j = 2; j <= 10; j++) 
            {
              LEDON(j);
            }
            delay(200);
          }
            allLEDOFF();
        }
          
        // wait for button release
        while(digitalRead(buttonPin) == HIGH)
        {
          delay(10);
        }
        return; // exit early
      }
      delay(10);
    }
      
    // only process normal button actions if reset was NOT triggered
    if (!resetTriggered)
    {
      if (shootingPhase && myTurn)
      {
        // check if this target was already selected
        listMap = lastOnLED - 2;
        if (selectedList[listMap] == 1)
        {
          // rapidly for 2 seconds if has been selected
          unsigned long flashStart = millis();
          while (millis() - flashStart < 2000)
          {
            LEDON(lastOnLED);
            delay(100);
            LEDOFF(lastOnLED);
            delay(100);
          }
          // continue shot selection
          return;
        }
      
        // player turn to shoot at new target
        selectedList[listMap] = 1;
        ShotPattern();

        // send target to opponent
        sendTarget(lastOnLED);
      
        // wait for hit/miss response
        hitmiss = readTarget();

        // try this commented out to see if this can be removed
        //if (hitmiss == GAME_WON)
        //{
        //  // set flag to reset on next loop iteration
        //  gameResetPending = true;
        //  return; // exit early, reset will happen next loop
        //}

        if (hitmiss == 1)
        {
          // increment score and reset both players to selection phase
          myScore++;
          displayScore(myScore);
          HitPattern();
        
          // check if player reached 2 points (game won)
          if(myScore >= 2)
          {
            // send GAME_WON signal to opponent to reset
            sendTarget(GAME_WON);
            delay(100);
            gameResetPending = true; // set flag to reset on next loop iteration
            return; // exit early reset will happen next loop
          }
        
          // clear selected lists and reset to selection phase
          for(int i = 0; i < 9; i++)
          {
            selectedList[i] = 0;
          }
        
          selectionPhase = true;
          shootingPhase = false;
          isInitialSelection = true; // both players need to select new ships
          myTurn = true; // winner goes first next round
        
          delay(2000);
          allLEDOFF();
        
          // reset cursor to top-left
          GridLocation[0] = 0;
          GridLocation[1] = 0;
          lastOnLED = LEDGRID[0][0];
        }

        else if (hitmiss == 0)
        {
          // display X pattern then switch turns
          XPattern();
          displayScore(myScore);
          myTurn = false;
          delay(1000);
          allLEDOFF();
        }
      }
      else if (selectionPhase && isInitialSelection)
      {
        // ship selection phase - both players can select ships when isInitialSelection=true
        LEDON(lastOnLED);
        myShipPos = lastOnLED;
        digitalWrite(readyPin, HIGH);

        // wait until both players are ready (AND gate output high)
        while(digitalRead(readyCheck) != HIGH)
        {
          delay(10);
        }
        delay(500);
    
        digitalWrite(readyPin, LOW);
    
        // move to shooting phase
        selectionPhase = false;
        shootingPhase = true;
        isInitialSelection = false; // no longer initial selection
    
        // reset cursor to top-left
        GridLocation[0] = 0;
        GridLocation[1] = 0;
        lastOnLED = LEDGRID[0][0];
      }
    } // end of normal button action processing
  
    // wait for button release to prevent multiple triggers
    while(digitalRead(buttonPin) == HIGH) 
    {
      delay(10);
    }
    delay(50); // debounce delay
  }
  
  // handle opponent turn
  if (shootingPhase && !myTurn)
  {
    // wait for opponent shot with validation
    if (Serial.available()) 
    {
      receivedTarget = 0;
      // wait for valid shot data, reset request, confirm, or game won signal
      while(receivedTarget < 2 || (receivedTarget > 10 && receivedTarget != RESET_REQUEST && receivedTarget != GAME_WON && receivedTarget != RESET_CONFIRM)) 
      {
        receivedTarget = readTarget();
        delay(10);
      }
      
      if(receivedTarget == RESET_REQUEST)
      {
        sendTarget(RESET_CONFIRM);
        delay(100);
        gameResetPending = true; // set flag to reset on next loop iteration
        return; // reset will happen next loop
      }
      
      // check if opponent won the game
      if(receivedTarget == GAME_WON)
      {
        // set flag to reset on next loop iteration
        gameResetPending = true;
        return; // reset will happen next loop
      }
      
      if(receivedTarget == myShipPos)
      {
        sendTarget(1); // send hit confirm
        HitPattern();
        
        // wait to see if opponent sends GAME_WON signal
        delay(300);
        if(Serial.available())
        {
          int followUp = readTarget();
          if(followUp == GAME_WON)
          {
            // set flag to reset on next loop iteration
            gameResetPending = true;
            return; // don't process normal hit logic
          }
        }
        
        // normal hit processing - opponent didn't win
        for(int i = 0; i < 9; i++)
        {
          selectedList[i] = 0;
        }
        
        selectionPhase = true;
        shootingPhase = false;
        isInitialSelection = true; // both players need to select new ships
        myTurn = false; // opponent goes first next round
        
        delay(2000);
        allLEDOFF();
        
        // reset cursor to top-left
        GridLocation[0] = 0;
        GridLocation[1] = 0;
        lastOnLED = LEDGRID[0][0];
      }
      else
      {
        // send back miss signal
        sendTarget(0);
        XPattern();
        myTurn = true;
        delay(1000);
        allLEDOFF();
      }
    }
  }

  if (validMovementDetected) 
  {
    joystickPos = newPosition;
    handleMovement(joystickPos);
  }
  else if (newPosition == CENTER && joystickPos != CENTER) 
  {
    joystickPos = CENTER;
  }

  currentTime = millis();
  if (currentTime - lastBlinkTime >= BLINK_INTERVAL) 
  {
    blinkState = !blinkState;
    lastBlinkTime = currentTime;
  }
  
  // 7 segment display control
  if (!displayPatternActive) // only run display control when not showing patterns
  {
    if ((selectionPhase && isInitialSelection) || (shootingPhase && myTurn)) 
    {
      // show S for selection and shooting
      displayS();
    }
    else 
    {
      // show score at all other times
      displayScore(myScore);
    }
  }
  
  updateLEDs();
  delay(50);
}


// check if joystick is centered
bool isJoystickCentered() 
{
  int xValue = analogRead(xpotPin);
  int yValue = analogRead(ypotPin);
  
  int xDistance = abs(xValue - ORIGIN);
  int yDistance = abs(yValue - ORIGIN);
  
  return (xDistance < THRESHOLD && yDistance < THRESHOLD);
}

// determine joystick direction
int getJoystickDirection() 
{

  // check if joystick is centered
  bool isCentered = isJoystickCentered();
  validMovementDetected = false;
  
  // if centered return CENTER
  if (isCentered) 
  {
    wasLastMoveCentered = true;
    return CENTER;
  }
  
  if (wasLastMoveCentered) 
  {
    // if not centered and last move centered, determine joystick direction
    wasLastMoveCentered = false;
    validMovementDetected = true;
    
    int xRead = analogRead(xpotPin);
    int yRead = analogRead(ypotPin);
    int xDistance = xRead - ORIGIN;
    int yDistance = yRead - ORIGIN;
    
    if (abs(xDistance) > abs(yDistance)) 
    {
      return (xDistance > 0) ? RIGHT : LEFT;
    } 
    else 
    {
      return (yDistance > 0) ? DOWN : UP;
    }
  }

  /* if not centered and last move not centered, 
   * return last position with validMovementDetected set false
   */ 
  return joystickPos;
}

// move the selected LED on the grid based on joystick input
void handleMovement(int direction) 
{
  int currentListIndex = lastOnLED - 2;
  if (selectedList[currentListIndex] == 0) 
  {
    LEDOFF(lastOnLED);
  }

  switch(direction) 
  {
    case LEFT:
      // check if on left edge and wrap to right edge
      if(GridLocation[1] == 0)
      {
        GridLocation[1] = 2;
      }
      else
      {
        GridLocation[1]--;
      }

      // set cursor to new grid location
      lastOnLED = LEDGRID[GridLocation[0]][GridLocation[1]];
      break;

    case RIGHT:
      // check if on right edge and wrap to left edge
      if(GridLocation[1] == 2)
      {
        GridLocation[1] = 0;
      }
      else
      {
        GridLocation[1]++;
      }

      lastOnLED = LEDGRID[GridLocation[0]][GridLocation[1]];
      break;

    case UP:
      // check if on upper edge and wrap to bottom edge
      if(GridLocation[0] == 0)
      {
        GridLocation[0] = 2;
      }
      else
      {
        GridLocation[0]--;
      }

      lastOnLED = LEDGRID[GridLocation[0]][GridLocation[1]];
      break;

    case DOWN:
      // check if on bottom edge and wrap to upper edge
      if(GridLocation[0] == 2)
      {
        GridLocation[0] = 0;
      }
      else
      {
        GridLocation[0]++;
      }

      lastOnLED = LEDGRID[GridLocation[0]][GridLocation[1]];
      break;
  }
}

// update status of all LEDs
void updateLEDs() 
{
  // temporarily turn off all LEDs
  for (int i = 0; i < 3; i++) 
  {
    for (int j = 0; j < 3; j++) 
    {
      LEDOFF(LEDGRID[i][j]);
    }
  }
  
  // Only show LEDs when it's my turn or during selection phase
  if ((shootingPhase && myTurn) || (selectionPhase && isInitialSelection)) 
  {
    // turn back on LEDs that are in the selected list (only during shooting phase)
    if (shootingPhase) 
    {
      for (int i = 0; i < 9; i++) 
      {
        if (selectedList[i] == 1) 
        {
          LEDON(i + 2);
        }
      }
    }
    
    // turn on blinking for cursor
    if (blinkState) 
    {
      LEDON(lastOnLED);
    }
    else
    {
      LEDOFF(lastOnLED);
    }
  }
}

// turn on specified LED
void LEDON (int LEDNumber)
{
  digitalWrite(LEDNumber, HIGH);
}

// turn of specified LED
void LEDOFF (int LEDNumber)
{
  digitalWrite(LEDNumber, LOW);
}

// turn off all LEDs
void allLEDOFF()
{
  for (int i = 0; i < 3; i++) 
  {
    for (int j = 0; j < 3; j++) 
    {
      LEDOFF(LEDGRID[i][j]);
    }
  }
}

// display an X on a miss
void XPattern()
{
  displayPatternActive = true; // 7-segment display control flag
  for (int i = 0; i < 3; i++)
  {
    for (int i = 0; i < 3; i++) 
    {
      for (int j = 0; j < 3; j++) 
      {
        LEDOFF(LEDGRID[i][j]);
      }
    }
    
    delay(400);

    LEDON(2);
    LEDON(8);
    LEDON(6);
    LEDON(4);
    LEDON(10);

    delay(400);
  }
  displayPatternActive = false; // reset flag to allow 7-segment display control again
}

// display incrementing row pattern on each shot
void ShotPattern()
{
  displayPatternActive = true;
  for(int i = 0; i < 3; i++)
  {
    for (int i = 0; i < 3; i++) 
    {
      for (int j = 0; j < 3; j++) 
      {
        LEDOFF(LEDGRID[i][j]);
      }
    }
    
    delay(400);

    for (int i = 0; i < 3; i++)
    {
      LEDON(LEDGRID[2-i][0]);
      LEDON(LEDGRID[2-i][1]);
      LEDON(LEDGRID[2-i][2]);

      delay(300);

      if (i < 2)
      {
        LEDON(LEDGRID[1-i][0]);
        LEDON(LEDGRID[1-i][1]);
        LEDON(LEDGRID[1-i][2]);

        delay(100);
      }
      
      LEDOFF(LEDGRID[2-i][0]);
      LEDOFF(LEDGRID[2-i][1]);
      LEDOFF(LEDGRID[2-i][2]);
    }
  }
  displayPatternActive = false;
}

// display pixelated explosion on hit
void HitPattern()
{
  displayPatternActive = true;
  for (int i = 0; i < 3; i++) 
  {
    for (int j = 0; j < 3; j++) 
    {
      LEDOFF(LEDGRID[i][j]);
    }
  }
    
  delay(400);

  LEDON(6);

  delay(300);

  LEDON(5);
  LEDON(3);
  LEDON(9);
  LEDON(7);

  delay(300);

  LEDOFF(6);
  LEDON(2);
  LEDON(8);
  LEDON(4);
  LEDON(10);

  delay(300);
  displayPatternActive = false;
}

// functions for displaying numbers to 7-segment display
void display0()
{
  digitalWrite(pinB, LOW);
  digitalWrite(pinA, LOW);
}

void display1()
{
  digitalWrite(pinB, HIGH);
  digitalWrite(pinA, LOW);
}

void display2()
{
  digitalWrite(pinB, LOW);
  digitalWrite(pinA, HIGH);
}

void displayS()
{
  // display S on 7-segment indicating selection/shooting
  digitalWrite(pinB, HIGH);
  digitalWrite(pinA, HIGH);
}

void displayScore(int score)
{
  // display value on 7-segment based on score
  switch(score)
  {
    case 0:
      display0();
      break;

    case 1:
      display1();
      break;

    case 2:
      display2();
      break;
  }
}

int readTarget()
{
  int recTarget;
  
  // wait until there is data
  while(!Serial.available())
  {
  }
  
  recTarget = Serial.parseInt();

  // clear serial buffer
  while (Serial.available() > 0) 
  {
    char c = Serial.read();
    if (c == '\n') break;  // stop clearing when newline found
  }

  return recTarget;
}

void sendTarget(int target)
{
  // send integer over UART
  Serial.println(target);
}

void performReset()
{
  // reset game state (score and leds)
  myScore = 0;
  for(int i = 0; i < 9; i++)
  {
    selectedList[i] = 0;
  }

  selectionPhase = true;
  shootingPhase = false;
  isInitialSelection = true;
  myTurn = isPlayerOne;  // initial Player 1 always goes first after reset
  gameResetPending = false;
  
  // flash all LEDs to indicate reset
  for(int flash = 0; flash < 5; flash++)
  {
    for(int i = 2; i <= 10; i++)
    {
      LEDON(i);
    }
    delay(200);
    allLEDOFF();
    delay(200);
  }
  
  // reset cursor to top-left
  GridLocation[0] = 0;
  GridLocation[1] = 0;
  lastOnLED = LEDGRID[0][0];
  displayS();
}
