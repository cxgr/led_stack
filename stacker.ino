#include <LedControl.h>

#define DIN 8
#define CS 7
#define CLK 6

#define BTN_IN 4

LedControl lc = LedControl(DIN, CLK, CS, 4);

void setup()
{
  Serial.begin(9600);

  delay(20);

  pinMode(OUTPUT, DIN);
  pinMode(OUTPUT, CS);
  pinMode(OUTPUT, CLK);

  pinMode(INPUT, BTN_IN);

  for (int i = 0; i < 4; ++i)
  {
    lc.shutdown(i, false);
    lc.setIntensity(i, 1);
    lc.clearDisplay(i);
  }

  Restart();
}

bool btnClicked = false;
bool btnDown = false;
bool btnPressedLastFrame = false;

enum GameStates
{
  Standby,
  Active,
  Gameover
};
GameStates state = Standby;

int currentMatrix = 0;
int currentRow = 7;

int runnerSize = 5;
int runnerPos = 6;
bool movingLeft = true;
bool alterDir = true;

unsigned long ms = 0;
const unsigned long standbyTimeout = 10000;
const unsigned long endgameTick = 27;
unsigned long tickTimer = 0;
unsigned long idleTimer = 0;
unsigned long overTimer = 0;

const unsigned long startMoveTick = 120;
unsigned long currentMoveTick = 120;

byte stacksPlaced = 0;
byte lastStackedBits = 0;
byte survivingBits = 0;

bool won = false;

void loop()
{
  ms = millis();

  btnDown = digitalRead(BTN_IN);
  btnClicked = btnDown && !btnPressedLastFrame;

  switch (state)
  {
  case Standby:
    if (btnClicked)
    {
      WakeUp();
    }
    break;

  case Active:
    GameUpdate();
    if (btnClicked)
    {
      Stack();
      idleTimer = ms;
    }
    else
    {
      if (ms - idleTimer >= standbyTimeout)
      {
        Sleep();
      }
    }

    break;

  case Gameover:
    if (ms - overTimer >= endgameTick)
    {
      overTimer = ms;
      UpdateGameOverAnim();
    }
    break;

  default:
    break;
  }

  btnPressedLastFrame = btnDown;
}

void GameUpdate()
{
  if (millis() - tickTimer >= currentMoveTick)
  {
    if (movingLeft)
    {
      if (runnerPos <= -runnerSize + 1)
      {
        movingLeft = false;
        runnerPos++;
      }
      else
      {
        runnerPos--;
      }
    }
    else
    {
      if (runnerPos > 6)
      {
        movingLeft = true;
        runnerPos--;
      }
      else
      {
        runnerPos++;
      }
    }

    tickTimer = millis();
  }
  lc.setColumn(currentMatrix, currentRow, GetRunnerBits());
  lc.setColumn(currentMatrix, currentRow - 1, GetRunnerBits());
}

void Stack()
{
  survivingBits = GetRunnerBits() & (stacksPlaced <= 0 ? 0b11111111 : lastStackedBits);
  stacksPlaced++;
  lastStackedBits = survivingBits;
  runnerSize = CountBits(survivingBits);
  lc.setColumn(currentMatrix, currentRow, survivingBits);
  lc.setColumn(currentMatrix, currentRow - 1, survivingBits);

  if (currentRow >= 2)
  {
    currentRow -= 2;
  }
  else if (currentMatrix < 3)
  {
    currentMatrix++;
    currentRow = 7;
  }

  if (runnerSize <= 0 || currentMatrix >= 4 || stacksPlaced == 16)
  {
    EndGame(stacksPlaced >= 16);
  }
  else
  {
    movingLeft = !alterDir;
    runnerPos = alterDir ? 1 - runnerSize : 6;
    currentMoveTick = startMoveTick - 20 * currentMatrix - 3 * (7 - currentRow) / 2;
    alterDir = !alterDir;
  }
}

byte GetRunnerBits()
{
  byte b = B0;
  for (int i = 0; i < 8; ++i)
  {
    if (i >= runnerPos && i < runnerPos + runnerSize)
      b |= (byte)(1 << (7 - i));
  }
  return b;
}

int CountBits(byte b)
{
  byte mask = 1;
  int cnt = 0;
  for (int i = 0; i < 8; ++i)
  {
    cnt += (abs(b & mask) > 0);
    mask <<= 1;
  }
  return cnt;
}

void EndGame(bool won)
{
  state = Gameover;
  overTimer = ms;

  if (won)
  {
    for (int i = 0; i < 3; ++i)
    {
      Flash(true);
      delay(50);
      Flash(false);
      delay(100);
    }
  }
  else
  {
    delay(500);
    currentMatrix = 3;
    currentRow = 0;
    lc.setColumn(currentMatrix, currentRow, 0b11111111);
  }
}

void UpdateGameOverAnim()
{
  lc.setColumn(currentMatrix, currentRow, 0);
  currentRow++;
  if (currentRow > 7)
  {
    currentRow = 0;
    currentMatrix--;
  }

  if (currentMatrix < 0)
  {
    lc.setColumn(0, 7, 0);
    state = Active;
    Restart();
  }
  else
  {
    lc.setColumn(currentMatrix, currentRow, 0b11111111);
  }
}

void Restart()
{
  runnerSize = 5;
  runnerPos = 6;
  movingLeft = true;
  alterDir = true;

  ms = millis();
  tickTimer = ms;
  idleTimer = ms;
  overTimer = ms;

  currentMoveTick = startMoveTick;

  stacksPlaced = 0;
  lastStackedBits = 0;
  survivingBits = 0;

  currentRow = 7;
  currentMatrix = 0;

  for (int i = 0; i < 4; ++i)
  {
    lc.clearDisplay(i);
  }
}

void Sleep()
{
  state = Standby;

  for (int i = 0; i < 4; ++i)
  {
    lc.clearDisplay(i);
    lc.shutdown(i, true);
  }
}

void WakeUp()
{
  for (int i = 0; i < 4; ++i)
  {
    lc.shutdown(i, false);
    lc.setIntensity(i, 1);
    lc.clearDisplay(i);
  }
  Restart();
  state = Active;  
}

void Flash(bool on)
{
  for (int m = 0; m < 4; ++m)
  {
    for (int r = 0; r < 8; ++r)
    {
      lc.setRow(m, r, on ? 0b11111111 : 0);
    }
  }
}