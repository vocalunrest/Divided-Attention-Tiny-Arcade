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

int const maxHistory = 11; // the max # of session bars to show (includes both SD history and current session)
LevelStats history[maxHistory][levels]; // stats loaded from SD card
LevelStats currentSession[maxHistory][levels]; // stats from games played this session
int historySize = 0; // how many previous sessions were actually loaded
int currentSessionSize = 0; // how many games were played this session

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
//    SD.remove("stats.txt");
    printFile("stats.txt");
    readStatsFromSD();
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
    if ((tutorialStep == 6 || tutorialStep == 9)) {
      if (leftPressed) {
        nextTutorialStep();
      }
    } else if (rightPressed) {
      nextTutorialStep();
    }
  }
  
  if (screen == END && (leftPressed || rightPressed)) {
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
  if (tutorialStep > 10)
  {
    nextLevel();
  }
}

void displayTutorialStep()
{
  display.clearWindow(0, 0, SCREENWIDTH + 1, SCREENHEIGHT);
  display.fontColor(TS_8b_White, TS_8b_Black);

  struct TutorialText {
      const char* text;
      int yPos;
  };

  TutorialText tutorialSteps[11][5] = {
      {{"Cognitive Therapy", 5}, {"Game", 15}, {"Improving", 30}, {"Focus & Memory", 40}, {"[Press right]", 55}},
      {{"Our goal is to", 12}, {"improve cognitive", 22}, {"skills like focus", 32}, {"and attention!", 42}, {"[Press right]", 55}},
      {{"Play to train your", 12}, {"brain & Practice", 22}, {"important skills!", 32}, {"", 0}, {"[Press right]", 55}},
      {{"Buttons:", 2}, {"Right = Match", 22}, {"Left = Mismatch", 32}, {"", 0}, {"[Press right]", 55}},
      {{"Level 1: Colors", 2}, {"Keep your focus", 22}, {"on the color", 32}, {"of objects", 42}, {"[Press right]", 55}},
      {{"Level 1: Colors", 2}, {"Possible senarios:", 10}, {"", 0}, {"", 0}, {"[Right = Match]", 55}},
      {{"Level 1: Colors", 2}, {"Possible senarios:", 10}, {"", 0}, {"", 0}, {"[Left = Mismatch]", 55}},
      {{"Level 2: Shapes", 2}, {"Keep your focus", 22}, {"on the shapes", 32}, {"of objects", 42}, {"[Press right]", 55}},
      {{"Level 2: Shapes", 2}, {"Possible senarios:", 10}, {"", 0}, {"", 0}, {"[Right = Match]", 55}},
      {{"Level 2: Shapes", 2}, {"Possible senarios:", 10}, {"", 0}, {"", 0}, {"[Left = Mismatch]", 55}},
      {{"Start Game!", 15}, {"", 0}, {"", 0}, {"", 0}, {"[Press right]", 55}}
  };
  
  int length = sizeof(tutorialSteps[tutorialStep]) / sizeof(tutorialSteps[tutorialStep][0]);
  for (int i = 0; i < length; i++) {
      if (strlen(tutorialSteps[tutorialStep][i].text) > 0) {
          int setCursor = SCREENWIDTH / 2 - display.getPrintWidth((char*)tutorialSteps[tutorialStep][i].text) / 2;
          display.setCursor(setCursor, tutorialSteps[tutorialStep][i].yPos);
          display.print(tutorialSteps[tutorialStep][i].text);
          if (tutorialStep == 5){
            drawShape(true, TS_8b_Yellow, 1);  // Left: Yellow square
            drawShape(false, TS_8b_Yellow, 2); // Right: Yellow circle
          }
          else if (tutorialStep == 6) {
            drawShape(true, TS_8b_Blue, 1);  // Left: Blue square
            drawShape(false, TS_8b_Yellow, 2); // Right: Yellow circle
          }
          else if (tutorialStep == 8) {
            drawShape(true, TS_8b_Blue, 0); // Left: Blue triangle
            drawShape(false, TS_8b_Red, 0); // Right: Red triangle
          }
          else if (tutorialStep == 9) {
            drawShape(true, TS_8b_Blue, 1); // Left: Blue square
            drawShape(false, TS_8b_Blue, 0); // Right: Blue triangle
          }
      }
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

void printStatBar(struct LevelStats stats[], int xStart)
{
  const int levelHeight = 20;
  const int incompleteLevelHeight = 15;
  const int gap = 2;
  const int width = 6;
  const int fromBottom = 2;

  SerialUSB.println("Printing statline " + xStart);
  for (int i = 0; i < levels; i++)
  {
    if (stats[i].correct > 0 || stats[i].incorrect > 0)
    { // if we got to this level
      int bottom = SCREENHEIGHT - (levelHeight + gap) * i - fromBottom;
      int total = stats[i].correct + stats[i].incorrect;
      int totalHeight = stats[i].correct >= threshold ? levelHeight : incompleteLevelHeight;
      int correctPixels = rintf((float)stats[i].correct * totalHeight / total);
      int incorrectPixels = rintf((float)stats[i].incorrect * totalHeight / total);
      if (correctPixels > 0) {
        display.drawRect(xStart, bottom - totalHeight, width, correctPixels, TSRectangleFilled, TS_8b_Green);
      }
      if (incorrectPixels > 0) {
        display.drawRect(xStart, bottom - totalHeight + correctPixels, width, incorrectPixels, TSRectangleFilled, TS_8b_Red);
      }
    }
  }
}

void gameOver()
{
  screen = END;
  display.clearWindow(0, 0, SCREENWIDTH + 1, SCREENHEIGHT);
  display.fontColor(TS_8b_White, TS_8b_Black);

  memcpy(currentSession[currentSessionSize++], stats, sizeof(stats));

  int totalBars = min(currentSessionSize + historySize, maxHistory);
  int sessionBars = min(currentSessionSize, totalBars);
  
  for (int i = 0; i < sessionBars; i++) {
    printStatBar(currentSession[sessionBars - i - 1], (totalBars - i - 1) * 8);
  }

  for (int i = 0; i < totalBars - sessionBars; i++) {
    printStatBar(history[i], (totalBars - sessionBars - i - 1) * 8);
  }

  if (totalBars > 0) {
    display.drawLine((totalBars - 1)* 8, SCREENHEIGHT - 1, (totalBars - 1) * 8 + 6, SCREENHEIGHT - 1, TS_8b_Yellow);
  }
  
  writeStatsToSD();

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
    drawRoundedRect(x, y, w, w, 5, color, false);
    // drawSprite(int x, int y, int width, int height, int radius, int color, bool fill) {
    drawRoundedRect((x + 4), (y + 4), (w - 8), (w - 8), 5, color, false);
    return;
  }
  drawRoundedRect(x, y, w, w, 5, color, true);

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
    file.seek(0);
    // Copy the file, prepend the new line, then overwrite the file
    String prev = "";
    while (file.available()) {
      prev += file.readStringUntil('\n') + '\n';
    }
    String next = "";
    for (int i = 0; i < levels; i++) {
      if (stats[i].correct > 0 || stats[i].incorrect > 0) {
        next.concat(stats[i].correct);
        next.concat(",");
        next.concat(stats[i].incorrect);
        next.concat(";");
      }
    }
    file.seek(0);
    file.println(next);
    file.print(prev);
    file.close();
    SerialUSB.print("Stats written to SD card: ");
    SerialUSB.println(next);
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

//For testing
void printFile(char* filename) {
  File file = SD.open(filename);
  if (file) {
    SerialUSB.print("Printing ");
    SerialUSB.println(filename);
    while(file.available()) {
      String line = file.readStringUntil('\n');
      SerialUSB.println(line);
    }
    SerialUSB.println("----");
    file.close();
    return;
  } 
  SerialUSB.print(filename);
  SerialUSB.println(" does not exist");
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
