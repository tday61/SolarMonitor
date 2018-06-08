
/*
  Solar Monitor
 
 created 25 April 2013
 by TD
 
 
 */
//#include <Time.h>
#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
//#include <Cosm.h>
#include <string.h>
#include <DHT.h>
#include <Event.h>
#include <Timer.h>
#include <Time.h>
#include <TimeLib.h>
#include "Wire.h"

#define DS3231_I2C_ADDRESS 0x68
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
   0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE1};

byte ip[] = { 10, 0, 0, 35};

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);
EthernetClient client;

// Tick Timer
Timer t;


boolean bAccessCodeEntered = false;

// Analog pin which we're monitoring (0 and 1 are used by the Ethernet shield)
int sensorPin = 2;

// Command outputs
#define LightsOnCmd   100
#define LightsOffCmd  1
#define W1onCmd   102
#define W1offCmd  2
#define W2onCmd   103
#define W2offCmd  3
#define W3onCmd   104
#define W3offCmd  4

// Relay output
#define LED 3 //
#define RELAY 8 // 
#define WaterRelay1 5
#define WaterRelay2 6
#define WaterRelay3 7
#define WDOG 24 // 
boolean bFlipFlop = false;
boolean bWDTreset = true; // Reset watch dog if true


// String for HTTP request variables
char pcHttpReqRsCmd[20] = {'\0'};
//volatile int iUrlCmd = -1;  // URL value received from webpage HTTP request
 int iUrlCmd = -1;
 
// Define field name in the submitted form
#define SUBMIT_BUTTON_FIELDNAME "RSCmd"
#define SUBMIT_PW_FIELDNAME "PwCmd"

// Solar variables
float fBattery = 0;
float fBattery2 = 0;
float fBatteryTotal = 0;
float fBatteryTotalAve = 0;
float fCurrentAve = 0;
float fCurrent = 0;
//float fLoadAmp = 0;
//volatile float fBattery3 = 0;
//volatile float fBattery4 = 0;
int iLDRValue = 0;
long iLDRValueAve = 0;
int iLDRlimit = 200;
int BatVoltPin1 = A0;
int BatVoltPin2 = A1;
//int LoadAmpPin = A2;
int ChargeAmpPin = A2;
int LDRPin = A4;
int MoisturePin = A3;
float fTemperature=0;
float fHumidity=0;
int iMoistureValue=0;
long iMoistureValueAve=0;
int iMoistureLimit=512;

// number of readings to average
int avecnt=0; 
#define AVENUM 60

boolean bLightsOverRide = false; // Override time setting
boolean bLightsRelayOn = false;  // Relay on status
boolean bWater1RelayOn = false;  // Water soleniod 1
boolean bWater2RelayOn = false;  // Water soleniod 2
boolean bWater3RelayOn = false;  // Water soleniod 3
boolean bWaterOverRide = false; // Override time setting
boolean bPowerOn = false; // Power point

long   InternalVcc    = 0;
char sNewTime[20] = "T1300D12-12-2016";
int led = 26;
int RadioD2 = 2;
int PowerOn = 30;

int LightsOnTime = 1730;
int LightsOffTime = 2330;
int WaterOnTime = 1800;
int Water1OnLen = 3;
int Water2OnLen = 0;
int Water3OnLen = 3;

//DHT
#define DHTPIN 38
// Setup a DHT22 instance
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);


// Time
//unsigned long epoch = 0;
//unsigned long secsSince1900 =0;
unsigned long lToday=0,lDelayTime=600,lLightsTimeOn=0,lLightsTimeOff=0;
unsigned long lWaterTimeOn=0;
unsigned long lSolenoid1TimeOn=0, lSolenoid1TimeOff=0;
unsigned long lSolenoid2TimeOn=0, lSolenoid2TimeOff=0;
unsigned long lSolenoid3TimeOn=0, lSolenoid3TimeOff=0;
unsigned long lNow = 0;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void setup() {
  // Wait for ethernet to reset
  delay(200);
  
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Wire.begin();
  // Disable SD SPI
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);
  
  // start the Ethernet connection and the server:
  Serial.println("Init network");
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  doSetTimeFromRTC();
  t.every(600000, doSetTimeFromRTC);
 
  // Control Loop
  t.every(5000, doControlActions);
  

  // Set read sensor callback
  t.every(1000, doReadSensorLoop);
 
  // Set DHT22 callback
 // dht.begin();
 // int tickEvent = t.every(10000, doDHTLoop);
 // doDHTLoop();
  
  t.every(1000, WebServerLoop);

  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);     
  pinMode(RadioD2, INPUT); 
  pinMode(PowerOn, OUTPUT); 
  pinMode(RELAY, OUTPUT);
  pinMode(WaterRelay1, OUTPUT);  
  pinMode(WaterRelay2, OUTPUT); 
  pinMode(WaterRelay3, OUTPUT); 
   
   digitalWrite(WaterRelay1,LOW);
  digitalWrite(WaterRelay2,LOW);
  digitalWrite(WaterRelay3,LOW);
  
  t.every(1000, doPowerOn);
  t.every(500,doLedLoop);

}


void loop() {
   // main program loop
  t.update(); // Update timer loops
//  displayTime(); // display the real-time clock data on the Serial Monitor,
//  delay(1000); // every second
}

// Functions
void doLedLoop(void)
{
   if(bFlipFlop) {
    digitalWrite(led,HIGH);
    bFlipFlop = false;
  }
  else {
    digitalWrite(led,LOW);
    bFlipFlop = true;
  }    
  // Reset watchdog
  pinMode(40,OUTPUT);
  delay(200);
  pinMode(40,INPUT);
}

void doSetTimeFromRTC()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);   
  displayTime();
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}
void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
  setTime(*hour,*minute,*second,*dayOfMonth,*month,*year); 
}
void displayTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  // send it to the serial monitor
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute<10)
  {
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second<10)
  {
    Serial.print("0");
  }
  Serial.print(second, DEC);
  Serial.print(" ");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print(" Day of week: ");
  switch(dayOfWeek){
  case 1:
    Serial.println("Sunday");
    break;
  case 2:
    Serial.println("Monday");
    break;
  case 3:
    Serial.println("Tuesday");
    break;
  case 4:
    Serial.println("Wednesday");
    break;
  case 5:
    Serial.println("Thursday");
    break;
  case 6:
    Serial.println("Friday");
    break;
  case 7:
    Serial.println("Saturday");
    break;
  }
}


void doPowerOn(void)
{
   if (digitalRead(RadioD2) == HIGH)
   {
     if(bPowerOn == false)
     {
       bPowerOn = true;
       digitalWrite(PowerOn,HIGH);
     }
     else
     {
       bPowerOn = false;
       digitalWrite(PowerOn,LOW);
     }
   }
}

void doControlActions(void)
{
//  Serial.println("do controls");
  lNow = now();
  unsigned long lSecToday;
  // Calculate today in seconds
  lSecToday = ((unsigned long)hour()*3600) + ((unsigned long)minute()*60) + (unsigned long)second();
  lToday = lNow - lSecToday;
  lLightsTimeOn = lToday + ConvertToSec(LightsOnTime);
  lLightsTimeOff = lToday + ConvertToSec(LightsOffTime);
  
  lWaterTimeOn = lToday + ConvertToSec(WaterOnTime);
  lSolenoid1TimeOn = lWaterTimeOn;
  lSolenoid1TimeOff = lSolenoid1TimeOn + ConvertToSec(Water1OnLen);
  lSolenoid2TimeOn = lSolenoid1TimeOff ;
  lSolenoid2TimeOff = lSolenoid2TimeOn + ConvertToSec(Water2OnLen);
  lSolenoid3TimeOn = lSolenoid2TimeOff ;
  lSolenoid3TimeOff = lSolenoid3TimeOn + ConvertToSec(Water3OnLen);
  
  // Check time to turn on lights
  if(!bLightsOverRide) 
  {
    if((lNow > lLightsTimeOn) && (lNow < lLightsTimeOff) && (iLDRValue < iLDRlimit) && !bLightsRelayOn)
    {
      // Wait for a delay before turning on - deadband for LDR
      PrintCurrentTime();
        Serial.println("turn lights on");
        digitalWrite(RELAY,HIGH);
        bLightsRelayOn = true;
     }
     if (((lNow > lLightsTimeOff) || (iLDRValue > iLDRlimit)) && bLightsRelayOn) { 
        PrintCurrentTime();
      Serial.println("turn ligts off");
      digitalWrite(RELAY,LOW);
      // Wait before turning back on the next night
       bLightsRelayOn = false;
    }
  }  
 
  if(!bWaterOverRide) 
  {
    if((lNow > lSolenoid1TimeOn) && (lNow < lSolenoid1TimeOff) && (iMoistureValue < iMoistureLimit) && !bWater1RelayOn)  
    {
      PrintCurrentTime();
      Serial.println("turn water1 on");
      digitalWrite(WaterRelay1,HIGH);
      bWater1RelayOn = true;
    }
    if ((lNow > lSolenoid1TimeOff) && bWater1RelayOn) 
    { 
      PrintCurrentTime();
      Serial.println("turn water1 off");
      digitalWrite(WaterRelay1,LOW);
      bWater1RelayOn = false;
    }

    if((lNow > lSolenoid2TimeOn) && (lNow < lSolenoid2TimeOff) && (iMoistureValue < iMoistureLimit) && !bWater2RelayOn)  
    {
      PrintCurrentTime();
      Serial.println("turn water2 on");
      digitalWrite(WaterRelay2,HIGH);
      bWater2RelayOn = true;
    }
    if ((lNow > lSolenoid2TimeOff) && bWater2RelayOn) 
    { 
      PrintCurrentTime();
      Serial.println("turn water2 off");
      digitalWrite(WaterRelay2,LOW);
      bWater2RelayOn = false;
    }
 
    if((lNow > lSolenoid3TimeOn) && (lNow < lSolenoid3TimeOff) && (iMoistureValue < iMoistureLimit) && !bWater3RelayOn)  
    {
      PrintCurrentTime();
      Serial.println("turn water3 on");
      digitalWrite(WaterRelay3,HIGH);
      bWater3RelayOn = true;
    }
    if ((lNow > lSolenoid3TimeOff) && bWater3RelayOn) 
    { 
      PrintCurrentTime();
      Serial.println("turn water3 off");
      digitalWrite(WaterRelay3,LOW);
      bWater3RelayOn = false;
    }
  }

}

void doReadSensorLoop(void)
{
  int sensorValue = 0;
  float fval = 0; 
//     Serial.println("sensor loop");
  sensorValue = analogRead(BatVoltPin1);    
//  Serial.println(sensorValue, 1);
  fval = (float)(sensorValue-6)/31.8;
  fBatteryTotalAve = fBatteryTotalAve + fval;
//  Serial.println(fval, 2);

  // 20A sensor = 100mV/A
  // ADC sensitivity = 0.004883v/count
  // 0A = 2.5V measured 3.4
  // a=(0.004883*c - 2.5)/0.1
  // a = .04883*c - 25
  fval = analogRead(ChargeAmpPin);    
  fval = fval ;
//  Serial.println(fval, 0);
  fval = fval-518; // 0 value from sensor
//  Serial.println(fval, 2);
  fval = fval*-0.077; // 
//  Serial.println(fval, 2);
  fCurrent = fval;
  // Read LDR
  iLDRValueAve = iLDRValueAve + analogRead(LDRPin);    

  // Read Moisture
  iMoistureValueAve = iMoistureValueAve + analogRead(MoisturePin);    
 
  avecnt++;
  if(avecnt >= AVENUM) // number of readings
  {
    fBatteryTotal = fBatteryTotalAve/avecnt;
//    Serial.println(fBatteryTotal, 2);
   fCurrent = fCurrentAve/avecnt;
//    Serial.println(fCurrent, 5);
    iLDRValue = iLDRValueAve/avecnt;
    iMoistureValue = iMoistureValueAve/avecnt;
    
    // Reset values
    fBatteryTotalAve = 0;
    fCurrentAve = 0;
    iLDRValueAve = 0;
    iMoistureValueAve = 0;
    avecnt = 0;
  }   

}


void WebServerLoop(void)
{
  // listen for incoming clients
  EthernetClient client = server.available();
  boolean bPendingHttpResponse = false; // True when we've received a whole HTTP request and need to output the webpage
  char c;  // For reading in HTTP request one character at a time

 
  if (client) {
    Serial.println("new client");
    char buf[10];
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
        delay(1000); //timing delay else webserver hangs.
      // Do we have pending data (an HTTP request)
        if (client.available()) {
          // Indicate we need to respond to the HTTP request as soon as we're done processing it
          bPendingHttpResponse = true;

          ParseHttpHeader(client);        
        }
        else
        {
          // There's no data waiting to be read in on the client socket.  Do we have a pending HTTP request?
          if(bPendingHttpResponse)
          {
            // Yes, we have a pending request.  Clear the flag and then send the webpage to the client
            bPendingHttpResponse = false;
            // send a standard http response header and HTML header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();
 //           client.println("<HTML>\n<HEAD><META HTTP-EQUIV=\"refresh\" CONTENT=\"15\">");
            client.println("<TITLE>Arduino Solar Monitor Webserver</TITLE>");//
            client.println("</HEAD><BODY bgcolor=\"#9bbad6\">");
            // Put out a text header
            client.print("<H1>Solar Lights & Water System Control</H1>");   
            
      //      client.println("<A HREF=\"javascript:history.go(0)\"></A>");
        //    client.println("<FORM><INPUT TYPE=\"button\" onClick=\"history.go(0)\" VALUE=\"Refresh\"></FORM>"); 
            client.println("<a href=\"http://10.0.0.35:8080\">Refresh</a>"); 

            
            // Print time
           client.print("<p><b>");
           if(day() < 10) client.print('0');
           client.print(day());
           client.print('-');
           if(month() < 10) client.print('0');
           client.print(month());
           client.print('-');
           client.print(year());
           client.print("  ");
           if(hour() < 10) client.print('0');
           client.print(hour());
           client.print(':');
           if(minute() < 10) client.print('0');
           client.print(minute());
           client.print(':');
           if(second() < 10) client.print('0');
           client.print(second());
           client.print("</p></b>");
           
           // Time change
           client.print("<form method='get'><input type='text' name='time' size=20 value=");
           client.print(sNewTime);
           client.print(" ><input type='submit' value='Update'></form>");

          
            // Create table
            client.print("<table border=2>");
            client.print("<tr><th>Battery Volts</th><th>Battery Amps</th></tr>");
            // Table row 1
            client.print("<tr><td align=center>");
            client.print(fBatteryTotal);
            client.print("</td><td align=center>");
            client.print(fCurrent);
            client.print("</td></tr></table>");
            
            client.println("<br><br>");

            client.print("<table border=2>");
            client.print("<tr><th>Temperature</th><th>Humidity</th></tr>");
            client.print("<tr><td align=center>");
            client.print(fTemperature);
            client.print("</td><td align=center>");
            client.print(fHumidity);
            client.print("</td></tr>");
            client.print("</table>"); 
 
            client.println("<br><br>");

            client.print("<table border=2>");
            client.print("<tr><th colspan='2'");
            if(iLDRValue < iLDRlimit) 
              client.print(" bgcolor=#FF0000>");
            else client.print(" bgcolor=#00FF00>");
            client.print("LDR</th></tr>");
            client.print("<tr><td>Value</td><td align=center >Limit</td>");
            client.print("<tr><td>");
            client.print(iLDRValue);
            client.print("</td><td><form method='get'><input type='text' name='llim' size=4 value=");
            client.print(iLDRlimit);
            client.print(" ><input type='submit' value='Update'></form></tr></td>");
            client.print("</table>");    
            
            client.println("<br><br>");
            
            // Display lights table
            client.print("<table border=2>");
            client.print("<tr><th colspan='3' ");
            if(bLightsRelayOn) 
              client.print("th bgcolor=#FF0000>");
            else client.print("th bgcolor=#00FF00>");
            client.print("Lights</th></tr>");
            client.print("<tr><td  align=center colspan='3'><form method='get'>On Time:  <input type='text' name='lon' size=5 value=");
            sprintf(buf, "%04d",LightsOnTime); 
            client.print(buf);
            client.print("  ><input type='submit' value='Update'></form></tr></td>");
            client.print("<tr><td  align=center colspan='3'><form method='get'>Off Time:  <input type='text' name='loff' size=5  value=");
            sprintf(buf, "%04d",LightsOffTime); 
            client.print(buf);
            client.print("  ><input type='submit' value='Update'></form></tr></td>");
 
            client.println("<tr><td>Override:</td>");
            client.println("<td align=center>");
            // Create buttons
            SubmitButton(client, "On", LightsOnCmd);          
            client.println("</td><td align=center>");
            SubmitButton(client, "Off", LightsOffCmd);
            client.println("</td></tr></table>");
            
            client.println("<br><br>");
 
            client.print("<table border=2>");
            client.print("<tr><th colspan='2'");
            if(iMoistureValue < iMoistureLimit) 
              client.print(" bgcolor=#FF0000>");
            else client.print(" bgcolor=#00FF00>");
            client.print("Moisture</th></tr>");
            client.print("<tr><td>Value</td><td align=center >Limit</td>");
            client.print("<tr><td>");
            client.print(iMoistureValue);
            client.print("</td><td><form method='get'><input type='text' name='mlim' size=4 value=");
            client.print(iMoistureLimit);
            client.print(" ><input type='submit' value='Update'></form></tr></td>");
            client.print("</table>");    
            
            client.println("<br><br>");
 
           // Display water table
            client.print("<table border=2>");
            client.print("<tr><th colspan='3' ");
            if(bWater1RelayOn || bWater2RelayOn || bWater3RelayOn) 
              client.print("th bgcolor=#FF0000>");
            else client.print("th bgcolor=#00FF00>");
            client.print("Water</th></tr>");
            client.print("<tr><td  align=center colspan='3'><form method='get'>On Time:  <input type='text' name='won' size=5 value=");
            sprintf(buf, "%04d",WaterOnTime); 
            client.print(buf);
            client.print("  ><input type='submit' value='Update'></form></tr></td>");
            client.print("<tr><th>Solenoid</th><th>Minutes</th><th>Status</th></tr>");

            client.println("<form method='get'><tr><td align=center>1</td>");
            client.println("<td  align=center><input type='text' name='w1len' size=3 value= "); 
            client.print(Water1OnLen);
            client.println("></td>");
            if(bWater1RelayOn) 
              client.print("<td bgcolor=#FF0000>On</td></tr>");
            else client.print("<td bgcolor=#00FF00>Off</td></tr>");
            
            client.println("<tr><td align=center>2</td>");
            client.println("<td align=center> <input type='text' name='w2len' size=3 value= "); 
            client.print(Water2OnLen);
            client.println("></td>");
            if(bWater2RelayOn) 
              client.print("<td bgcolor=#FF0000>On</td></tr>");
            else client.print("<td bgcolor=#00FF00>Off</td></tr>");
        
            client.println("<tr><td align=center>3</td>");
            client.println("<td align=center><input type='text' name='w3len' size=3 value= "); 
            client.print(Water3OnLen);
            client.println("></td>");
            if(bWater3RelayOn) 
              client.print("<td bgcolor=#FF0000>On</td></tr>");
            else client.print("<td bgcolor=#00FF00>Off</td></tr>");
            client.println("<tr><td colspan='3' align=center><input type='submit' value='Update On Time'></form></tr></td>");          
           
 
            // Water override butons
            client.println("<tr><td>Override 1:</td>");
            client.println("<td align=center>");
            SubmitButton(client, "On", W1onCmd);          
            client.println("</td><td align=center>");
            SubmitButton(client, "Off", W1offCmd);
            client.println("</td></tr>");
            
            client.println("<tr><td>Override 2:</td>");
            client.println("<td align=center>");
            SubmitButton(client, "On", W2onCmd);          
            client.println("</td><td align=center>");
            SubmitButton(client, "Off", W2offCmd);
            client.println("</td></tr>");
            
            client.println("<tr><td>Override 3:</td>");
            client.println("<td align=center>");
            SubmitButton(client, "On", W3onCmd);          
            client.println("</td><td align=center>");
            SubmitButton(client, "Off", W3offCmd);
            client.println("</td></tr>");           
            client.println("</td></tr></table>");
         

            // Access status
            SubmitAccessRequest(client);
            
 
            //  HTML footer
            client.println("</BODY></HTML>");
            
            // give the web browser time to receive the data
            delay(1);
            client.stop();
            Serial.println("client disonnected");
         }  
        }
     } //End while loop

  }
}

// Parse an HTTP request header one character at a time, seeking string variables
void ParseHttpHeader(Client &client)
{
  char c;
  int MaxBuf = 50;
  char cbuff[MaxBuf];
  int i=0;
  
 // Serial.println("ParseHTTPheader");
  
    while(client.available())
    {
      c = client.read();
      if(i < MaxBuf)   // Keep first 20 chars
        cbuff[i++] = c;
      // Debug - print data
      Serial.print(c);
    }

   
    i = 5; // start of ?
    if((cbuff[i] == '?') && (cbuff[i+1] == 'm') && (cbuff[i+2] == 'e') && (cbuff[i+4] == 's')) {
      if((cbuff[i+9] == '1') && (cbuff[i+10] == '2') && (cbuff[i+11] == 'a') && (cbuff[i+12] == 's')) {
        bAccessCodeEntered=true;
      }
      else bAccessCodeEntered=false;
    }
    
    // Send button command  
    if(bAccessCodeEntered && ((cbuff[i+1] != 'm') || (cbuff[i+1] != 'o'))) 
    {
      char pcUrlNum[3], *pc;
      // We have enough data for a hex number - read it
      for(int x=0; x < 2; x++)
      {
        // Read and dump data to debug port
        pcUrlNum[x] = cbuff[i+x+2];
 //       Serial.print(pcUrlNum[x]);
      }
      // Null terminate string
      pcUrlNum[2] = '\0';
    
      // Get hex number
      iUrlCmd = strtol(pcUrlNum, &pc, 0x10);   
      Serial.print("Sending command: ");
//      Serial.println(iUrlCmd, HEX);
      SendCommand(iUrlCmd);
    }

    // New time entered
 //     Serial.println(cbuff[i]);  Serial.println(cbuff[i+1]); Serial.println(cbuff[i+2]); Serial.println(cbuff[i+3]);

    if(bAccessCodeEntered && (cbuff[i+1] == 't') && (cbuff[i+2] == 'i') && (cbuff[i+3] == 'm')) 
    {
        int h,m,s,d,mo,y;
        c = cbuff[i+7];
        h = int(c-0x30)*10; 
        c = cbuff[i+8];
        h = h + int(c-0x30); 
        
        c = cbuff[i+9];
        m = int(c-0x30)*10; 
        c = cbuff[i+10];
        m = m + int(c-0x30); 

        c = cbuff[i+12];
        d = int(c-0x30)*10; 
        c = cbuff[i+13];
        d = d + int(c-0x30); 

        c = cbuff[i+15];
        mo = int(c-0x30)*10; 
        c = cbuff[i+16];
        mo = mo + int(c-0x30); 

        c = cbuff[i+18];
        y = int(c-0x30)*1000; 
        c = cbuff[i+19];
        y = y + int(c-0x30)*100; 
        c = cbuff[i+20];
        y = y + int(c-0x30)*10; 
        c = cbuff[i+21];
        y = y + int(c-0x30); 
       setTime(h,m,s,d,mo,y); 
    }

    // Lights On time value entered? /?lon=HHMM
    //Serial.println(cbuff[i]);  Serial.println(cbuff[i+1]);
    if(bAccessCodeEntered && (cbuff[i+1] == 'l') && (cbuff[i+2] == 'o') && (cbuff[i+3] == 'n')) 
    {
        int y=0;
        c = cbuff[i+5];
        y = int(c-0x30)*1000; 
        c = cbuff[i+6];
        y = y + int(c-0x30)*100; 
        c = cbuff[i+7];
        y = y + int(c-0x30)*10; 
        c = cbuff[i+8];
        y = y + int(c-0x30); 
        if(y >= 0 && y<2400)
          LightsOnTime = y;
    }
    // Lights Off time value entered? /?loff=HHMM
   // Serial.println(cbuff[i]);  Serial.println(cbuff[i+1]);
    if(bAccessCodeEntered && (cbuff[i+1] == 'l') && (cbuff[i+2] == 'o') && (cbuff[i+3] == 'f')) 
    {
        int y=0;
        c = cbuff[i+6];
        y = int(c-0x30)*1000; 
        c = cbuff[i+7];
        y = y + int(c-0x30)*100; 
        c = cbuff[i+8];
        y = y + int(c-0x30)*10; 
        c = cbuff[i+9];
        y = y + int(c-0x30); 
        if(y >= 0 && y<2400)
          LightsOffTime = y;
    }
 
    // Water On time value entered? /?won=HHMM
    //Serial.println(cbuff[i]);  Serial.println(cbuff[i+1]);
    if(bAccessCodeEntered && (cbuff[i+1] == 'w') && (cbuff[i+2] == 'o') && (cbuff[i+3] == 'n')) 
    {
        int y=0;
        c = cbuff[i+5];
        y = int(c-0x30)*1000; 
        c = cbuff[i+6];
        y = y + int(c-0x30)*100; 
        c = cbuff[i+7];
        y = y + int(c-0x30)*10; 
        c = cbuff[i+8];
        y = y + int(c-0x30); 
        if(y >= 0 && y<2400)
          WaterOnTime = y;
    }

    // Water On length entered? /?w1len=x&w2len=x&w3len=x
   // Serial.println(cbuff[i]);  Serial.println(cbuff[i+1]);
    if(bAccessCodeEntered && (cbuff[i+1] == 'w') && (cbuff[i+2] == '1') && (cbuff[i+3] == 'l')) 
    {
        int y=0;
        c = cbuff[i+7];
        y = int(c-0x30); 
        if (y >= 0 && y < 10)
          Water1OnLen = y;
        c = cbuff[i+15];
        y = int(c-0x30); 
        if (y >= 0 && y < 10)
          Water2OnLen = y;
        c = cbuff[i+23];
        y = int(c-0x30); 
        if (y >= 0 && y < 10)
          Water3OnLen = y;
    }

   // LDR limit entered? /?llim=xxx
   // Serial.println(cbuff[i]);  Serial.println(cbuff[i+1]);
    if(bAccessCodeEntered && (cbuff[i+1] == 'l') && (cbuff[i+2] == 'l') && (cbuff[i+3] == 'i')) 
    {
        // Expect a number between 1 and 999
        int y=0;
        c = cbuff[i+6];
        y = int(c-0x30); 
        if (y >= 0 && y < 10)
          iLDRlimit = y;
        c = cbuff[i+7];
        y = int(c-0x30); 
        if (y >= 0 && y < 10)
          iLDRlimit = (iLDRlimit*10) + y;
        c = cbuff[i+8];
        y = int(c-0x30); 
        if (y >= 0 && y < 10)
          iLDRlimit = (iLDRlimit*10) + y;
    }

   // Moisture limit entered? /?mlim=xxx
 //   Serial.println(cbuff[i]);  Serial.println(cbuff[i+1]);
    if(bAccessCodeEntered && (cbuff[i+1] == 'm') && (cbuff[i+2] == 'l') && (cbuff[i+3] == 'i')) 
    {
        int y=0;
        c = cbuff[i+6];
        y = int(c-0x30); 
        if (y >= 0 && y < 10)
          iMoistureLimit = y;
        c = cbuff[i+7];
        y = int(c-0x30); 
        if (y >= 0 && y < 10)
          iMoistureLimit = (iMoistureLimit*10) + y;
        c = cbuff[i+8];
        y = int(c-0x30); 
        if (y >= 0 && y < 10)
          iMoistureLimit = (iMoistureLimit*10) + y;
    }
  // Skip through and discard all remaining data

    Serial.println("remaining data");
    client.read();
    // Debug - print data
    Serial.print(c = client.read());
 }


// Print a submit button with the indicated label wrapped in a form for the indicated hex command
void SubmitButton(Client &client, char *pcLabel, int iCmd)
{
  client.print("<form  method=post action=\"/?");
  client.print(iCmd, HEX);
  client.print("\"><input type=submit value=\"");
  client.print(pcLabel);
  client.print("\" name=\"" SUBMIT_BUTTON_FIELDNAME "\">");
  client.println("</form>");  
}

// Print a submit button with the indicated label wrapped in a form for the indicated hex command
void SubmitAccessRequest(Client &client)
{
   client.print("<p><i><b>Warning: controls ");
   if(bAccessCodeEntered) 
     client.print("enabled</b></i></p>");
   else 
     client.print("disabled</b></i></p>");
 
   client.print("<p> <form method=get>Access: <input type='text' name='message' size=6 autocomplete='off' /> ");
   client.print("<input type='submit' value='Access'></form>");//
}


// send the whole 8 bits
void SendCommand(int command) 
{
 // Serial.print("command ");
 // Serial.println(command, HEX);
  switch(command)
  {
    case LightsOffCmd: 
      digitalWrite(RELAY,LOW);
      bLightsOverRide = false;
      bLightsRelayOn = false;
      Serial.println("override off");
       break;
    case LightsOnCmd: 
      digitalWrite(RELAY,HIGH);
      bLightsOverRide = true;
      bLightsRelayOn = true;
      Serial.println("override on");
      break;
    case W1offCmd: 
      digitalWrite(WaterRelay1,LOW);
      bWaterOverRide = false;
      bWater1RelayOn = false;
      Serial.println("w1 override off");
       break;
    case W1onCmd: 
      digitalWrite(WaterRelay1,HIGH);
      bWaterOverRide = true;
      bWater1RelayOn = true;
      Serial.println("w1 override on");
      break;
    case W2offCmd: 
      digitalWrite(WaterRelay2,LOW);
      bWaterOverRide = false;
      bWater2RelayOn = false;
      Serial.println("w2 override off");
       break;
    case W2onCmd: 
      digitalWrite(WaterRelay2,HIGH);
      bWaterOverRide = true;
      bWater2RelayOn = true;
      Serial.println("w2 override on");
      break;
    case W3offCmd: 
      digitalWrite(WaterRelay3,LOW);
      bWaterOverRide = false;
      bWater3RelayOn = false;
      Serial.println("w3 override off");
       break;
    case W3onCmd: 
      digitalWrite(WaterRelay3,HIGH);
      bWaterOverRide = true;
      bWater3RelayOn = true;
      Serial.println("w3 override on");
      break;
  }
 }



void doDHTLoop(void)
{
 // fHumidity = dht.readHumidity();
  //fTemperature = dht.readTemperature();

  // DISPLAY DATA
  PrintCurrentTime();
   Serial.print(fHumidity, 1);
   Serial.print(",\t");
   Serial.print(fTemperature, 1);
    Serial.print(",\t");
   Serial.print(fBatteryTotal, 1);
    Serial.print(",\t");
   Serial.print(fCurrent, 1);
    Serial.print(",\t");
   Serial.print(iMoistureValue, 1);
    Serial.print(",\t");
   Serial.print(iMoistureLimit, 1);
    Serial.print(",\t");
   Serial.print(iLDRValue, 1);
    Serial.print(",\t");
   Serial.println(iLDRlimit, 1);
}


void PrintCurrentTime(void)
{
  
 //   Serial.println(); 
   Serial.print(day());
   Serial.print(" ");
   Serial.print(month());
   Serial.print(" ");
   Serial.print(year()); 
  
   Serial.print(" ");
   Serial.print(hour());
   printDigits(minute());
   printDigits(second());
   Serial.print(" ");

 
}

void printDigits(int digits){
   // utility function for digital clock display: prints preceding colon and leading 0
   Serial.print(":");
   if(digits < 10)
     Serial.print('0');
   Serial.print(digits);
}



unsigned long ConvertToSec(int value)
{
    unsigned long h, m;
    h = value/100;
 //   Serial.println(h);
    m = value%100;
 //   Serial.println(m);
    return((h*3600) + (m*60));
}




