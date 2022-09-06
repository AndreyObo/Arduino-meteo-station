#include <Wire.h>
#include <DS3231.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include<string.h>

#define BackButton 3 //Grey wire
#define UpButton 4 // White wire 
#define DownButton 5 //Pink wire
#define OkButton 6 //Blue wire

#define scale_size 17

LiquidCrystal_I2C lcd(0x27,20,4);

int Temp_change[scale_size] = {100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100};
int Press_change[scale_size] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int Hum_change[scale_size] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

int chart_index;


//Custom characters for drawing chart on lc dispaly
byte Arrow[8] = {
  0b00000,
  0b00100,
  0b00010,
  0b11111,
  0b00010,
  0b00100,
  0b00000,
  0b00000
};


byte Graph[8][8] = {
  {0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111 },
  
 {0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b11111 },
  
  {0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b11111,
  0b11111 },
  
  {0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b11111,
  0b11111,
  0b11111 },

  {0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111 },

  {0b00000,
  0b00000,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111 },

  {0b00000,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111 },

 {0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111 }
};

//App menu
class UserMenu {
  public:
  void addItems(String it) {
    Items[index] = it;
    index +=1;
  }
  void ShowMenu(int it2, bool item_exit) {
    if(item_exit) {
      scoreCl =0;
      scoreDb =0;
      block =0;
    }
    lcd.clear();
    lcd.createChar(0, Arrow);
    lcd.setCursor(0,0);
    lcd.write(byte(0));
    for(int i=0; i <= 3; i++) {
      if((it2+i) < index) {
      lcd.setCursor(1,i);
      lcd.print(Items[it2+i]);
      }
    }
  }
  
  void CheckNext() {
    if (scoreCl+1 != index) {
               if (scoreDb ==3) {
                   ShowMenu(scoreCl+(3*block+1), false);
                   scoreCl +=1;
                   scoreDb =0;
                   block +=1;
               }
               else {
                lcd.setCursor(0,scoreDb);
                lcd.print(" ");
                lcd.setCursor(0,scoreDb+1);
                lcd.write(byte(0));
                scoreDb +=1;
                scoreCl +=1;
               }
    
    }
  
  }
  void CheckBack() {
    if (scoreCl !=0) {
               if (scoreDb ==0 && scoreCl !=0) {
                   ShowMenu(scoreCl-(3*block+1), false);
                   lcd.setCursor(0,0);
                   lcd.print(" ");
                   lcd.setCursor(0,3);
                   lcd.write(byte(0));
                   scoreCl -=1;
                   scoreDb =3;
                   block -=1;
               }
               else {
                   lcd.setCursor(0,scoreDb);
                   lcd.print(" ");
                   lcd.setCursor(0,scoreDb-1);
                   lcd.write(byte(0));
                   scoreCl -=1;
                   scoreDb -=1;
               }
    }
  }
  int RetIndex() {
    return scoreCl;
  }
  
  private:
  int block = 0;
  int scoreDb=0;
  int scoreCl=0;
  int index=0;
  String Items[10]; 
};


boolean back_but_press;

enum Station_state {ShowAll, MenuActive, ShowTempGr, ShowPressGr, ShowHummGr, ShowAllGr}; //App state

enum Measure_type {temperature, pressure, humidity};

unsigned long last_millis_time;
unsigned long last_millis_meteo;

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C

DS3231  rtc(SDA, SCL);

Time  t;

String s;
String temp;
String presse;
String hm;

bool flash;

UserMenu Menu;

Station_state state; 

Measure_type measure;

void setup() {

 chart_index = -1;

  flash = false;
  
  last_millis_time = 0;
  last_millis_meteo = 0;
  rtc.begin();
 
  lcd.init();                     
  lcd.init();
  lcd.backlight();  
  s = "Time - ";
  temp = "T(C) - ";
  presse = "Pr(mm Hg) - ";
  hm = "Hum(%) - ";
  bme.begin();  

  Menu.addItems("Main screen");
  Menu.addItems("Temperature chart");
  Menu.addItems("Pressure chart");
  Menu.addItems("Humidity chart");
  Menu.addItems("Clock settings");
  Menu.addItems("About LiveWether");

  state = MenuActive;
}

unsigned long int PrevTime;

void virt_interrupt() {
  if(millis()-PrevTime > 30000) {
    PrevTime=millis();
 if(chart_index == 16) {
    ShfArray(Temp_change);
    ShfArray(Press_change);
    ShfArray(Hum_change);
   }
   else {
    chart_index++;
   }    
      
   double val_d = bme.readTemperature();
   int val_i = (int)val_d;
   double val_dec =val_d- val_i;
  
   if(val_dec > 0.5) {
      Temp_change[chart_index] = val_i+1;
  }
  else {
     Temp_change[chart_index] = val_i;
  }

  int val_pr = (int)((bme.readPressure() / 100.0F)*0.75);        
  Press_change[chart_index] = val_pr;

   val_d = bme.readHumidity();
   val_i = (int)val_d;
   val_dec =val_d- val_i;
   
   if(val_dec > 0.5) {
      Hum_change[chart_index] = val_i+1;
  }
  else {
    Hum_change[chart_index] = val_i;
  }
  }
}

void InterruptFunct() {
   lcd.setCursor(0,0);
   lcd.print("Interupt");
   delay(1000);
}


void ShfArray(int* mass) { //using scale_size define for all memory array
  for(int i =0; i <= scale_size -2; i++) {
     mass[i]=mass[i+1];
  }
}

void PrintClock(int col, int pos) {
 t = rtc.getTime();
 if(t.hour >=0 && t.hour < 10) {
   lcd.setCursor(pos,col);
   lcd.print("0");
   lcd.setCursor(pos+1,col);
   lcd.print(t.hour);
 }
 else {
 lcd.setCursor(pos,col);
 lcd.print(t.hour);
 }
 pos +=2;
 lcd.setCursor(pos,col);
 lcd.print(":");
 pos +=1;
 if(t.min >=0 && t.min < 10) {
   lcd.setCursor(pos,col);
   lcd.print("0");
   lcd.setCursor(pos+1,col);
   lcd.print(t.min);
 }
 else {
 lcd.setCursor(pos,col);
 lcd.print(t.min);
 }
 pos +=2;
 lcd.setCursor(pos,col);
 lcd.print(":");
 pos+=1;
 if(t.sec >=0 && t.sec < 10) {
   lcd.setCursor(pos,col);
   lcd.print("0");
   lcd.setCursor(pos+1,col);
   lcd.print(t.sec);
 }
 else {
 lcd.setCursor(pos,col);
 lcd.print(t.sec);
 }
 
}

void PrintTemp(int col, int pos) {
  lcd.setCursor(pos,col);
  lcd.print(bme.readTemperature());
}

void PrintPressure(int col, int pos) {
  lcd.setCursor(pos,col);
  lcd.print((bme.readPressure() / 100.0F)*0.75);
}

void PrintHumidity(int col, int pos) {
  lcd.setCursor(pos,col);
  lcd.print(bme.readHumidity());
}

void loop() {
 switch(state) {
  case MenuActive:
       StartMenu();
  break;
  case ShowAll:
       lcd.clear();
       PrintAll();
  break;
  case ShowTempGr:
        lcd.clear();
        Print_temp_Gr_test(Temp_change, temperature,100);
  break;
  case ShowPressGr:
        lcd.clear();
        Print_temp_Gr_test(Press_change, pressure,-1);
  break;
  case ShowHummGr:
        lcd.clear();
        Print_temp_Gr_test(Hum_change, humidity,-1);
  break; 
 }
 
}

void PrintAll() {
  back_but_press = false;
  while(!back_but_press) {
  virt_interrupt();
  if(millis() - last_millis_time > 1000) {
 lcd.setCursor(0,0); 
 lcd.print(s);
 PrintClock(0, s.length());
  last_millis_time = millis();
 }

 if(millis() - last_millis_meteo > 5000) {
 lcd.setCursor(0,1);
 lcd.print(temp);
 PrintTemp(1, temp.length());
 
 lcd.setCursor(0,2);
 lcd.print(presse);
 PrintPressure(2, presse.length());

 lcd.setCursor(0,3);
 lcd.print(hm);
 PrintHumidity(3, hm.length());
 last_millis_meteo = millis();
 }
 back_but_press = digitalRead(BackButton);
  }
   state = MenuActive;
}

void StartMenu() {
  boolean ActiveM = false;
  boolean ExitCode = true;
  lcd.clear();
  Menu.ShowMenu(0, true);
  while(ExitCode) {
    if(digitalRead(DownButton)) {
      Menu.CheckNext();
      delay(1000);
    }
    if(digitalRead(UpButton)) {
      Menu.CheckBack();
      delay(1000);
    }
    if(digitalRead(OkButton)) {

      switch(Menu.RetIndex()) {
        case 0:
              state = ShowAll;
              ExitCode = false;
              break;   
        case 1:
              state = ShowTempGr;
              ExitCode = false;
              break;   
        case 2:
              state = ShowPressGr;
              ExitCode = false;
              break;
        case 3:
             state = ShowHummGr;
             ExitCode = false;
             break;
      }
      delay(100);
    }
  }
}

void Print_temp_Gr_test(int* measure_arr, Measure_type type_param, int mask) {
  back_but_press = false;
  if(measure_arr[0]==mask) {
    lcd.setCursor(0,0);
    lcd.print("Stations have'not date");
  }
  else {
  int min_val = min_value(measure_arr, mask);
  int max_val = max_value(measure_arr, mask);

  int step_val = 24/(max_val-min_val);

  int res;

  if(min_val == max_val) {
    int space;
    lcd.setCursor(0,0);
    lcd.print("From the last 8 hour");
    lcd.setCursor(0,1);
    switch(type_param) {
    case temperature:
        lcd.print("Temperature");    
        space = 10;  
    break;
    case pressure:
        lcd.print("Pressure");     
        space = 7;  
    break;
    case humidity:
         lcd.print("Humidity");     
         space = 7;  
    break;
  }
   lcd.setCursor(space+1,1);
   lcd.print("=");
   lcd.setCursor(space+3,1);
   lcd.print(min_val);  
  }
  else {
    
  for(int i=0; i <=7; i++) {
    lcd.createChar(i, Graph[i]);
  } 
 
  for(int i = 0; i <= scale_size-1; i++) {
    if(measure_arr[i] != 100) {
      if(measure_arr[i] == min_val) {
        res = step_val;
      }
      else {
      res = ((measure_arr[i]-min_val)*step_val)+step_val;
      }
      Build_coll(res,i);
    }
  }
  
  lcd.setCursor(0,0);
  switch(type_param) {
    case temperature:
        lcd.print("Temperature");
        lcd.setCursor(18,3);
        lcd.print(min_val);
        lcd.setCursor(18,1);
        lcd.print(max_val);
    break;
    case pressure:
        lcd.print("Pressure");
        lcd.setCursor(17,3);
        lcd.print(min_val);
        lcd.setCursor(17,1);
        lcd.print(max_val);
    break;
    case humidity:
         lcd.print("Humidity");
         lcd.setCursor(18,3);
         lcd.print(min_val);
         lcd.setCursor(18,1);
         lcd.print(max_val);
    break;
  }
  }
  }
     while(!back_but_press) {
     back_but_press = digitalRead(BackButton);
  }
   state = MenuActive;
}

int min_value(int* mass, int mask) {
  if(mass[0] !=mask) {
  int min_val=mass[0];
  for(int i=1; i<=scale_size-1; i++) {
    if(mass[i] !=mask) {
        if(mass[i] < min_val) min_val=mass[i];
    }
    else {
      break;
    }
  }
  return min_val;
  }
  else {
    return mask;
  }
}

int max_value(int* mass, int mask) {
  if(mass[0] !=mask) {
  int max_val=mass[0];
  for(int i=1; i<=scale_size-1; i++) {
    if(mass[i] !=mask) {
        if(mass[i] > max_val) max_val=mass[i];
    }
    else {
      break;
    }
  }
  return max_val;
  }
  else {
    return mask;
  }
}

void Build_coll(int val, int coll) {
  int full = val /8;
  int dec = val % 8;
  for(int i =0; i < full; i++) {
    lcd.setCursor(coll,3-i);
    lcd.write(byte(7));
  }
  if(dec > 0) {
  lcd.setCursor(coll,3-full);
  lcd.write(byte(dec-1));
  }
}

void Print_interr() {
   back_but_press = false;
   while(!back_but_press) {
     back_but_press = digitalRead(BackButton);
  }
   state = MenuActive;
}
  
