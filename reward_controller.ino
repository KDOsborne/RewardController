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

#define GATESIGNAL 35
#define TRIGGERSIGNAL 37

#define FLUSHDURATION 600000 //600000 ms = 10 minutes

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

const uint8_t rs = 53, rw = 51, en = 49, d4 = 47, d5 = 45, d6 = 43, d7 = 41;

int default_reward = 50, default_increment = 50, ACTIVESIGNAL = SIGNAL12V, x, y;
unsigned long flush_time = 0;
bool flushing = false, dial_set = false, reset_ = false;

void (*reset) (void) = 0;

void reset_lcd(bool clear)
{
	if(clear)
		lcd.clear();
	lcd.print(F("Reward Amount:"));
	lcd.setCursor(0,1);
	lcd.print(default_reward);
	lcd.print(F("    "));
}
void flush_(bool f)
{
  digitalWrite(ACTIVESIGNAL, f);
  digitalWrite(REWARDLED, f);
  flushing = f;
  
  lcd.clear();
  
  if(f)
  {
    flush_time = millis();
    lcd.print(F("Flushing..."));
  }
  else
  {
    reset_lcd(false);
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
}

void bnc_check()
{
	if(digitalRead(GATESIGNAL))
	{
		digitalWrite(ACTIVESIGNAL, HIGH);
		digitalWrite(REWARDLED, HIGH);
    
		lcd.clear();
		lcd.print(F("Rewarding..."));
		
		while(digitalRead(GATESIGNAL))
		{
			delay(1);
		}
		digitalWrite(ACTIVESIGNAL, LOW);
    digitalWrite(REWARDLED, LOW);
		reset_lcd(true);
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
    lcd.clear();
    lcd.print(F("Release to reset..."));
    
    while(!digitalRead(POWERBUTTON))
    {
      delay(100);
    }
    reset();
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

void flush_check()
{
  if(flushing)
  {
    if((millis() - flush_time >= FLUSHDURATION) || (millis() < flush_time))
      flush_(false);
  }
}

void rewardLoop()
{
  while(!reset_)
  {
	  bnc_check();
    button_check();
    dial_check();
    flush_check();
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
  
  //Set the hardware pinmodes
  pinMode(ACTIVESIGNAL, OUTPUT);
  pinMode(REWARDLED, OUTPUT);
  pinMode(POWERLED, OUTPUT);
  pinMode(GATESIGNAL, INPUT);
  //pinMode(TRIGGERSIGNAL, OUTPUT);
  
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
  reset_lcd(false);
}
