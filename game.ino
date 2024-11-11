#include <TinyScreen.h>
#include <Wire.h>
#include <SPI.h>
#include "TinyArcade.h"
#include <SdFat.h> // Include the SdFat library for SD card functionality (Sketch -> Include Library -> Manage Libraries -> SdFat)

TinyScreen display = TinyScreen(TinyScreenPlus);

#define SCREENWIDTH 94
#define SCREENHEIGHT 64

SdFat SD;                      // SD card object
const uint8_t chipSelect = 10; // SD card chip select pin set to 10

const int speakerPin = A0; // Speaker pin

enum Screen
{
  TUTORIAL,
  GAMEPLAY,
  END,
};

enum Screen screen = TUTORIAL;

const int threshold = 15; // how many correct answers to progress to the next level
const int levels = 2;
const int startingLives = 10;
char *prompts[] = {"Same color", "Same shape"};

struct GameState
{
  int timerMax;         // how long, in milliseconds, do we have to answer
  int timerStartMillis; // when did we start the timer
  int timerX;           // the last drawn X position of the timer
  bool shapesHidden;    // we hide the shapes halfway through the timer; have we done it yet?
  bool currSameShape;   // are the currently generated shapes the same shape?
  bool currSameColor;
  int lives;
  int correct;
  int level;
};

struct LevelStats
{
  int correct;
  int incorrect;
};

struct LevelStats stats[levels];

int const maxHistory = 5; // the max # of previous session stats to load
LevelStats history[maxHistory][levels];
int historySize = 0; // how many previous sessions were actually loaded

int tutorialStep = 0;

struct GameState game = {.timerMax = 1500};

struct DebounceState
{
  bool state;
  bool lastState;
  unsigned long lastTime; // the last time we got a reading that differs from the one that came before it
};

struct DebounceState button1Debounce;
struct DebounceState button2Debounce;
struct DebounceState leftDebounce;
struct DebounceState rightDebounce;

unsigned long debounceDelay = 30;

int colors[] = {TS_8b_Blue, TS_8b_Red, TS_8b_Yellow};

// Function prototypes for functions defined in shapes.ino
void drawTriangle(int x0, int y0, int height, int color, bool fill);
void drawCircle(int x0, int y0, int radius, int color, bool fill);

void setup()
{
  // Initialize SerialUSB for debugging
  USBDevice.init();
  USBDevice.attach();
  SerialUSB.begin(9600);

  delay(1000);

  // Initialize the SD card
  if (!SD.begin(chipSelect, SD_SCK_MHZ(25)))
  {
    SerialUSB.println("SD card initialization failed!");
    // Handle error accordingly (e.g., disable SD card functionality)
  }
  else
  {
    SerialUSB.println("SD card initialized.");
  }

  randomSeed(analogRead(2));
  arcadeInit();
  display.begin();
  display.setBrightness(10); // 0-15
  display.setFont(thinPixel7_10ptFontInfo);

  pinMode(speakerPin, OUTPUT); // Initialize speaker pin

  displayTutorialStep(); // Display the first tutorial step
}

void loop()
{
  checkInput();
  if (screen == GAMEPLAY)
  {
    updateTimer();
  }
}

void playBeep()
{
  tone(speakerPin, 500, 200); // Play a 1000Hz tone for 100ms
}

void playCorrectSound()
{
  tone(speakerPin, 1500, 100); // Higher pitch for correct answer
}

void playIncorrectSound()
{
  tone(speakerPin, 500, 100); // Lower pitch for incorrect answer
}

bool debounce(bool reading, DebounceState *debounceState)
{
  if (reading != debounceState->lastState)
  {
    debounceState->lastTime = millis();
  }

  bool valueChanged = false;
  if ((millis() - debounceState->lastTime) > debounceDelay)
  {
    if (reading != debounceState->state)
    {
      debounceState->state = reading;
      valueChanged = true;
    }
  }

  debounceState->lastState = reading;
  return valueChanged;
}

void checkInput()
{
  // These are only true once per press (they're "click handlers" that trigger on press, not release)
  bool button1Pressed = debounce(checkButton(TAButton1), &button1Debounce) && button1Debounce.state;
  bool button2Pressed = debounce(checkButton(TAButton2), &button2Debounce) && button2Debounce.state;
  bool leftStickPressed = debounce(checkJoystick(TAJoystickLeft), &leftDebounce) && leftDebounce.state;
  bool rightStickPressed = debounce(checkJoystick(TAJoystickRight), &rightDebounce) && rightDebounce.state;

  bool leftPressed = button1Pressed || leftStickPressed;
  bool rightPressed = button2Pressed || rightStickPressed;

  if (screen == TUTORIAL)
  {
    if ((tutorialStep == 3 || tutorialStep == 5))
    {
      if (leftPressed)
      {
        nextTutorialStep();
      }
    }
    else if (rightPressed)
    {
      nextTutorialStep();
    }
  }

  if (screen == END && (leftPressed || rightPressed))
  {
    playBeep();
    game.level = 0;
    nextLevel();
  }

  if (screen == GAMEPLAY)
  {
    if (rightPressed)
    {
      playBeep();
      if ((game.level == 1 && game.currSameColor) || (game.level == 2 && game.currSameShape))
      {
        next(true);
      }
      else
      {
        next(false);
      }
    }
    else if (leftPressed)
    {
      playBeep();
      if ((game.level == 1 && !game.currSameColor) || (game.level == 2 && !game.currSameShape))
      {
        next(true);
      }
      else
      {
        next(false);
      }
    }
  }
}

void updateTimer()
{
  int progress = millis() - game.timerStartMillis;
  int newTimerX = round(((double)progress / (double)game.timerMax) * SCREENWIDTH);

  if (!game.shapesHidden && progress >= game.timerMax / 2)
  {
    game.shapesHidden = true;
    display.clearWindow(0, 20, SCREENWIDTH, SCREENHEIGHT - 25);
  }
  if (progress >= game.timerMax)
  {
    next(false);
    return;
  }
  if (newTimerX != game.timerX)
  {
    display.drawLine(newTimerX, SCREENHEIGHT - 4, newTimerX, SCREENHEIGHT, TS_8b_Red);
    game.timerX = newTimerX;
  }
}

void next(bool wasCorrect)
{
  if (wasCorrect)
  {
    playCorrectSound(); // Play correct sound
    game.correct++;
  }
  else
  {
    playIncorrectSound(); // Play incorrect sound
    game.lives--;
    if (game.lives == 0)
    {
      populateStats();
      gameOver();
      return;
    }
  }
  if (game.correct == threshold)
  {
    nextLevel();
    return;
  }
  game.timerStartMillis = millis();
  display.clearWindow(0, 20, SCREENWIDTH + 1, SCREENHEIGHT);
  game.shapesHidden = false;
  drawHUD();
  drawShapes();
}

void nextTutorialStep()
{
  tutorialStep++;
  displayTutorialStep();
  playBeep();
  if (tutorialStep > 6)
  {
    nextLevel();
  }
}

void displayTutorialStep()
{
  display.clearWindow(0, 0, SCREENWIDTH + 1, SCREENHEIGHT);
  display.fontColor(TS_8b_White, TS_8b_Black);

  if (tutorialStep == 0)
  {
    char *tutorial = "Welcome!";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorial) / 2, 15);
    display.print(tutorial);
    tutorial = "[Press right]";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorial) / 2, 55);
    display.print(tutorial);
  }
  else if (tutorialStep == 1)
  {
    char *tutorialStep1 = "Buttons:";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep1) / 2, 2);
    display.print(tutorialStep1);
    tutorialStep1 = "Right = Match";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep1) / 2, 22);
    display.print(tutorialStep1);
    tutorialStep1 = "Left = Mismatch";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep1) / 2, 32);
    display.print(tutorialStep1);
    tutorialStep1 = "[Press right]";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep1) / 2, 55);
    display.print(tutorialStep1);
  }
  else if (tutorialStep == 2)
  {
    char *tutorialStep2 = "Level 1:";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep2) / 2, 2);
    display.print(tutorialStep2);
    tutorialStep2 = "Matching Colors";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep2) / 2, 10);
    display.print(tutorialStep2);
    tutorialStep2 = "[Right = Match]";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep2) / 2, 55);
    display.print(tutorialStep2);

    drawShape(true, TS_8b_Yellow, 1);  // Left: Yellow square
    drawShape(false, TS_8b_Yellow, 2); // Right: Yellow circle
  }
  else if (tutorialStep == 3)
  {
    char *tutorialStep3 = "Level 1:";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep3) / 2, 2);
    display.print(tutorialStep3);
    tutorialStep3 = "Matching Colors";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep3) / 2, 10);
    display.print(tutorialStep3);
    tutorialStep3 = "[Left = Mismatch]";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep3) / 2, 55);
    display.print(tutorialStep3);

    drawShape(true, TS_8b_Blue, 1);    // Left: Blue square
    drawShape(false, TS_8b_Yellow, 2); // Right: Yellow circle
  }
  else if (tutorialStep == 4)
  {
    char *tutorialStep4 = "Level 2:";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep4) / 2, 2);
    display.print(tutorialStep4);
    tutorialStep4 = "Matching Shapes";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep4) / 2, 10);
    display.print(tutorialStep4);
    tutorialStep4 = "[Right = Match]";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep4) / 2, 55);
    display.print(tutorialStep4);

    drawShape(true, TS_8b_Blue, 0); // Left: Blue triangle
    drawShape(false, TS_8b_Red, 0); // Right: Red triangle
  }
  else if (tutorialStep == 5)
  {
    char *tutorialStep5 = "Level 2:";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep5) / 2, 2);
    display.print(tutorialStep5);
    tutorialStep5 = "Matching Shapes";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep5) / 2, 10);
    display.print(tutorialStep5);
    tutorialStep5 = "[Left = Mismatch]";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep5) / 2, 55);
    display.print(tutorialStep5);

    drawShape(true, TS_8b_Blue, 1);  // Left: Blue square
    drawShape(false, TS_8b_Blue, 0); // Right: Blue triangle
  }
  else if (tutorialStep == 6)
  {
    char *tutorialStep6 = "Start Game!";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep6) / 2, 15);
    display.print(tutorialStep6);
    tutorialStep6 = "[Press right]";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(tutorialStep6) / 2, 55);
    display.print(tutorialStep6);
  }
}

void populateStats()
{
  stats[game.level - 1] = {.correct = game.correct, .incorrect = startingLives - game.lives};
}

void nextLevel()
{
  if (game.level >= 1)
  {
    populateStats();
  }

  if (game.level == levels)
  {
    gameOver();
    return;
  }

  screen = GAMEPLAY;
  game.level++;
  game.lives = startingLives;
  game.correct = 0; // Resetting correct answers for the new level
  game.currSameShape = false;
  game.currSameColor = false;
  game.timerStartMillis = millis();
  display.clearWindow(0, 0, SCREENWIDTH + 1, SCREENHEIGHT);
  game.shapesHidden = false;
  drawHUD();
  drawShapes();
}

void printStatLine(struct LevelStats stats[], int xStart)
{
  const int levelHeight = 12;
  const int gap = 2;
  const int width = 6;

  for (int i = 0; i < levels; i++)
  {
    if (stats[i].correct > 0 || stats[i].incorrect > 0)
    { // if we got to this level
      int bottom = SCREENHEIGHT - (levelHeight + gap) * i - 1;
      int total = stats[i].correct + stats[i].incorrect;
      int totalHeight = stats[i].correct >= threshold ? levelHeight : levelHeight / 2;
      int correctPixels = rintf((float)stats[i].correct * totalHeight / total);
      int incorrectPixels = rintf((float)stats[i].incorrect * totalHeight / total);
      display.drawRect(xStart, bottom - 10, width, correctPixels, TSRectangleFilled, TS_8b_Green);
      display.drawRect(xStart, bottom - 10 + correctPixels, width, incorrectPixels, TSRectangleFilled, TS_8b_Red);
    }
  }
}

void gameOver()
{
  screen = END;
  display.clearWindow(0, 0, SCREENWIDTH + 1, SCREENHEIGHT);
  display.fontColor(TS_8b_White, TS_8b_Black);

  printStatLine(stats, 3);
  readStatsFromSD();
  for (int i = 0; i < historySize; i++) {
    printStatLine(history[i], i * 8);
  }
  writeStatsToSD();
  // Write the total score to the SD card
  //  writeScoreToSD(game.totalCorrect);
  //
  //  // Update high score if necessary
  //  if (game.totalCorrect > game.highScore)
  //  {
  //    game.highScore = game.totalCorrect;
  //    SerialUSB.println("New high score!");
  //  }

  if (game.lives == 0)
  {
    char *txt = "Game Over";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(txt) / 2, 10);
    display.print(txt);
    // txt = "You got to level ";
    // int width = display.getPrintWidth(txt) + display.getPrintWidth("2");
    // display.setCursor(SCREENWIDTH / 2 - width / 2, 25);
    // display.print(txt);
    // display.setCursor(SCREENWIDTH / 2 + display.getPrintWidth(txt) / 2, 25);
    // display.print(game.level);
  }
  else
  {
    char *txt = "You win";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(txt) / 2, 10);
    display.print(txt);
  }

  memset(stats, 0, sizeof stats);

  // Display the high score
  //  displayHighScore();
  //  game.totalCorrect = 0;
}

void drawHUD()
{
  display.clearWindow(0, 0, SCREENWIDTH, 20);

  display.fontColor(TS_8b_Green, TS_8b_Black);
  display.setCursor(2, 6);
  display.print(game.correct);

  display.fontColor(TS_8b_White, TS_8b_Black);
  display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(prompts[game.level - 1]) / 2, 6);
  display.print(prompts[game.level - 1]);

  display.fontColor(TS_8b_Red, TS_8b_Black);
  display.setCursor(SCREENWIDTH - 13, 6);
  display.print(game.lives);
}

void drawShapes()
{
  if (game.level == 1)
  {
    // We want to get two different colors half the time (shape is totally random)
    int leftColorIndex = random(3);
    int rightColorIndex;
    if (random(2) == 0)
    {
      rightColorIndex = leftColorIndex;
      game.currSameColor = true;
    }
    else
    {
      // choose the other color and make sure it's different
      rightColorIndex = (leftColorIndex + random(1, 3)) % 3;
      game.currSameColor = false;
    }

    drawShape(true, colors[leftColorIndex], random(3));
    drawShape(false, colors[rightColorIndex], random(3));
  }
  else if (game.level == 2)
  {
    // We want to get two different shapes half the time (color is totally random)
    int leftShapeType = random(3);
    int rightShapeType;
    if (random(2) == 0)
    {
      rightShapeType = leftShapeType;
      game.currSameShape = true;
    }
    else
    {
      rightShapeType = (leftShapeType + random(1, 3)) % 3;
      game.currSameShape = false;
    }

    drawShape(true, colors[random(3)], leftShapeType);
    drawShape(false, colors[random(3)], rightShapeType);
  }
}

void drawShape(bool left, int color, int shapeType)
{
  switch (shapeType)
  {
  case 0:
    randomTriangle(left, color);
    break;
  case 1:
    randomSquare(left, color);
    break;
  case 2:
    randomCircle(left, color);
    break;
  }
}

void randomTriangle(bool left, int color)
{
  int x = SCREENWIDTH / 2 + (left ? -20 : 20);
  int y = SCREENHEIGHT * 2 / 3;
  if (random(2) == 0)
  {
    drawTriangle(x, y, 16, color, false);
    drawTriangle(x, y, 28, color, false);
    return;
  }
  drawTriangle(x, y, 28, color, true);
}

void randomSquare(bool left, int color)
{
  int x = SCREENWIDTH / 2 + (left ? -30 : 4);
  int y = SCREENHEIGHT * 2 / 3 - 18;
  int w = 28;
  if (random(2) == 0)
  {
    display.drawRect(x, y, w, w, TSRectangleNoFill, color);
    display.drawRect(x + 4, y + 4, w - 8, w - 8, TSRectangleNoFill, color);
    return;
  }
  display.drawRect(x, y, w, w, TSRectangleFilled, color);
}

void randomCircle(bool left, int color)
{
  int x = SCREENWIDTH / 2 + (left ? -20 : 20);
  int y = SCREENHEIGHT * 2 / 3 - 4;
  if (random(2) == 0)
  {
    drawCircle(x, y, 14, color, false);
    drawCircle(x, y, 10, color, false);
    return;
  }
  drawCircle(x, y, 14, color, true);
}

void writeStatsToSD() {
  File file = SD.open("stats.txt", FILE_WRITE);

  if (file)
  {
    for (int i = 0; i < levels; i++) {
      if (stats[i].correct > 0 || stats[i].incorrect > 0) {
        file.print(stats[i].correct);
        file.print(",");
        file.print(stats[i].incorrect);
        file.print(";");
      }
    }
    file.println();
    file.close();
    SerialUSB.println("Stats written to SD card.");
  }
  else
  {
    SerialUSB.println("Error opening stats.txt");
  }
}


void readStatsFromSD() {
  File file = SD.open("stats.txt");
  if (file) {
    historySize = 0;
    while (file.available() && historySize < maxHistory) {
      String line = file.readStringUntil('\n');
      SerialUSB.println(line);
      int statCount = 0;
      int lastPos = 0;
      int pos = 0;

      while (lastPos < line.length() && statCount < levels) {
        pos = line.indexOf(';', lastPos);
        if (pos == -1) pos = line.length();
        String statString = line.substring(lastPos, pos);
        if (statString.length() > 0) {
          int commaPos = statString.indexOf(',');
          if (commaPos >= 0) {
            String correctStr = statString.substring(0, commaPos);
            String incorrectStr = statString.substring(commaPos + 1);
            int correct = correctStr.toInt();
            int incorrect = incorrectStr.toInt();
            SerialUSB.print(correct);
            SerialUSB.print(",");
            SerialUSB.print(incorrect);
            SerialUSB.println(";");
    
            history[historySize][statCount].correct = correct;
            history[historySize][statCount].incorrect = incorrect;
            statCount++;
          }
        }
        lastPos = pos + 1; // Move past the semicolon
      }
      historySize++;
    }
    file.close();
  } else {
    // Handle error opening file
    SerialUSB.println("Error opening stats.txt");
  }
}

// int readHighScoreFromSD()
// {
//   File file = SD.open("scores.txt");

//   if (file)
//   {
//     int highScore = 0;

//     while (file.available())
//     {
//       String line = file.readStringUntil('\n');
//       int score = line.toInt();
//       if (score > highScore)
//       {
//         highScore = score;
//       }
//     }
//     file.close();
//     SerialUSB.print("High score read from SD card: ");
//     SerialUSB.println(highScore);
//     return highScore;
//   }
//   else
//   {
//     SerialUSB.println("Error opening scores.txt");
//     return 0; // Return 0 if the file doesn't exist or can't be opened
//   }
// }

// void displayHighScore()
//{
//   display.fontColor(TS_8b_Yellow, TS_8b_Black);
//   String highScoreText = "High Score: " + String(game.highScore);
//
//   // Create a modifiable character array
//   char highScoreTextArray[30]; // Ensure the array is large enough
//   strcpy(highScoreTextArray, highScoreText.c_str());
//
//   int x = SCREENWIDTH / 2 - display.getPrintWidth(highScoreTextArray) / 2;
//   int y = SCREENHEIGHT - 15; // Adjust the position as needed
//   display.setCursor(x, y);
//   display.print(highScoreTextArray);
// }
