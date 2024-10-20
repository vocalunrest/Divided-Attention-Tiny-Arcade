#include <TinyScreen.h>
#include <Wire.h>
#include <SPI.h>
#include "TinyArcade.h"

TinyScreen display = TinyScreen(TinyScreenPlus);

#define SCREENWIDTH 94 
#define SCREENHEIGHT 64

int state = -1; // 1 is gameplay, 2 is game over. 0 reserved for tutorial

int timerMax = 1500; // how long, in milliseconds, do we have to answer
int timerStartMillis; // when did we start the timer
int timerX = 0; // the last drawn X position of the timer
int shapesHidden = false; // we hide the shapes halfway through the timer; have we done it yet?

int threshold = 15; // how many correct answers to progress to the next level
int levels = 2;
char* prompts[] = {"Same color", "Same shape"};

int correct = 0;
int startingLives = 15;
int lives;
int level = 0; // increments to 1 by nextLevel()

int currSameShape = false; // are the currently generated shapes the same shape?
int currSameColor = false;

// When button is clicked, becomes false until released
bool clickable1 = true;
bool clickable2 = true;

int colors[] = {TS_8b_Blue, TS_8b_Red, TS_8b_Yellow};

void setup() {
  // put your setup code here, to run once:
  USBDevice.init();
  USBDevice.attach();
  SerialUSB.begin(9600);
  randomSeed(analogRead(2));
  arcadeInit();
  display.begin();
  display.setBrightness(10); //0-15
  display.setFont(thinPixel7_10ptFontInfo);

  nextLevel();
}

void loop() {
  checkInput();
  if (state == 1) {
     updateTimer();
  }
}

void checkInput() {
  if (state == 2 && ((checkButton(TAButton2) && clickable2) || (checkButton(TAButton1) && clickable1))) {
    level = 0;
    nextLevel();
  }

  if (state == 1) {
    if (checkButton(TAButton2) && clickable2) {
      if ((level == 1 && currSameColor) || (level == 2 && currSameShape)) {
         next(true);
       } else {
       next(false);
       }
    }
    else if (checkButton(TAButton1) && clickable1) {
      if ((level == 1 && !currSameColor) || (level == 2 && !currSameShape)) {
        next(true);
      }
      else { next(false); }
    }
  }

  if (checkButton(TAButton2)) {
    clickable2 = false;
  } else {
    clickable2 = true;
  }
  if (checkButton(TAButton1)) {
    clickable1 = false;
  } else {
    clickable1 = true;
  }

  
//  if (checkButton(TAButton2)) {
//      if (clickable2 == true) {
//         clickable2 = false;
//
//      }
//      return;
//    }
//    clickable2 = true;
//    if (checkButton(TAButton1)) {
//      if (clickable1 == true) {
//        clickable1 = false;
//        
//        next(false);
//      }
//      return;
//    }
//    clickable1 = true;
//  }
}

void updateTimer() {
  int progress = millis() - timerStartMillis;
  int newTimerX = round(((double) progress / (double) timerMax) * SCREENWIDTH);

  if (!shapesHidden && progress >= timerMax / 2) {
    shapesHidden = true;
    display.clearWindow(0, 20, SCREENWIDTH, SCREENHEIGHT - 25); 
  }
  if (progress >= timerMax) {
    next(false);
    return;
  }
  if (newTimerX != timerX) {
    display.drawLine(newTimerX,SCREENHEIGHT - 4,newTimerX,SCREENHEIGHT,TS_8b_Red);
  }
}
void nextLevel() {
  if (level == levels) {
    gameOver();
    return;
  }
  state = 1;
  level++;
  lives = startingLives;
  correct = 0;
  currSameShape = false;
  currSameColor = false;
  timerStartMillis = millis(); 
  display.clearWindow(0, 0, SCREENWIDTH + 1, SCREENHEIGHT);
  shapesHidden = false;
  drawHUD();
  drawShapes();
}

void gameOver () {
  state = 2;
  display.clearWindow(0, 0, SCREENWIDTH + 1, SCREENHEIGHT);
  display.fontColor(TS_8b_White,TS_8b_Black);
  if (lives == 0) {
    char* txt = "Game Over";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(txt) / 2, 10);
    display.print(txt);
    txt = "You got to level ";
    display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(txt) / 2, 25);
    display.print(txt);
    display.setCursor(SCREENWIDTH / 2 + display.getPrintWidth(txt) / 2, 25);
    display.print(level);
    return;
  }
  char* txt = "You win";
  display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(txt) / 2, 10);
  display.print(txt);
}

void drawHUD () {
  display.fontColor(TS_8b_Green,TS_8b_Black);
  display.setCursor(2, 10);
  display.print(correct);

  display.fontColor(TS_8b_White,TS_8b_Black);
  display.setCursor(SCREENWIDTH / 2 - display.getPrintWidth(prompts[level - 1]) / 2, 10);
  display.print(prompts[level - 1]);
  
  display.fontColor(TS_8b_Red,TS_8b_Black);
  display.setCursor(SCREENWIDTH - display.getPrintWidth("ww"), 10);
  display.print(lives);
}

void next(bool wasCorrect) {
  if (wasCorrect) {
    correct++;
  } else {
    lives--;
    if (lives == 0) {
      gameOver();
      return;
    }
  }
  if (correct == threshold) {
    nextLevel();
    return;
  }
  timerStartMillis = millis(); 
  display.clearWindow(0, 20, SCREENWIDTH + 1, SCREENHEIGHT);
  shapesHidden = false;
  drawHUD();
  drawShapes();
}

void drawShapes() {
  if (level == 1) {
    // We want to get two different colors half the time (shape is totally random)
    int leftColorIndex = random(3);
    int rightColorIndex;
    if (random(2) == 0) {
      rightColorIndex = leftColorIndex;
      currSameColor = true;
    } else {
      // choose the other color and make sure it's different
      rightColorIndex = random(2);
      if (rightColorIndex >= leftColorIndex) {
        rightColorIndex++;
      }
      currSameColor = false;
    }

    drawShape(true, colors[leftColorIndex], random(3));
    drawShape(false, colors[rightColorIndex], random(3));
  }
  if (level == 2) {
    // We want to get two different shapes half the time (color is totally random)
    int leftShapeType = random(3);
    int rightShapeType;
    if (random(2) == 0) {
      rightShapeType = leftShapeType;
      currSameShape = true;
    } else {
      rightShapeType = random(2);
      if (rightShapeType >= leftShapeType) {
        rightShapeType++;
      }
      currSameShape = false;
    }

    drawShape(true, colors[random(3)], leftShapeType);
    drawShape(false, colors[random(3)], rightShapeType);
  }
}

void drawShape(bool left, int color, int shapeType) {
  switch (shapeType) {
    case 0: randomTriangle(left, color); break;
    case 1: randomSquare(left, color); break;
    case 2: randomCircle(left, color); break;
  }
}

void randomTriangle(bool left, int color) {
  int x = SCREENWIDTH / 2 + (left ? -20 : 20);
  int y = SCREENHEIGHT * 2 / 3;
  if (random(2) == 0) {
    drawTriangle(x, y, 12, color, false);
    drawTriangle(x, y, 24, color, false);
    return;
  }
  drawTriangle(x, y, 24, color, true);
}

void randomSquare(bool left, int color) {
  int x = SCREENWIDTH / 2 + (left ? -26 : 10);
  int y = SCREENHEIGHT * 2 / 3 - 14;
  if (random(2) == 0) {
    display.drawRect(x, y, 24, 24, TSRectangleNoFill, color);
    display.drawRect(x + 4, y + 4, 24 - 8, 24 - 8, TSRectangleNoFill, color);
    return;
  }
  display.drawRect(x, y, 24, 24, TSRectangleFilled, color);
}

void randomCircle(bool left, int color) {
  int x = SCREENWIDTH / 2 + (left ? -20 : 20);
  int y = SCREENHEIGHT * 2 / 3 - 2;
    if (random(2) == 0) {
    drawCircle(x, y, 12, color, false);
    drawCircle(x, y, 8, color, false);
    return;
  }
  drawCircle(x, y, 12, color, true);
}
