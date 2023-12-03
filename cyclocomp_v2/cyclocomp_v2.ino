#include "Ucglib.h"  // Include Ucglib library to drive the display

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"


const char* ssid ="droidd";
const char* password="hitler87"; 
Ucglib_ST7735_18x128x160_HWSPI ucg(2, 5, 4);  // (A0=2, CS=5, RESET=4)
PulseOximeter pox;//SPI--hardware


//----variables
byte bpm;
volatile byte magnet;
unsigned long runtime;
unsigned long prevtime_speed=0;
unsigned long prevtime_pulse=0;
float omega=0;
float wheel_rad_in_m=0;
byte wheel_size=16;
byte age=15;
byte weight=50;
byte height=150;
byte gen=1;
float trip_m=0;
float dist_m=2000;
float calorie_permin=0;
float linear_speed;
int rpm;

 volatile unsigned long int t1=00000000UL;
 volatile unsigned long int t2;
float pulse_duration=50000;


//------button definition
#define right_bt 14
#define ok_bt 15
#define left_bt 12
#define up_bt 33
#define down_bt 0
#define interruptPin 34
//-----------------


void setup() {
  // put your setup code here, to run once:
  ////----initialize buttons------
pinMode(right_bt,INPUT_PULLUP);
pinMode(left_bt,INPUT_PULLUP);
pinMode(up_bt,INPUT_PULLUP);
pinMode(down_bt,INPUT_PULLUP);
pinMode(ok_bt,INPUT_PULLUP);
pinMode(interruptPin, INPUT_PULLDOWN);
// attachInterrupt(digitalPinToInterrupt(interruptPin), duration, RISING);
magnet=1;

Serial.begin(115200);
//---------------------------------

ucg.begin(UCG_FONT_MODE_SOLID);  // It writes a background for the text. This is the recommended option
ucg.setRotate180();  // Put 90, 180 or 270, or comment to leave default

ucg.clearScreen();  // Clear the screen
start_function();

 //----------Start WIFI---------
  WiFi.begin(ssid, password);
  ucg.setFont(ucg_font_5x8_mr);
  ucg.setColor(0, 255, 100, 0);  // Set color (0,R,G,B)
  ucg.setPrintPos(70,75);  // Set position (x,y)
  ucg.print("...");  // Print text or value

  while (WiFi.status() != WL_CONNECTED) {
      ucg.setPrintPos(65,75);  // Set position (x,y)
      ucg.print("...");  // Print text or value
      delay(300);
      ucg.setPrintPos(65,75);  // Set position (x,y)
      ucg.print("___"); 
      delay(200); // Print text or value  
      if(digitalRead(ok_bt)==0){
        // ucg.clearScreen();  // Clear the screen
        break;
      } 
      
   }

//---end wifi

if(WiFi.status()==WL_CONNECTED){
  ucg.setFont(ucg_font_6x10_mr);  //set font type
  ucg.setColor(0, 255, 100, 0);  // Set color (0,R,G,B)
  ucg.setPrintPos(5,95);  // Set position (x,y)
  ucg.print("Connected to:");  // Print text or value
  ucg.setPrintPos(5,110);  // Set position (x,y)
  ucg.print(ssid);  // Print text or value
  delay(2000);
}


delay(2000);
ucg.clearScreen();  // Clear the screen

//pulse sensor--------
if (!pox.begin()) {
        //confirm connection
        //Serial.println("FAILED");
        for(;;);
    } else {
        //Serial.println("SUCCESS");
        //restart...
  }
//---

menu();
ucg.clearScreen();
main_screen();
update_odo((int)dist_m);

runtime=millis();
t2=millis();
Serial.println(t2);
Serial.println(t1);

}
byte run_counter;
byte run_min=0;

float avg_speed_ms=0;
int avg_bpm=0;
bool in_state=LOW;

void loop() {
  

   if(digitalRead(ok_bt)==0){
    delay(300);
    Serial.println("in function");
      menu();
      ucg.clearScreen();
      main_screen();
    }

  if(digitalRead(34)!=in_state){
    in_state=!in_state;
    delay(50);
       magnet++;
      }
  // Serial.printf("loop\n");
   
  if(millis()-prevtime_pulse>2000){
    Serial.print("magnet==");
    Serial.print(magnet);
    pulse_duration=(4000/magnet);
    magnet=1;
    Serial.print("     ----pulse duration=");
    Serial.print(pulse_duration);

 
      pox.update();
      bpm=pox.getHeartRate();
      update_pulse(bpm);
      Serial.print("  -----bpm: ");
      Serial.print(bpm);
      avg_bpm=avg_bpm+bpm;

      rpm=6667/pulse_duration;
      Serial.print("         rpm:  ");
      Serial.print(rpm);
      
      update_rpm(rpm);

      omega=1570/pulse_duration;//rad/s
      Serial.print("        w: ");
      Serial.print(omega);
      linear_speed=omega*wheel_rad_in_m*3.6;
      Serial.print("     Speed");
      Serial.print(linear_speed);
      update_speed((int)linear_speed);
      avg_speed_ms=avg_speed_ms+(omega*wheel_rad_in_m);

      run_counter++;
      Serial.print(" =----  ");
      Serial.println(run_counter);
      if(run_counter>=30){
        Serial.println("inside 1min");
        run_counter =0;
        //update others dist, cal, odo, trip_m
        avg_speed_ms=avg_speed_ms/30;
        avg_bpm=avg_bpm/30;
        trip_m=trip_m+(avg_speed_ms*60);
        update_trip((int)trip_m);
        dist_m=dist_m+(avg_speed_ms*60);
        update_odo((int)dist_m);
        //male
          if (gen==1){
          calorie_permin = calorie_permin+(((13.75*(float)weight)+(5*(float)height)-6.67*(float)age+66)/float(24))*8/*MET*//(float)30;
          }
        //female
          if (gen==0){
          calorie_permin = calorie_permin+(((9.56*(float)weight)+(1.85*(float)height)-4.68*(float)age+655)/float(24))*8/*MET*//(float)30;
          }
          update_cal((int)calorie_permin);
          update_runtime();
          send((int)(avg_speed_ms*3.6), rpm, (int)trip_m, (int)(millis()/3600), (int)dist_m, (int)calorie_permin,bpm);

      }

      prevtime_pulse=millis();

    }

  
}




void menu(){
  delay(200);
  ucg.clearScreen();
  byte feild[5]={1,0,0,0,0};
  byte feild_pointer=1;
  byte temp_wheel=wheel_size;
  byte temp_age=age;
  byte temp_height=height;
  byte temp_weight=weight;
  
  char* temp_gender[2]={" female "," male "};
  char* temp_gen=temp_gender[gen];
  

  ucg.setFont(ucg_font_6x10_mr);
  ucg.setColor(0, 255, 0, 0);  // Set color (0,R,G,B)
  ucg.setPrintPos(120,45);  // Set position (x,y)
  ucg.print("<-");

  //---------disp
    ucg.setFont(ucg_font_6x10_mr);
    ucg.setColor(0, 0, 255, 0);  // Set color (0,R,G,B)
    ucg.setPrintPos(3,15);  // Set position (x,y)
    ucg.print(" ENTER YOUR DETAILS");  // Print text or value
    
    ucg.setFont(ucg_font_6x13_mr);
    ucg.setColor(0, 255, 100, 0);  // Set color (0,R,G,B)
    ucg.setPrintPos(3,45);  // Set position (x,y)
    ucg.print("wheel size(in):<  >");  // Print text or value

    ucg.setPrintPos(99,45);  // Set position (x,y)
    ucg.print(temp_wheel);  // Print text or value

    ucg.setPrintPos(3,60);  // Set position (x,y)
    ucg.print("Age(yrs):<   >");  // Print text or value
    ucg.setPrintPos(62,60);  // Set position (x,y)
    ucg.print(temp_age);  // Print text or value

    ucg.setPrintPos(3,75);  // Set position (x,y)
    ucg.print("weight(Kgs):<   >");  // Print text or value
    ucg.setPrintPos(81,75);  // Set position (x,y)
    ucg.print(temp_weight);  // Print text or value


    ucg.setPrintPos(3,90);  // Set position (x,y)
    ucg.print("Height(cms):<    >");  // Print text or value
    ucg.setPrintPos(82,90);  // Set position (x,y)
    ucg.print(temp_height);  // Print text or value

    ucg.setPrintPos(3,105);  // Set position (x,y)
    ucg.print("Gender:<      >");  // Print text or value
    ucg.setPrintPos(50,105);  // Set position (x,y)
    ucg.print(temp_gen);  // Print text or value
  //-----------------

  while(digitalRead(ok_bt)!=0){

    
    if(digitalRead(up_bt)==0){
      ucg.setColor(0,255,0,0);
      ucg.setPrintPos(118, 45+(feild_pointer-1)*15);
      ucg.print("  ");
      if(feild_pointer==1){feild_pointer=5;}else{feild_pointer--;}
      ucg.setPrintPos(118, 45+(feild_pointer-1)*15);
      ucg.print("<-");
      delay(500);
    }
    if(digitalRead(down_bt)==0){
      ucg.setColor(0,255,0,0);
      ucg.setPrintPos(118, 45+(feild_pointer-1)*15);
      ucg.print("  ");
      if(feild_pointer==5){feild_pointer=1;}else{feild_pointer++;}
      ucg.setPrintPos(118, 45+(feild_pointer-1)*15);
      ucg.print("<-");
      delay(500);
    }

    if(digitalRead(left_bt)==0){
      switch(feild_pointer){
        case 1:
        //change size
        ucg.setColor(0,255,0,0);
        ucg.setPrintPos(99, 45+(feild_pointer-1)*15);
        ucg.print(--temp_wheel);
        
        break;

        case 2:
        //change age
        ucg.setPrintPos(62,60);  // Set position (x,y)
        ucg.print(--temp_age);  // Print text or value

        break;

        case 3:
        //change weight
        ucg.setPrintPos(81,75);  // Set position (x,y)
        ucg.print(--temp_weight);  // Print text or value
        break;

        case 4:
        //change height
        temp_height-=5;
        ucg.setPrintPos(82,90);  // Set position (x,y)
        ucg.print(temp_height);  // Print text or value
        
        break;

        case 5:
        //change gender
        if(temp_gen==" male ") temp_gen=" female ";
        else temp_gen=" male ";
        ucg.setPrintPos(50,105);  // Set position (x,y)
        ucg.print(temp_gen); 
        delay(200);
        
        break;
      }
      delay(250);


    }
    if(digitalRead(right_bt)==0){
          switch(feild_pointer){
        case 1:
        //change size
        ucg.setColor(0,255,0,0);
        ucg.setPrintPos(99, 45+(feild_pointer-1)*15);
        ucg.print(++temp_wheel);
        break;

        case 2:
        //change age
        ucg.setPrintPos(62,60);  // Set position (x,y)
        ucg.print(++temp_age);  // Print text or value
        break;
        
        case 3:
        //change weight
        ucg.setPrintPos(81,75);  // Set position (x,y)
        ucg.print(++temp_weight);  // Print text or value
        
        break;

        case 4:
        //change height
        temp_height+=5;
        ucg.setPrintPos(82,90);  // Set position (x,y)
        ucg.print(temp_height);  // Print text or value
        break;

        case 5:
        //change gender
        if(temp_gen==" male ") temp_gen=" female ";
        else temp_gen=" male ";
        ucg.setPrintPos(50,105);  // Set position (x,y)
        ucg.print(temp_gen); 
        delay(100);
        break;

    }
      delay(100);
  

  }
  delay(100);

  }
  age=temp_age;
  weight=temp_weight;
  wheel_size=temp_wheel;
  wheel_rad_in_m=(float)wheel_size*0.0127;
  height=temp_height;
  if(temp_gen==" female ") gen=0;
  else gen=1;

  
}

void start_function(){
  
  // Write to the display the text "":
  ucg.setFont(ucg_font_profont29_mr);
  ucg.setColor(0, 255, 0, 100);  // Set color (0,R,G,B)
  ucg.setColor(1, 0, 0, 0);  // Set color of text background (1,R,G,B)
  ucg.setPrintPos(29,24);  // Set position (x,y)
  ucg.print("CYCLO");  // Print text or value

  ucg.setFont(ucg_font_inb16_mr);
  ucg.setColor(0, 255, 100, 0);  // Set color (0,R,G,B)
  ucg.setPrintPos(10,50);  // Set position (x,y)
  ucg.print("COMPUTER");  // Print text or value

  ucg.setFont(ucg_font_6x10_mr);
  ucg.setColor(0, 255, 100, 0);  // Set color (0,R,G,B)
  ucg.setPrintPos(3,65);  // Set position (x,y)
  ucg.print("press ok to skip");  // Print text or value

  ucg.setFont(ucg_font_5x8_mr);
  ucg.setColor(0, 255, 100, 0);  // Set color (0,R,G,B)
  ucg.setPrintPos(3,80);  // Set position (x,y)
  ucg.print("conecting");  // Print text or value



}

void main_screen(){
  ucg.clearScreen();

  ucg.setFont(ucg_font_inb42_mr);  //set font type
  ucg.setColor(0, 255, 3, 29);  // Set color (0,R,G,B)
  ucg.setPrintPos(4,50);  // Set position (x,y)
  ucg.print(00);  // Print text or value

  ucg.setFont(ucg_font_profont12_mr);  //set font type
  ucg.setColor(0, 255, 3, 29);  // Set color (0,R,G,B)
  ucg.setPrintPos(75,50);  // Set position (x,y)
  ucg.print("Km/h"); 

 ucg.setColor(0, 0, 255, 20);  // Set color (0,R,G,B)
  ucg.setPrintPos(4,68);  // Set position (x,y)
  ucg.print("Rpm:0 rpm");  
  

  ucg.setPrintPos(4,83);  // Set position (x,y)
  ucg.print("Trip:0 meter");
  

  ucg.setPrintPos(4,98);  // Set position (x,y)
  ucg.print("Time:0min");
  

  ucg.setPrintPos(4,113);  // Set position (x,y)
  ucg.print("ODO:0 meter");
 

  ucg.setPrintPos(4,128);  // Set position (x,y)
  ucg.print("Cal:0 cal");
  
  ucg.setPrintPos(4,143);  // Set position (x,y)
  ucg.print("Pulse:0bpm");
  

}


void update_speed(int arg_speed){
  //----------on update speed
  ucg.setFont(ucg_font_inb42_mr);  //set font type
  ucg.setColor(0, 200, 200, 0);  // Set color (0,R,G,B)
  ucg.setPrintPos(4,47);  // Set position (x,y)
  if(arg_speed<10){
    ucg.print("0"+String(arg_speed));
  }else{
  ucg.print(arg_speed);  // Print text or value
  }
  //------------
}

void update_rpm(int arg_rpm){
  if (arg_rpm==1) arg_rpm=0;
  //update rpm---
  ucg.setFont(ucg_font_profont12_mr);  //set font type
  ucg.setColor(0, 50, 10, 250);  // Set color (0,R,G,B)
  ucg.setPrintPos(28, 68);
  ucg.print(arg_rpm);
  ucg.setColor(0, 255, 3, 29);  // Set color (0,R,G,B)
  ucg.print(" RPM");
  ////---

}

void update_trip(int arg_trip){
  ucg.setFont(ucg_font_profont12_mr);  //set font type
  ucg.setColor(0, 50, 10, 250);  // Set color (0,R,G,B)
  //------update trip
  ucg.setPrintPos(34,83);  // Set position (x,y)
  ucg.print(arg_trip);
  ucg.setColor(0, 255, 3, 29);  // Set color (0,R,G,B)
  ucg.print(" Meters");
  ///-------
}

void update_runtime(){
  ucg.setFont(ucg_font_profont12_mr);  //set font type
 ucg.setColor(0, 50, 10, 250);  // Set color (0,R,G,B)
  //-----update runtime
   ucg.setPrintPos(34,98);  // Set position (x,y)
  ucg.print((int)(millis()/(float)60000));
  ucg.setColor(0, 255, 3, 29);  // Set color (0,R,G,B)
  ucg.print(" Mins");
}

void update_odo(int arg_dist){
  //-----update odo
  ucg.setFont(ucg_font_profont12_mr);  //set font type
  ucg.setColor(0, 50, 10, 250);  // Set color (0,R,G,B)
  ucg.setPrintPos(28,113);  // Set position (x,y)
  ucg.print(arg_dist);
  ucg.setColor(0, 255, 3, 29);  // Set color (0,R,G,B)
  ucg.print(" Meters"); 
}

void update_cal(int arg_cal){
  //-------update cal;
  ucg.setFont(ucg_font_profont12_mr);  //set font type
  ucg.setColor(0, 50, 10, 250);  // Set color (0,R,G,B)
  ucg.setPrintPos(28,128);  // Set position (x,y)
  ucg.print(arg_cal);
 ucg.setColor(0, 255, 3, 29);  // Set color (0,R,G,B) 
  ucg.print(" Cal");   

}

void update_pulse(byte arg_bpm){
  //update pulse-------
  ucg.setFont(ucg_font_profont12_mr);  //set font type
  ucg.setColor(0, 50, 10, 250);  // Set color (0,R,G,B)
  ucg.setPrintPos(40,143);  // Set position (x,y)
  ucg.print(arg_bpm);
  ucg.setColor(0, 255, 3, 29);  // Set color (0,R,G,B)
  ucg.print(" BPM");  

}

void send(int arg_speed, int arg_rpm, int arg_trip, int arg_time, int arg_dist, int arg_cal,byte arg_bpm){
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
  
    HTTPClient http;
      String speed=String(arg_speed);
      String rpm=String(arg_rpm);
      String cadence=String();
      String trip_duration=String(arg_time);
      String trip_dist=String(arg_dist);
      String calorie_burnt=String(arg_cal);
      String pulserate=String(arg_bpm);

                // https://script.google.com/macros/s/AKfycby8jYkOxDbYM5Wl8GoU07OxZctiZrhXJf48_1ZhzPbdVftU8_E-BbOpjrVXxJGbAcGtWA/exec
    http.begin("https://script.google.com/macros/s/AKfycby8jYkOxDbYM5Wl8GoU07OxZctiZrhXJf48_1ZhzPbdVftU8_E-BbOpjrVXxJGbAcGtWA/exec?action=create&speed="+speed+"&rpm="+rpm+"&trip_duration="+trip_duration+"&trip_distance="+trip_dist+"&calorie_burnt="+calorie_burnt+"&pulserate="+pulserate); //Specify the URL
    int httpCode = http.GET();                                        //Make the request
  
    if (httpCode > 0) { //Check for the returning code
  
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
      }
  
    else {
      Serial.println("Error on HTTP request");
    }
  
    http.end(); //Free the resources
  }
  
}





