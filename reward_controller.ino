/*
  To connect with Picto (as of 07/16/2021), set the COM port of the Arduino to 3 once it's connected in Windows device manager.
*/

#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

#define OFF 0x0
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

#define SIGNAL12V 26
#define SIGNAL24V 27
#define POWERBUTTON 28
#define REWARDJACK 29
#define FLUSHBUTTON 30
#define REWARDBUTTON 31
#define REWARDLED 32
#define POWERLED 33
#define ROTARY3 48
#define ROTARY2 50
#define ROTARY1 52

#define GATESIGNAL 6
#define TRIGGERSIGNAL 7

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

const uint8_t rs = 53, rw = 51, en = 49, d4 = 47, d5 = 45, d6 = 43, d7 = 41;

int default_reward = 50, default_increment = 50, ACTIVESIGNAL = SIGNAL12V, x, y;
char command, buf[5];
bool flushing = false, dial_set = false, reset_ = false;

void flush_(bool f)
{
  digitalWrite(ACTIVESIGNAL, f);
  digitalWrite(REWARDLED, f);
  flushing = f;

  lcd.clear();
  
  if(f)
    lcd.print(F("Flushing..."));
  else
  {
    lcd.print(F("Reward Amount:"));
    lcd.setCursor(0,1);
    lcd.print(default_reward);
    lcd.print(F("    "));
  }
}

/*
  Give a reward for 'amount' ms. If this was requested by Serial, send a confirmation once the reward is finished
*/
void reward(int amount, bool serial)
{
  if(flushing)
    flush_(false);
    
  digitalWrite(ACTIVESIGNAL, HIGH);
  digitalWrite(REWARDLED, HIGH);
  
  delay(amount);
  
  digitalWrite(ACTIVESIGNAL, LOW);
  digitalWrite(REWARDLED, LOW);

  if(serial)
    Serial.print(1);
}

void processCommand(char c, int v)
{
  if(c == '1')
    reward(v, true);
  else if(c == '2')
    flush_(true);
  else if(c == '3')
    flush_(false);
}

/*
  Serial commands are 6 bytes long with a leading character, followed by a space, then a 4 digit integer between 0000 - 9999.
  A command of "1 1000" would mean to give a 1000ms reward.
  A command of "2 0000" would mean to start a flush.
  A command of "3 0000" would mean to stop a flush.
*/
void serial_check()
{
  if(Serial.available() >= 6)
  {  
    Serial.readBytes(&command, 1);
    Serial.read();
    Serial.readBytes(buf, 4);
    
    buf[4] = '\0';
    processCommand(command, atoi(buf));
  }
}

void button_check()
{
  //Check if button has been pressed
  if(!digitalRead(REWARDBUTTON))
  {
    //Wait for it to be released
    while(!digitalRead(REWARDBUTTON))
    {
      delay(100);
    }
    reward(default_reward, false);
  }
  else if(!digitalRead(REWARDJACK))
  {
    reward(default_reward, false);
    while(!digitalRead(REWARDJACK))
    {
      delay(100);
    }
  } 
  else if(!digitalRead(FLUSHBUTTON))
  {
    while(!digitalRead(FLUSHBUTTON))
    {
      delay(100);
    }
    flush_(!flushing);
  }
  else if(!digitalRead(POWERBUTTON))
  {
    while(!digitalRead(POWERBUTTON))
    {
      delay(100);
    }
    reset_ = true;
  }
}

void dial_check()
{
  x = digitalRead(ROTARY3);
  y = digitalRead(ROTARY2);

  //Increment manual reward amount when dial is turned
  if(x < y && dial_set)
  {
    default_reward += default_increment * (default_reward < 10000);
    lcd.setCursor(0,1);
    lcd.print(default_reward);
    lcd.print(F("    "));
    dial_set = false;
  }
  else if(x > y && dial_set)
  {
    default_reward -= default_increment * (default_reward > default_increment);
    lcd.setCursor(0,1);
    lcd.print(default_reward);
    lcd.print(F("    "));
    dial_set = false;
  }
  else if(x == 1 && y == 1)
  {
    dial_set = true;
  }
}

void rewardLoop()
{
  while(!reset_)
  {
    serial_check();
    button_check();
    dial_check();
  }
}

void setup() 
{ 
  lcd.init(true, rs, rw, en, d4, d5, d6, d7, -1, -1, -1, -1);
  lcd.begin(16,2);

  //Write to the LCD screen
  lcd.setCursor(0,0);
  lcd.print(F("Reward Amount:"));
  lcd.setCursor(0,1);
  lcd.print(default_reward);

  //Initialize Serial
  Serial.begin(9600);
  Serial.print(0);
  
  //Set the hardware pinmodes
  pinMode(ACTIVESIGNAL, OUTPUT);
  pinMode(REWARDLED, OUTPUT);
  pinMode(POWERLED, OUTPUT);

  pinMode(POWERBUTTON, INPUT);
  pinMode(REWARDJACK, INPUT);
  pinMode(FLUSHBUTTON, INPUT);
  pinMode(REWARDBUTTON, INPUT);
  
  pinMode(ROTARY3, INPUT);
  pinMode(ROTARY2, INPUT);
  pinMode(ROTARY1, INPUT);

  //Set the button pins high
  digitalWrite(POWERBUTTON, HIGH);
  digitalWrite(FLUSHBUTTON, HIGH);
  digitalWrite(REWARDBUTTON, HIGH);
  digitalWrite(REWARDJACK, HIGH);

  digitalWrite(POWERLED, HIGH);
}

void loop() 
{
  rewardLoop();
  reset_ = false;

  //Reset the LCD screen in case it became unsynced
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print(F("Reward Amount:"));
  lcd.setCursor(0,1);
  lcd.print(default_reward);
  lcd.print(F("    "));
}
