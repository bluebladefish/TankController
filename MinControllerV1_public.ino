// esp8266 fishtank controller
// connectivity to main Tank controller website
// v0.2 11/14/2018 - wireless networking and RTC
// v0.4 11/24/2018 - basic web communication and data parsing, time sync
// v1.0 12/02/2018 - added DS18b20 support, relay controll, heater controll, timed based process system, updated/modified web data processes, x-mit, wifi, and sync indicator
// http post support, interrupt management (lights, timing and clear), web responce/loggining,
// needed - light controll, interrupt management (lights)
//known issues - web connections take 4 seconds to end (each).

// dependancy libraries
#include <ESP8266WiFi.h>
#include "DS1307new.h"  // extension to new DS1307 Arduino library that includes weekday alarms
#include <DallasTemperature.h> //1-wire library  - used for DS18b20 sensors
#include <OneWire.h>        // used for DS18b20 sensors

#define ONE_WIRE_BUS 2 // One with temp sensors on GPIO 2 (label D4)
#define TEMPERATURE_PRECISION 9 // precision for thermomiters
OneWire oneWire(ONE_WIRE_BUS); // set up onewire bus
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

DeviceAddress Thermometer = { 0x28, 0x5A, 0x36, 0x80, 0x03, 0x00, 0x00, 0xDA }; // add address for your DS18b20 device
// arrays to hold device addresses for multiple ds18b20 thermometers
//DeviceAddress Thermometer1 = { 0x28, 0x8F, 0x32, 0xBF, 0x03, 0x00, 0x00, 0x38 },
//Thermometer2   = { 0x28, 0x0C, 0x83, 0xB7, 0x03, 0x00, 0x00, 0x15 },
//Thermometer3 = { 0x28, 0x3E, 0x78, 0xB5, 0x03, 0x00, 0x00, 0x47 };

// WIFI connection info
const char* ssid     = "yourSSID"; // update with your info
const char* password = "yourPassword"; // update with your info

//tank controller webserver connection info - change for each device
const char* host = "192.168.0.1"; // update with your info
const int httpPort = 81; // update with your info
const char* clientId   = "ESPcont1";
const char* webdatapg1 = "data.txt";
const char* websettings = "settings.php";
const char* weboutput = "respond2.php";
const char* varnameprefix = "esp1_";


//PIN definitions:
// D1 (GPIO5) = SCL, D2(GPIO6) = SCA for RTC
const int Heatpin = 14; // GPIO14 (label D5)digital pin for relay 1 controll - 110v AC heaters on/off
const int Lightpin = 12; // GPIO12 (label D6)digital pin for relay 2 controll - 110v AC Lights on/off
const int LED1 = 16; // GPIO16 (label D0)digital pin for X-mit LED indicator
const int LED2 = 15; // GPIO15 (label D8)digital pin for Wifi LED indicator
const int LED3 = 13; // GPIO13 (label D7)digital pin for Sync LED indicator

// other constants
const bool pwrOn = false; // closed = ON/active for low activation relay. used with ActivateRelay function to make code easy to read
const bool pwrOff = true; // open = OFF/inactive by sending high signal. used with ActivateRelay function to make code easy to read

// global variables
float LastTemperature [3] = {75.00,75.00,75.00} ; // stores last temp reading from Thermometer1[0], Thermometer2[1], Thermometer3[2]
int value = 0;
int CurrentTime [6] = {2017,11,11,00,16,50}; //  current year/month/day/hour/minute/second
long CurrentSecSince2k = 0; //second counter
String CurrentTimestamp = "first_sec_Arduino_init"; //varible for text based timestamp
long TimeSecCount [2] = {0,0}; // track last init times (in sec) for timekeepng [0], timekeeping 5 seconds [1]
int TimeMinCount [4] = {0,0,0,0}; // track last init times (in min) for timekeepng [0], 5 minute intervals [1], variable fade [2], 30 minutes after the hour
int TimeHourCount = 0; // track last init times (in hours) for timekeepng [0],
int TimeDayCount = 0; // track last init times (in days) for timekeepng [0]

//-------------------Variables to adjust (pull from web/r-pi) --------------------
int LcdButton[4]{0,0,0,0}; //hold timer values for interrupts - LcdButton[0] is light toggle, LcdButton[3] for heater, LcdButton[1] TBD
int tempgoal = 78;// temperature goal
int tempdelta = 1; // temp delta, before case and canopy fans or heaters come on
int LtTimeON [2] ={1,9}; // minute, hour to turn light on (must be before LtTimeOFF in 24 hour day)
int LtTimeOFF [2] ={1,21}; // minute, hour to turn light off 


// Main setup -------------------------------------------------
void setup() {
  //init outputs:
  digitalWrite(Heatpin, HIGH);
  digitalWrite(Lightpin, HIGH);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
  pinMode(Heatpin, OUTPUT);
  pinMode(Lightpin, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  //start //Serial for debugging
  //Serial.begin(115200);
  delay(10);
  sensors.begin(); //init thermomenters
  sensors.setResolution(Thermometer, TEMPERATURE_PRECISION);
  // We start by connecting to a WiFi network
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  ////Serial.println("WiFi connected");
  ////Serial.println("IP address: ");
  ////Serial.println(WiFi.localIP());
  RTC.ctrl = 0x00;   // 0x00=disable SQW pin, 0x10=1Hz, 0x11=4096Hz, 0x12=8192Hz, 0x13=32768Hz
  RTC.setCTRL();
  InitTimers ();
  pollwebservertime();
  delay(200);
  pollwebsettings();
  delay(200);
  getMyTime();
}


//Main loop
void loop() {
  delay(100);
  TimedScheduler();
}

//-------------------------------------
// web services

// check wifi connect status - connect if needed - set indicator light based on status.
void connecttowifi (){
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED2, LOW); // Wi-fi indicator
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    delay(500); 
  } else {
    digitalWrite(LED2, HIGH); // Wi-fi indicator
  }
}


//gather data from webserver (page at /tank/devices/(clientId)/(websettings) )
void pollwebsettings() {
  digitalWrite(LED1, LOW); //x-mit indicator
  ////Serial.print("connecting to ");
  ////Serial.println(host);
  // Use WiFiClient class to create TCP connections
  int settingsync = 0;
  WiFiClient client;
  if (!client.connect(host, httpPort)) {
    //Serial.println("connection failed");
    return;
  }
  // We now create a URI for the request
  String url = "/tank/devices/";
  url += clientId;
  url += "/";
  url += websettings;
  //Serial.print("Requesting data URL: ");
  //Serial.println(url);
  digitalWrite(LED1, HIGH); //x-mit indicator
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      //Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  // Read all the lines of the reply (skipping header) from server and print them to //Serial
  int linecount = 1;
  int index1=0; // , locations
  int index2=0; // , locations
  int index3=0; // , locations
  String varname; //data String
  String val1=""; //data String
  String val2=""; //data String
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (linecount >= 9){
      index1 = line.indexOf(':');  //finds location of first :
      varname = line.substring(0, index1);   //captures first data String
      index2 = line.indexOf(':', index1+1 );   //finds location of second :
      val1 = line.substring(index1+1, index2);   //captures second data String
      index3 = line.indexOf(':', index2+1 );   //finds location of third :
      val2 = line.substring(index2+1, index3);   //captures third data String 
      //(ignoring 4th and 5th)
      // set variables:
      if (varname == "\nLtimeOFF") {
        LtTimeOFF[0]=val1.toInt();//minute
        LtTimeOFF[1]=val2.toInt();//hour
        settingsync = settingsync + 1;
        //Serial.print("LtTimeOFF=");
        //Serial.print(LtTimeOFF[0]);
        //Serial.print(", ");
        //Serial.println(LtTimeOFF[1]);
      } else if (varname == "\nLtimeON"){
        LtTimeON[0]=val1.toInt();//minute
        LtTimeON[1]=val2.toInt();//hour
        settingsync = settingsync + 1;
        //Serial.print("LtTimeON=");
        //Serial.print(LtTimeON[0]);
        //Serial.print(", ");
        //Serial.println(LtTimeON[1]);
      } else if (varname == "\ntempdelta"){
        tempdelta=val1.toInt();
        settingsync = settingsync + 1;
        //Serial.print("tempdelta=");
        //Serial.println(tempdelta);
      } else if (varname == "\ntempgoal"){
        tempgoal=val1.toInt();
        settingsync = settingsync + 1;
        //Serial.print("tempgoal=");
        //Serial.println(tempgoal);
      }
    }
    linecount ++;
  }
  if (settingsync == 4){
    digitalWrite(LED3, HIGH); // sync indicator
    //Serial.println("all settings in sync");
  } else {
    digitalWrite(LED3, LOW); // sync indicator
    //Serial.println("sync error");
  }
  client.stop();
  //Serial.println("connection closed");
  digitalWrite(LED1, LOW); //x-mit indicator
}

//gather stored settings from webserver (page at /tank/devices/(clientId)/(webdatapg1) )
void pollwebserver() {
  digitalWrite(LED1, LOW); //x-mit indicator
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, httpPort)) {
    //Serial.println("connection failed");
    return;
  }
  // We now create a URI for the request
  String url = "/tank/devices/";
  url += clientId;
  url += "/";
  url += webdatapg1;
  //Serial.print("Requesting data URL: ");
  //Serial.println(url);
  digitalWrite(LED1, HIGH); //x-mit indicator
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      //Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  // Read all the lines of the reply (skipping header) from server and print them to //Serial
  int linecount = 1;
  int foundaction = 0;
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (linecount >= 11){
      if (line == "\nlight_toggle"){
        foundaction = 1;
      } else if (line == "\nHeater_toggle"){
        foundaction = 2;
      } else if (line == "\nTBD"){
        // act on setting and clear file
        //Serial.println("TBD interrupt detected");
        foundaction = 3;
      }
      ////Serial.print("raw data:");
      ////Serial.println(line);
    }
    linecount ++;
  }
  client.stop();
  //Serial.println("connection closed");
  digitalWrite(LED1, LOW); //x-mit indicator
  if (foundaction == 1){ //light interrupt
    if (LcdButton[0] >= 1){ //if interrupt already active cancil
      LcdButton[0] = 0;
      //Serial.println("Light interrupt cancil detected");
      webPostrespond("clear" , "clear", 0); // clear data from file if anything was read.
      // return to standard light state 
      lightShedule();// turn lights on or off based on schedule/time
      webPostrespond("light_interrupt", "end", 1);
    } else { //else new interrupt, process 
      LcdButton[0] = 60;
      //Serial.println("Light interrupt toggle detected");
      webPostrespond("clear" , "clear", 0); // clear data from file if anything was read. 
      if (digitalRead(Lightpin) == HIGH){ //if lights are off (HIGH = off),
        ActivateRelay(Lightpin, pwrOn);// turn on
        //Serial.println("Lights On");
        webPostrespond("lights", "ON", 1);
      } else { // else lights are ON 
        ActivateRelay(Lightpin, pwrOff); ;//turn off
        //Serial.println("Lights OFF");
        webPostrespond("lights", "OFF", 1);
      }
      webPostrespond("light_interrupt", "start", 1);
    }
  } else if (foundaction == 2){ //heater interrupt
    if (LcdButton[3] >= 1){ //if interrupt already active cancil
      LcdButton[3] = 0;
      //Serial.println("heater interrupt end detected");
      //Serial.println("Heater Enabled");
      webPostrespond("clear" , "clear", 0); // clear data from file if anything was read. 
      webPostrespond("heater_interrupt", "end", 1);
    } else { //else new interrupt, process 
      LcdButton[3] = 60;
      ActivateRelay(Heatpin,  pwrOff);
      //Serial.println("heater interrupt start detected");
      //Serial.println("Heater OFF");
      webPostrespond("clear" , "clear", 0); // clear data from file if anything was read. 
      webPostrespond("heater", "OFF", 1);
      webPostrespond("heater_interrupt", "start", 1);
    }
  } else if (foundaction == 3){ //TBD interrupt
       if (LcdButton[1] >= 1){ //if interrupt already active cancil
      LcdButton[1] = 0;
      //Serial.println("TBD interrupt end detected");
      webPostrespond("clear" , "clear", 0); // clear data from file if anything was read. 
      //webPostrespond("heater_interrupt", "end", 1);
    } else { //else new interrupt, process 
      LcdButton[1] = 60;
      //Serial.println("TBD interrupt start detected");
      webPostrespond("clear" , "clear", 0); // clear data from file if anything was read. 
      //webPostrespond("heater_interrupt", "start", 1);
    }
  }
}



// get time from webserver (page at /tank/devices/time.php)
void pollwebservertime() {
  digitalWrite(LED1, LOW); //x-mit indicator
  ////Serial.print("connecting to ");
  ////Serial.println(host);
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, httpPort)) {
    //Serial.println("connection failed");
    return;
  }
  // We now create a URI for the request
  String url = "/tank/devices/time.php";
  //Serial.print("Requesting Time URL: ");
  //Serial.println(url);
  // This will send the request to the server
  digitalWrite(LED1, HIGH); //x-mit indicator
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      //Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  // Read all the lines of the reply from server and print them to //Serial
  int linecount = 1;
  bool settimenow = false;
  int NewTime [6] = {2017,11,11,00,16,50}; //  current year/month/day/hour/minute/second
  while (client.available()) {
    String line = client.readStringUntil('\r');
    switch (linecount) {
      case 8:
        if (line == "\nstart"){
           settimenow = true;
           //Serial.println(" gathering data to set time");
        }
      break;
      case 9:
        NewTime [0] = line.toInt();
      break;
      case 10:
        NewTime [1] = line.toInt();
      break;
      case 11:
        NewTime [2] = line.toInt();
      break;
      case 12:
        NewTime [3] = line.toInt();
      break;
      case 13:
        NewTime [4] = line.toInt();
      break;
      case 14:
        NewTime [5] = line.toInt();
      break;
      case 15:
        if ((line == "\nend") && (settimenow)){
           //settimenow = true;
           SetTime(NewTime[0], NewTime[1], NewTime[2], NewTime[3], NewTime[4], NewTime[5]);
           //Serial.println("--setting time--");
        }
    }
    ////Serial.print(line);
    linecount ++;
  }
  client.stop();
  //Serial.println("connection closed");
  digitalWrite(LED1, LOW); //x-mit indicator
}

// respond back to website with data
void webPostrespond(String Mypostvarname, String MyPostData, bool useprefix) {
  digitalWrite(LED1, LOW); //x-mit indicator
  String PostData = ("varname=");
  if (useprefix){
    PostData = ( PostData + varnameprefix + Mypostvarname + "&val1=" + MyPostData + "&undefined=\r\n");
  } else {
    PostData = ( PostData + Mypostvarname + "&val1=" + MyPostData + "&undefined=\r\n");
  }
  WiFiClient client;
  if (!client.connect(host, httpPort)) {
    //Serial.println("connection failed");
    return;
  }
  // We now create a URI for the request
  String url = "/tank/devices/";
  url += clientId;
  url += "/";
  url += weboutput;
  //Serial.print("Posting (");
  //Serial.print(" to URL: ");
  //Serial.println(url);
  digitalWrite(LED1, HIGH); //x-mit indicator
  // This will send the request to the server
  client.print("POST " + url + " HTTP/1.1\r\nHost: " + host + ":" + httpPort + "\r\n");
  //Serial.print("POST " + url + " HTTP/1.1\r\nHost: " + host + ":" + httpPort + "\r\n");
  client.print("cache-control: no-cache\r\n");
  //Serial.print("cache-control: no-cache\r\n");
  client.print("Content-Type: application/x-www-form-urlencoded\r\n");
  //Serial.print("Content-Type: application/x-www-form-urlencoded\r\n");
  client.print("Content-Length: ");
  //Serial.print("Content-Length: ");
  client.println(PostData.length());
  //Serial.print(PostData.length());
  client.println();
  //Serial.println(" ");
  client.println(PostData);
  //Serial.println(PostData);
  unsigned long timeout = millis();
  while(!client.available()){
    if (millis() - timeout > 8000) {
      //Serial.println(">>> Client Timeout !");
      client.stop();
      digitalWrite(LED1, LOW); //x-mit indicator
      return;
    }
  }
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.println(line);     
  }
  client.stop();
  //Serial.println("post connection closed");
  digitalWrite(LED1, LOW); //x-mit indicator
}


//--------------------------------------------------
// RTC time services
// sets time in RTC
void SetTime (int y1,int m1,int d1, int h1,int min1,int s1) {
  RTC.stopClock();
  RTC.fillByYMD(y1,m1,d1); //put in current year/month/day
  RTC.fillByHMS(h1,min1,s1);//put in hour/minute/second of upload - military time
  RTC.setTime();
  RTC.startClock();
  getMyTime ();
  //Serial.println("clock set");
}

//get the curent time, for string output to display or other.
void getMyTime () {
  RTC.getTime();
  CurrentTime [3] =(RTC.hour);
  CurrentTime [4] =(RTC.minute);
  CurrentTime [5] =(RTC.second);
  CurrentTime [2] =(RTC.day);
  CurrentTime [1] =(RTC.month);
  CurrentTime [0] =(RTC.year);  
  CurrentSecSince2k = RTC.time2000;
  CurrentTimestamp = (String(CurrentTime [0]) + "-" + TwoDigStr(CurrentTime [1]) + "-" + TwoDigStr(CurrentTime [2]) + " " + TwoDigStr(CurrentTime [3]) + ":" + TwoDigStr(CurrentTime [4]) + ":" + TwoDigStr(CurrentTime [5]) );
}

//convert integer to 2 digit string (9 to 09, etc)
String TwoDigStr (int intsent)
{
  String workingvalue = "00";
  if (intsent < 10) { // if one digit
    workingvalue = ("0" + String(intsent));
  } else {
    workingvalue = String(intsent);
  }
  return workingvalue;
}

// --------------------- temperature monitoring --------------

// function to retrieve the temperature in F for a onewire temp devices
float getTemperature(DeviceAddress deviceAddress){
  float tempC = sensors.getTempC(deviceAddress);
  return (DallasTemperature::toFahrenheit(tempC));
}

// output the temperature data for logging
void ReportTemp(void) { 
  sensors.requestTemperatures(); //calls up the 1 wire sensors
  LastTemperature[0] = getTemperature(Thermometer);
  webPostrespond("temp", String(LastTemperature[0]), 1);
  //Serial.print(" temp= ");
  //Serial.println(LastTemperature[0]);
}

// trigger (on or off) heater based on temp 
void TempTriggers(){
  if (LcdButton[3] == 0) { //if no heater interrupt is active
    // reef heaters
    if (digitalRead(Heatpin) == LOW ){   // check if reef heater is on (low=on)
      if (LastTemperature[0] >= tempgoal ) { // if on, check if temp is OK now (if so, turn off and log event).
        ActivateRelay(Heatpin,  pwrOff);
        webPostrespond("heater", "OFF", 1);
        //Serial.println("Heater OFF");
      }    
    } else {  // else, check if temp is too low (if so, turn on and log event).
      if ( LastTemperature[0] <= (tempgoal - tempdelta) ){ 
        ActivateRelay(Heatpin,  pwrOn);
        webPostrespond("heater", "ON", 1);
        //Serial.println("Heater ON");
      }    
    }
  }
}

// function to activate relays when needed
// pwrOn = false; // closed = ON/active for LOW activation relay. used with ActivateRelay function to make code easy to read
// pwrOff = true; // open = OFF/inactive by sending HIGH signal. used with ActivateRelay function to make code easy to read
void ActivateRelay(int pinnumber, bool relaystate) {
    if (relaystate){ 
    digitalWrite(pinnumber, HIGH); // pwrOff 
    } else { 
    digitalWrite(pinnumber, LOW); // pwrOn 
    }
}

// ---------------  Time scheduling ------------------

// compare currenttime to sample, return true if diff is more than delta
boolean CompareTime( int myUnits, long mySample, int myDelta) {// myUinis: 1:seconds, 2:minutes, 3:hours, 4:days
  long myTimeTempDelta = 0;
  switch(myUnits) {
  case 1: { // seconds
    if ((myDelta > 0) && (mySample == CurrentSecSince2k)) { // if no seconds have passed exit
      return false;
      break;
    }
    myTimeTempDelta = CurrentSecSince2k - mySample;
    if ((myTimeTempDelta-myDelta) >= 0) {
      return true;
      break;
    } else {
      return false;
      break;
    }
  }
  case 2: { // minutes
    if ((myDelta > 0) && (mySample == CurrentTime[4])) { // if no minutes have passed exit
      return false;
      break;
    }    
    if ((mySample + myDelta) > 59) {
      myTimeTempDelta = (CurrentTime[4] + 60) - mySample;
    } else {
      myTimeTempDelta = CurrentTime[4] - mySample;
    }
    if (( myTimeTempDelta-myDelta ) >= 0) {
      return true;
      break;
    } else {
      return false;
      break;
    }
  }
  case 3: { // hours
    if ((myDelta > 0) && (mySample == CurrentTime[3])) { // if no hours have passed exit
      return false;
      break;
    }
    if ((mySample + myDelta) > 23) {
      myTimeTempDelta = (CurrentTime[3] + 24) - mySample;
    } else {
      myTimeTempDelta = CurrentTime[3] - mySample;
    }
    if (( myTimeTempDelta-myDelta ) >= 0) {
      return true;
      break;
    } else {
      return false;
      break;
    }
  }
  case 4: { // Days
    if ((myDelta > 0) && (mySample == CurrentTime[2])) { // if no days have passed exit
      return false;
      break;
    }
    if ( (CurrentTime[1] -1) == 2) { // last month was feb - 29 days
      if ((mySample + myDelta) > 28) {
        myTimeTempDelta = ( CurrentTime[2] + 29) - mySample;
       } else {
        myTimeTempDelta = CurrentTime[2] - mySample;
       }
    } else if ( (CurrentTime[1] -1) == 4 || (CurrentTime[1] -1) == 6 || (CurrentTime[1] -1) == 9 || (CurrentTime[1] -1) == 11) { // last month was april,june,sept, or nov - 30 days
      if ((mySample + myDelta) > 29) {
        myTimeTempDelta = ( CurrentTime[2] + 30) - mySample;
      } else {
        myTimeTempDelta = CurrentTime[2] - mySample;
      }
    } else { // 31 days
      if ((mySample + myDelta) > 30) {
        myTimeTempDelta = ( CurrentTime[2] + 31) - mySample;
      } else {
        myTimeTempDelta = CurrentTime[2] - mySample;
      }
    }
    if (( myTimeTempDelta-myDelta ) >= 0) {
      return true;
      break;
    } else {
      return false;
      break;
    }
  }
  default: 
    return false;
    break;
  }
}

// init the timer variables
void InitTimers (){
  getMyTime ();
  TimeSecCount[0] = CurrentSecSince2k;
  TimeMinCount[0] = CurrentTime[4];
  TimeMinCount[1] = CurrentTime[4];
  TimeMinCount[2] = CurrentTime[4];
  TimeMinCount[3] = (CurrentTime[4] + 29);
  TimeHourCount = CurrentTime[3];
  TimeDayCount = CurrentTime[2];
}
//-------------------------------------------------------------------
// light controll

//convert hour and minutes time into minutes, return minutes
int ConverttoMinutes (int myHr, int myMin ) { 
  int MyTotalMins=(60 * myHr) + myMin;
  return MyTotalMins;
}

// turn lights on or off based on scheduled times, and in case of restart/power intterupt
void lightShedule (){
  int minutestoday = ConverttoMinutes(CurrentTime[3], CurrentTime[4]); //how many minutes into the day we are.
  // fish tank lights 
  if ( LcdButton[0] == 0){ //if no "all lights" intterupts are active
    int plantofftimemins = (ConverttoMinutes(LtTimeOFF[1], LtTimeOFF[0]));
    int plantontimemins = (ConverttoMinutes(LtTimeON[1], LtTimeON[0]));
    if ( (CurrentTime[4] == LtTimeON[0]) && (CurrentTime[3] == LtTimeON[1]) ){//turn light on if the time matches LtTimeON
      ActivateRelay(Lightpin, pwrOn); //turn on
      //Serial.println("Lights On");
      webPostrespond("lights", "ON", 1);
    } else if ((CurrentTime[4] == LtTimeOFF[0]) && (CurrentTime[3] == LtTimeOFF[1])){//turn light off if time matches LtTimeOFF
      ActivateRelay(Lightpin, pwrOff); //turn off
      //Serial.println("Lights OFF");
      webPostrespond("lights", "OFF", 1);
    } else {
      if (plantontimemins < plantofftimemins){ //if on in morning, off at night
        if ((minutestoday  > plantontimemins) && (minutestoday  < plantofftimemins)){// is it between start time and finish time?
          if (digitalRead(Lightpin) == HIGH){ //if lights are off (HIGH = off),
            ActivateRelay(Lightpin, pwrOn);// turn on
            //Serial.println("Lights On");
            webPostrespond("lights", "ON", 1);
          }
        } else { // not in ON window
          if (digitalRead(Lightpin) == LOW){ //if lights are ON (HIGH = off),
            ActivateRelay(Lightpin, pwrOff); ;//turn off
            //Serial.println("Lights OFF");
            webPostrespond("lights", "OFF", 1);
          }
        }
      } else { //if on at night, off in morning (on before midnight, off the next day)
        if ((minutestoday  > plantontimemins) || (minutestoday  < plantofftimemins)){// is it past start time or before finish time?
          if (digitalRead(Lightpin) == HIGH){ //if lights are off (HIGH = off),
            ActivateRelay(Lightpin, pwrOn);// turn on
            //Serial.println("Lights On");
            webPostrespond("lights", "ON", 1);
          }
        } else { // not in ON window
          if (digitalRead(Lightpin) == LOW){ //if lights are ON (HIGH = off),
            ActivateRelay(Lightpin, pwrOff); ;//turn off
            //Serial.println("Lights OFF");
            webPostrespond("lights", "OFF", 1);
          }
        }
      }
    }
  }
}



//-----------------------------------------------------------------
// Primary timing routine - do things on a schedule
void TimedScheduler () {
  getMyTime (); //get current time
  if (CompareTime(1,TimeSecCount[0], 1)) { //every second check ALL timed routines
    // command
    //LCDButtonTimer(); //remove after debug
    TimeSecCount[0] = CurrentSecSince2k; //place second based commands above here
    //--------------------------------------
    if (CompareTime(1,TimeSecCount[1], 15)) { //every 15 seconds
      // command
      connecttowifi();// if not already connected, connect to wifi - set indicator LED2 based on connection status
      pollwebserver();// check webserver file for interrupts
      TimeSecCount[1] = CurrentSecSince2k; //place second based commands above here
      //--------------------------------------
      if (CompareTime(2,TimeMinCount[0], 1)) { //every Minute check timed routines
        // command
        lightShedule();// turn lights on or off based on schedule/time
        LCDButtonTimer(); // 60 minute timer for interrupts
        pollwebsettings(); // update settings from webserver
        webPostrespond("time" , CurrentTimestamp, 0); //report back current controller time once per minute
        TimeMinCount[0] = CurrentTime[4]; //place on the minute based commands above here
        //------------------------------------------
        if (CompareTime(2,TimeMinCount[1], 5)) { //every 5 Minutes check timed routines
          // command
          TempTriggers(); //- (5 minutes) enable/disable fans or heaters
          ReportTemp(); // every 5 minute log and record temperature data
          TimeMinCount[1] = CurrentTime[4]; //place 5 minute based commands above here
          //------------------------------------------
          if (CompareTime(3,TimeHourCount, 1)) { //every Hour check timed routines
            // command
            pollwebservertime(); // update clock from PI
            TimeHourCount = CurrentTime[3]; //place on the Hour based commands above here
            //------------------------------------------
            if (CompareTime(4,TimeDayCount, 1)) { //every Day check timed routines
              // command
              if (CurrentTime[0] < 2017){ // update clock daily - main update will come from Pi, but if off by more than a year due to reset or error, request update
                TimeDayCount = CurrentTime[2]; //place on the Day based commands above here
                //------------------------------------------
              }
            }
          }  
        }
      }
    }
  }
}

// 60 minute button timer for LCD actions, run once per minute
void LCDButtonTimer(){
  int MyButton = 0;
  while(MyButton < 4){//loop through the 4 buttons
    if (LcdButton[MyButton] > 0){ //if timer is set
      LcdButton[MyButton]--; //decrease timer
      if (LcdButton[MyButton] == 0){ //if timer just hit 0
        switch (MyButton){ // set the default text and action based on button pressed
          case 0: //button 0
            //Serial.println("light interrupt expired");
            webPostrespond("light_interrupt", "end", 1);
          break;
          case 1: //button 1
            // re-enable TBD?
            //Serial.println("TBD interrupt expired");
          break;
          case 2: //button 2
            // re-enable ??
          break;
          case 3: //button 3
            //Serial.println("heater interrupt expired");
            webPostrespond("heater_interrupt", "end", 1);
          break;        
        }
      }
    }
    MyButton++;
  }
}
