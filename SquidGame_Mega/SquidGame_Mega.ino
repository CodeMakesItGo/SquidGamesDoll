
/// CodeMakesItGo Dec 2021
/// Program for the Arduino Mega 2560

/*-----( Includes )-----*/
#include <Servo.h> //by Arduino, (timer 5) *comment out servoTimers.h line 36-38 to only use timer 5
#include <DFPlayerMini_Fast.h> //by PowerBroker2 Version 1.2.4
#include <FireTimer.h> //by PowerBroker2 Version 1.0.5
#include <IRremote.h> //By Elegoo, included in this Github repo, (timer 2)
#include <LiquidCrystal_I2C.h> //by Marco Schwarts version 1.1.2
#include <SR04.h> //By Elegoo, included in this Github repo


/*-----( Analog Pins )-----*/
#define SONAR_TRIG_PIN A1
#define SONAR_ECHO_PIN A2
#define MOTION_IN A3
#define BUTTON1_IN A4
#define BUTTON2_IN A5
#define BUTTON3_IN A6

/*-----( Digital Pins )-----*/
#define LED_BLUE 13
#define LED_GREEN 12
#define LED_RED 11
#define SEGMENT_DATA 10 // DS
#define SEGMENT_CLOCK 9 // SHCP
#define SEGMENT_LATCH 8 // STCP
#define SEGMENT_1_OUT 7
#define SEGMENT_2_OUT 6
#define SEGMENT_3_OUT 5
#define IR_DIGITAL_IN 4 // IR Remote
#define SERVO_OUT 3
#define DFPLAYER_BUSY_IN 2

/*-----( Configuration )-----*/
#define TIMER_FREQUENCY 2000
#define TIMER_MATCH (int)(((16E+6) / (TIMER_FREQUENCY * 64.0)) - 1)
#define TIMER_2MS ((TIMER_FREQUENCY / 1000) * 2)
#define VOLUME 30                    // 0-30
#define BETTER_HURRY_S 5             // play clip at 5 seconds left
#define WIN_PROXIMITY_CM 50          // cm distance for winner
#define CLOSE_PROXIMITY_CM 100       // cm distance for close to winning
#define GREEN_LIGHT_MS 3000          // 3 seconds on for green light
#define RED_LIGHT_MS 5000            // 5 seconds on for green light
#define WAIT_FOR_STOP_MOTION_MS 5000 // 5 seconds to wait for motion detection to stop
#define USING_MOTION_SENSOR false    // set to false if not using motion sensor
#define USING_DISTANCE_SENSOR false  // set to false if not using distance sensor
#define BUTTON_DEBOUNCE 2            // 2*50ms=100ms, amount of time a button must be held down before triggering a press

/*-----( Global Variables )-----*/
static unsigned int timer_1000ms = 0;
static unsigned int timer_2ms = 0;
static unsigned char digit = 0;      // digit for 4 segment display
#if USING_MOTION_SENSOR
static int countDown = 60;           // Start 1 minute countdown on startup
#else
static int countDown = 0;           // Start 0 minute countdown on startup
#endif
static const int sonarVariance = 10; // detect movement if greater than this
static bool gameInPlay = false;
static bool faceTree = false;
static bool remotePlay = false;

//                                 0 ,   1,   2,   3,   4,   5,   6,   7,   8,   9,   A,   B,   C,   D,   E,   F, NULL
const unsigned char numbers[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x00};

const char *MenuItems[] = {"Language", "Play Time", "Play Type"};
typedef enum
{
  LANGUAGE,
  PLAYTIME,
  PLAYTYPE,
  MENUITEM_COUNT
} MenuItemTypes;

const char *Languages[] = {"English", "Korean"};
typedef enum
{
  ENGLISH,
  KOREAN,
  LANUAGE_COUNT
} LanguageTypes;
static int language = 0;

const char *PlayTime[] = {"300", "240", "180", "120", "60", "30", "15"};
typedef enum
{
  PT300,
  PT240,
  PT180,
  PT120,
  PT60,
  PT30,
  PT15,
  PLAYTIME_COUNT
} PlayTimeTypes;
const int playTimes[] = {300, 240, 180, 120, 60, 30, 15};
static int playTime = 0;

const char *PlayType[] = {"Auto", "Remote"};
typedef enum
{
  AUTO,
  REMOTE,
  PLAYTYPE_COUNT
} PlayTypeTypes;
static int playType = 0;

typedef enum
{
  BLACK,
  RED,
  GREEN,
  BLUE,
  WHITE,
  YELLOW,
  PURPLE
} EyeColors;
static EyeColors eyeColor = BLACK;

typedef enum
{
  WARMUP,
  WAIT,
  READY,
  GREENLIGHT,
  REDLIGHT,
  WIN,
  LOSE
} GameStates;
static GameStates gameState = WARMUP;

/*-----( Class Objects )-----*/
FireTimer task_50ms;
FireTimer task_250ms;
DFPlayerMini_Fast dfPlayer;
SR04 sonar = SR04(SONAR_ECHO_PIN, SONAR_TRIG_PIN);
IRrecv irRecv(IR_DIGITAL_IN);
decode_results irResults;
LiquidCrystal_I2C lcdDisplay(0x27, 16, 2); // 16x2 LCD display
Servo servo;

/*-----( Functions )-----*/
void translateIR() // takes action based on IR code received
{
  switch (irResults.value)
  {
  case 0xFFA25D:
    Serial.println("POWER");
    if (gameState == WAIT)
    {
      gameInPlay = true;
    }
    break;
  case 0xFFE21D:
    Serial.println("FUNC/STOP");
    break;
  case 0xFF629D:
    Serial.println("VOL+");
    break;
  case 0xFF22DD:
    Serial.println("FAST BACK");
    break;
  case 0xFF02FD:
    Serial.println("PAUSE");
    remotePlay = !remotePlay;
    break;
  case 0xFFC23D:
    Serial.println("FAST FORWARD");
    break;
  case 0xFFE01F:
    Serial.println("DOWN");
    break;
  case 0xFFA857:
    Serial.println("VOL-");
    break;
  case 0xFF906F:
    Serial.println("UP");
    break;
  case 0xFF9867:
    Serial.println("EQ");
    break;
  case 0xFFB04F:
    Serial.println("ST/REPT");
    break;
  case 0xFF6897:
    Serial.println("0");
    break;
  case 0xFF30CF:
    Serial.println("1");
    break;
  case 0xFF18E7:
    Serial.println("2");
    break;
  case 0xFF7A85:
    Serial.println("3");
    break;
  case 0xFF10EF:
    Serial.println("4");
    break;
  case 0xFF38C7:
    Serial.println("5");
    break;
  case 0xFF5AA5:
    Serial.println("6");
    break;
  case 0xFF42BD:
    Serial.println("7");
    break;
  case 0xFF4AB5:
    Serial.println("8");
    break;
  case 0xFF52AD:
    Serial.println("9");
    break;
  case 0xFFFFFFFF:
    Serial.println(" REPEAT");
    break;

  default:
    Serial.println(" other button   ");
  }
}

bool isPlayingSound()
{
  return (digitalRead(DFPLAYER_BUSY_IN) == LOW);
}

void updateTimeDisplay(unsigned char digit, unsigned char num)
{
  digitalWrite(SEGMENT_LATCH, LOW);
  shiftOut(SEGMENT_DATA, SEGMENT_CLOCK, MSBFIRST, numbers[num]);

  // Active LOW
  digitalWrite(SEGMENT_1_OUT, digit == 1 ? LOW : HIGH);
  digitalWrite(SEGMENT_2_OUT, digit == 2 ? LOW : HIGH);
  digitalWrite(SEGMENT_3_OUT, digit == 3 ? LOW : HIGH);

  digitalWrite(SEGMENT_LATCH, HIGH);
}

void updateServoPosition()
{
  if (!faceTree) // 180 degrees
  {
    servo.write(180);
  }
  else // 0 degrees
  {
    servo.write(0);
  }
}

void updateMenuDisplay(const int button)
{
  static int menuItem = 0;
  static int menuOption = 0;
  switch (button)
  {
  case 1:
    menuItem = (menuItem + 1) % MENUITEM_COUNT;
    if (menuItem == LANGUAGE)
    {
      menuOption = language;
    }
    else if (menuItem == PLAYTIME)
    {
      menuOption = playTime;
    }
    else if (menuItem == PLAYTYPE)
    {
      menuOption = playType;
    }
    else
    {
      menuOption = 0;
    }
    break;
  case 2:
    if (menuItem == LANGUAGE)
    {
      menuOption = (menuOption + 1) % LANUAGE_COUNT;
      language = menuOption;
    }
    else if (menuItem == PLAYTIME)
    {
      menuOption = (menuOption + 1) % PLAYTIME_COUNT;
      playTime = menuOption;
    }
    else if (menuItem == PLAYTYPE)
    {
      menuOption = (menuOption + 1) % PLAYTYPE_COUNT;
      playType = menuOption;
    }
    else
    {
      menuOption = 0;
    }
    break;
  case 3:
    if (gameState == WAIT)
    {
      gameInPlay = true;
    }
    if (gameState == GREENLIGHT || gameState == REDLIGHT)
    {
      gameInPlay = false;
    }
  default:
    break;
  }

  if (menuOption != -1)
  {
    lcdDisplay.clear();

    lcdDisplay.setCursor(0, 0);
    lcdDisplay.print(MenuItems[menuItem]);
    lcdDisplay.setCursor(0, 1);

    if (menuItem == LANGUAGE)
    {
      lcdDisplay.print(Languages[menuOption]);
    }
    else if (menuItem == PLAYTIME)
    {
      lcdDisplay.print(PlayTime[menuOption]);
    }
    else if (menuItem == PLAYTYPE)
    {
      lcdDisplay.print(PlayType[menuOption]);
    }
    else
    {
      lcdDisplay.print("unknown option");
    }
  }
  else
  {
    menuItem = 0;
    menuOption = 0;
  }
}

void handleButtons()
{
  static long buttonPressCount[3] = {0};
  int buttonPressed = 0;

  if(digitalRead(BUTTON1_IN) == LOW)
  {
    buttonPressCount[0]++;
  }
  else
  {
    if(buttonPressCount[0] >= BUTTON_DEBOUNCE)
    {
      Serial.println("button 1");
      buttonPressed = 1;
    }
    buttonPressCount[0] = 0;
  }



  if(digitalRead(BUTTON2_IN) == LOW)
  {
    buttonPressCount[1]++;
  }
  else
  {
    if(buttonPressCount[1] >= BUTTON_DEBOUNCE)
    {
      Serial.println("button 2");
      buttonPressed = 2;
    }
    buttonPressCount[1] = 0;
  }



  if(digitalRead(BUTTON3_IN) == LOW)
  {
    buttonPressCount[2]++;
  }
  else
  {
    if(buttonPressCount[2] >= BUTTON_DEBOUNCE)
    {
      Serial.println("button 3");
      buttonPressed = 3;
    }
    buttonPressCount[2] = 0;
  }

  if (buttonPressed != 0)
  {
      updateMenuDisplay(buttonPressed);
  }
}

static int lastSonarValue = 0;
void handleSonar()
{
  int value = sonar.Distance();

  if (value > lastSonarValue + sonarVariance ||
      value < lastSonarValue - sonarVariance)
  {
    Serial.println(value);
    lastSonarValue = value;
  }
}

static int lastMotion = 0;
void handleMotion()
{
  int value = digitalRead(MOTION_IN);

  if (value != lastMotion)
  {
    lastMotion = value;
  }

  if (lastMotion)
    Serial.println("Motion Detected");
}

void handleLeds()
{
  digitalWrite(LED_RED, eyeColor == RED || eyeColor == WHITE || eyeColor == PURPLE || eyeColor == YELLOW ? HIGH : LOW);
  digitalWrite(LED_GREEN, eyeColor == GREEN || eyeColor == WHITE || eyeColor == YELLOW ? HIGH : LOW);
  digitalWrite(LED_BLUE, eyeColor == BLUE || eyeColor == WHITE || eyeColor == PURPLE ? HIGH : LOW);
}

void handleRemote()
{
  // have we received an IR signal?
  if (irRecv.decode(&irResults))
  {
    translateIR();
    irRecv.resume(); // receive the next value
  }
}

// Timer 1 ISR
ISR(TIMER1_COMPA_vect)
{
  // Allow this ISR to be interrupted
  sei();

  if (timer_1000ms++ == TIMER_FREQUENCY)
  {
    timer_1000ms = 0;
    countDown--;
    if (countDown < 0)
    {
      countDown = 0;
    }
  }

  if (timer_2ms++ == TIMER_2MS)
  {
    timer_2ms = 0;
    if (digit == 0)
      updateTimeDisplay(1, countDown % 10);
    if (digit == 1)
      updateTimeDisplay(2, (countDown / 10) % 10);
    if (digit == 2)
      updateTimeDisplay(3, (countDown / 100) % 10);
    if (digit == 3)
      updateTimeDisplay(4, 16);

    digit = ((digit + 1) % 4);
  }
}

void playGame()
{
  static int sequence = 0;
  static long internalTimer = millis();
  static bool closerClipPlayed = false;
  static bool hurryUpClipPlayed = false;
  static int captureDistance = 0;
  long currentTimer = internalTimer;

  if(isPlayingSound()) return;

  if (gameState == WARMUP)
  {
#if USING_MOTION_SENSOR
    // power up sound
    if (sequence == 0)
    {
      Serial.println("Warming Up");
      dfPlayer.playFolder(1, 1);
      faceTree = false;
      eyeColor = YELLOW;
      sequence++;
    }

    // laugh at 30
    else if (sequence == 1 && countDown <= 30)
    {
      Serial.println("Laughing");
      dfPlayer.playFolder(1, 2);
      faceTree = true;
      sequence++;
    }

    else if (sequence == 2 && countDown <= 10)
    {
      Serial.println("Almost ready");
      dfPlayer.playFolder(1, 3);
      sequence++;
    }

    else if (sequence == 3 && 
#else
    if(
#endif //USING_MOTION_SENSOR
    countDown == 0)
    {
      Serial.println("All ready, lets play");
      dfPlayer.playFolder(1, 4);
      faceTree = false;
      sequence = 0;
      gameState = WAIT;
      gameInPlay = false;
    }
  }

  else if (gameState == WAIT)
  {
    currentTimer = millis();

    if (gameInPlay)
    {
      gameState = READY;
      remotePlay = false;
      sequence = 0;
    }

    // Every 30 seconds
    else if (currentTimer - internalTimer > 30000 ||
             sequence == 0)
    {
      internalTimer = millis();

      if(playType == AUTO)
      {
        // press the go button when you are ready
        Serial.println("Press the go button when you are ready");
        dfPlayer.playFolder(1, 5);
      }
      else
      {
        Serial.println("Press the power button on the remote when you are ready");
        dfPlayer.playFolder(1, 6);
      }

      // eyes are blue
      eyeColor = BLUE;

      // facing players
      faceTree = false;

      gameInPlay = false;

      sequence++;
    }
  }

  else if (gameState == READY)
  {
    currentTimer = millis();

    if (sequence == 0)
    {
      // get in position, game will start in 10 seconds
      Serial.println("Get in position.");
      dfPlayer.playFolder(1, 7);
      countDown = 10;

      // eyes are green
      eyeColor = WHITE;

      // facing players
      faceTree = false;

      sequence++;

      internalTimer = millis();
    }
    else if (sequence == 1)
    {
      if (playType == REMOTE)
      {
        if (remotePlay)
          sequence++;
      }
      else
        sequence++;
    }
    else if (sequence == 2)
    {
      // at 0 seconds, here we go!
      if (countDown == 0)
      {
        countDown = playTimes[playTime];
        Serial.print("play time set to ");
        Serial.println(countDown);

        Serial.println("Here we go!");
        dfPlayer.playFolder(1, 8);
        gameState = GREENLIGHT;
        sequence = 0;
      }
    }
  }

  else if (gameState == GREENLIGHT)
  {
    currentTimer = millis();

    if (sequence == 0)
    {
      // eyes are green
      eyeColor = GREEN;

      // play green light
      Serial.println("Green Light!");
      dfPlayer.playFolder(1, 9);
      
      sequence++;
    }

    else if(sequence == 1)
    {
      // play motor sound
      dfPlayer.playFolder(1, 19);
      
      // facing tree
      faceTree = true;

      sequence++;

      internalTimer = millis();
    }

    else if (sequence == 2)
    {
      // wait 3 seconds or until remote
      // switch to red light
      if (playType == AUTO && currentTimer - internalTimer > GREEN_LIGHT_MS)
      {
        sequence = 0;
        gameState = REDLIGHT;
      }
      else if (playType == REMOTE && remotePlay == false)
      {
        sequence = 0;
        gameState = REDLIGHT;
      }
      else
      {
        // look for winner button or distance
        if (gameInPlay == false 
#if USING_DISTANCE_SENSOR        
        || lastSonarValue < WIN_PROXIMITY_CM
#endif
           )
        {
          sequence = 0;
          gameState = WIN;
        }

        else if (countDown <= 0)
        {
          Serial.println("Out of Time");
          dfPlayer.playFolder(1, 16);
          sequence = 0;
          gameState = LOSE;
        }
        
#if USING_DISTANCE_SENSOR        
        // at 2 meters play "your getting closer"
        else if (lastSonarValue < CLOSE_PROXIMITY_CM &&
            closerClipPlayed == false)
        {
          Serial.println("Getting closer!");
          dfPlayer.playFolder(1, 11);
          closerClipPlayed = true;
        }
#endif
        // if less than 5 seconds play better hurry
        else if (countDown <= BETTER_HURRY_S &&
            hurryUpClipPlayed == false)
        {
          Serial.println("Better Hurry");
          dfPlayer.playFolder(1, 12);
          hurryUpClipPlayed = true;
        }
      }
    }
  }

  else if (gameState == REDLIGHT)
  {
    currentTimer = millis();

    if (sequence == 0)
    {
      // eyes are red
      eyeColor = RED;

      Serial.println("Red Light!");

      if(language == ENGLISH)
      {
        // play red light English 
        dfPlayer.playFolder(1, 10);
      }
      else
      {
        // play red light Korean  
        dfPlayer.playFolder(1, 18);
      }

      sequence++;
    }

    else if(sequence == 1)
    {
      // play motor sound
      dfPlayer.playFolder(1, 19);

      // facing players
      faceTree = false;

      // Save current distance
      captureDistance = lastSonarValue;

      sequence++;

      internalTimer = millis();
    }

    else if (sequence == 2)
    {
      //wait for head motion to settle
      if (lastMotion == 0 || (currentTimer - internalTimer) > WAIT_FOR_STOP_MOTION_MS)
      {
        internalTimer = millis();
        sequence++;
        Serial.println("Done settling");
      }
      else
      {
        Serial.println("Waiting to settle");
      }
    }

    else if (sequence == 3)
    {
      // back to green after 5 seconds
      if (playType == AUTO && currentTimer - internalTimer > RED_LIGHT_MS)
      {
        sequence = 0;
        gameState = GREENLIGHT;
      }
      else if (playType == REMOTE && remotePlay == true)
      {
        sequence = 0;
        gameState = GREENLIGHT;
      }

      else
      {
        // can't push the button while red light
        // detect movement
        // detect distance change
        if (gameInPlay == false 
#if USING_MOTION_SENSOR        
            || lastMotion == 1 
#endif            
#if USING_DISTANCE_SENSOR
            || lastSonarValue < captureDistance
#endif
           )
        {
          Serial.println("Movement detected!");
          dfPlayer.playFolder(1, 15);
          sequence = 0;
          gameState = LOSE;
        }

        if (countDown == 0)
        {
          Serial.println("Out of time");
          dfPlayer.playFolder(1, 16);
          sequence = 0;
          gameState = LOSE;
        }
      }
    }
  }

  else if (gameState == WIN)
  {
    if (sequence == 0)
    {
      // play winner sound
      Serial.println("You Won!");
      dfPlayer.playFolder(1, 13);

      // eyes are white
      eyeColor = WHITE;

      // facing players
      faceTree = false;

      sequence++;
    }

    else if (sequence == 1)
    {
      // wanna play again?
      Serial.println("Play Again?");
      dfPlayer.playFolder(1, 17);

      gameInPlay = false;
      countDown = 0;

      // go to wait
      gameState = WAIT;
      sequence = 0;
    }
  }

  else if (gameState == LOSE)
  {
    if (sequence == 0)
    {
      // sorry better luck next time
      Serial.println("Sorry, you lost");
      dfPlayer.playFolder(1, 14);

      // eyes are purple
      eyeColor = PURPLE;

      // face players
      faceTree = false;

      sequence++;
    }

    else if (sequence == 1)
    {
      // wanna play again?
      Serial.println("Play Again?");
      dfPlayer.playFolder(1, 17);

      gameInPlay = false;
      countDown = 0;

      // go to wait
      gameState = WAIT;
      sequence = 0;
    }
  }
  else
  {
    //Shouldn't ever get here
    gameState = WARMUP;
  }
}

void loop() /*----( LOOP: RUNS CONSTANTLY )----*/
{
  if (task_50ms.fire())
  {
    handleRemote();
    handleButtons();
  }

  if (task_250ms.fire())
  {
    
#if USING_DISTANCE_SENSOR
    handleSonar();
#endif

#if USING_MOTION_SENSOR
    handleMotion();
#endif

    handleLeds();
    playGame();
    updateServoPosition();
  }
}

// Setup Timer 1 for 2000Hz
void setupTimer()
{
  cli();      //stop interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1 = 0;  //initialize counter value to 0
  // set compare match register
  OCR1A = TIMER_MATCH; // = (16*10^6) / (2000*64) - 1 (must be <65536), 2000Hz
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS11 and CS10 bits for 64 prescaler
  TCCR1B |= (1 << CS11) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); //allow interrupts
}

void setup()
{
  Serial.begin(9600);  // For the serial monitor
  Serial1.begin(9600); // For the DFPlayer
  
  pinMode(MOTION_IN, INPUT);
  pinMode(BUTTON1_IN, INPUT_PULLUP);
  pinMode(BUTTON2_IN, INPUT_PULLUP);
  pinMode(BUTTON3_IN, INPUT_PULLUP);
  pinMode(DFPLAYER_BUSY_IN, INPUT_PULLUP);

  pinMode(SERVO_OUT, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  pinMode(SEGMENT_LATCH, OUTPUT);
  pinMode(SEGMENT_CLOCK, OUTPUT);
  pinMode(SEGMENT_DATA, OUTPUT);

  pinMode(SEGMENT_1_OUT, OUTPUT);
  pinMode(SEGMENT_2_OUT, OUTPUT);
  pinMode(SEGMENT_3_OUT, OUTPUT);


  servo.attach(SERVO_OUT); // Setup Servo
  irRecv.enableIRIn();     // Start the receiver
  dfPlayer.begin(Serial1); // Use serial 1 stream for DfPlayer
  dfPlayer.volume(VOLUME); // Set the DfPlay volume
  lcdDisplay.init();       // initialize the lcd
  lcdDisplay.backlight();  // Turn on backlight
  setupTimer();            // Start the high resolution timer ISR

  // Display welcome message
  lcdDisplay.setCursor(0, 0);
  lcdDisplay.print("Welcome to the");
  lcdDisplay.setCursor(0, 1);
  lcdDisplay.print("Squid Games!");

  // short delay to display welcome screen 
  delay(1000);

  task_50ms.begin(50);   // Start the 50ms timer task
  task_250ms.begin(250); // Start the 250ms timer task
}
