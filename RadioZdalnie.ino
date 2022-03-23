/*
 WebService example used

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 modified 02 Sept 2015
 by Arturo Guadalupi
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <MCP7940.h>  // Include the MCP7940 RTC library
#include <Timezone.h>    // https://github.com/JChristensen/Timezone
#include <EEPROM.h>

#define addr_conf        0x10
#define budzik_on        0x01
#define budzik_off       0x02

#define addr_on_godzina  0x12
#define addr_on_minuta   0x14

#define addr_off_godzina  0x16
#define addr_off_minuta   0x18

byte budzik_conf;
byte on_godzina;
byte on_minuta;

byte off_godzina;
byte off_minuta;


//byte value;

unsigned int localPort = 8888;       // local port to listen for UDP packets

const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

MCP7940_Class MCP7940; 
const uint8_t  SPRINTF_BUFFER_SIZE{32};  // Buffer size for sprintf()x
char          inputBuffer[SPRINTF_BUFFER_SIZE];  // Buffer for sprintf()/sscanf()
int led = 33;
String readString;
// Wiznet MAC beginning 00-08-DC
// 
byte mac[] = {
  0x00, 0x08, 0xDC, 0xDE, 0xAD, 0x00
};
IPAddress ip(192, 168, 50, 77);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

uint8_t   daySync = 0; 
uint8_t   dayAlarmOn = 0;
uint8_t   dayAlarmOff = 0;
uint8_t   daySleep = 0;
uint8_t   daySleepHour = 30;
uint8_t   daySleepMin = 60;

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);


void setup() {

  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  // You can use Ethernet.init(pin) to configure the CS pin
  Ethernet.init(10);  // Most Arduino shields

  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Ethernet WebServer Example");

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
  while (!MCP7940.begin()) {  // Initialize RTC communications
    Serial.println(F("Unable to find MCP7940M. Checking again in 3s."));  // Show error text
    delay(3000);                                                          // wait a second
  } 
  Serial.print("bateria jest ");
  if(MCP7940.getBattery()) Serial.println("wlaczona"); else Serial.println("wylaczona ");
  
  while (!MCP7940.deviceStatus()) {  // Turn oscillator on if necessary
    Serial.println(F("Oscillator is off, turning it on."));
    bool deviceStatus = MCP7940.deviceStart();  // Start oscillator and return state
    if (!deviceStatus) {                        // If it didn't start
      Serial.println(F("Oscillator did not start, trying again."));  // Show error and
      delay(1000);                                                   // wait for a second
    }                // of if-then oscillator didn't start
  }                  // of while the oscillator is off
  //MCP7940.adjust(); 
  DateTime       now = MCP7940.now();
  sprintf(inputBuffer, "%04d-%02d-%02d %02d:%02d:%02d",
            now.year(),  // Use sprintf() to pretty print
            now.month(), now.day(), now.hour(), now.minute(),
            now.second());                         // date/time with leading zeros
          Serial.println(inputBuffer);
                         // date/time with leading zeros
          Serial.println(now.unixtime());
  Udp.begin(localPort);
  
  syncTime();

  odczytBudzika();
  
  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


void loop() {
  // listen for incoming clients
   EthernetClient client = server.available();
   DateTime       teraz = MCP7940.now();
   TimeChangeRule *tcr;        // pointer to the time change rule, use to get the TZ abbrev
    uint32_t local_time = CE.toLocal(teraz.unixtime(), &tcr);
    DateTime now = DateTime(local_time);
  if (client) {
   
    
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        ///Serial.write(c);

        //read char by char HTTP request
        if (readString.length() < 100) {
          //store characters to string
          readString += c;
          //Serial.print(c);
         }
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          ///client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
       client.println("<HTML>");
           client.println("<HEAD>");
           client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
           client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
           client.println("<link rel='stylesheet' type='text/css' href='https://randomnerdtutorials.com/ethernetcss.css' />");
           client.println("<TITLE>Random Nerd Tutorials Project</TITLE>");
           client.println("</HEAD>");
           client.println("<BODY>");
           client.println("<H2>Arduino with Ethernet Shield</H2>");
           client.println("<br />");  
           client.println("<a href=\"/?button1on\"\">Turn On LED</a>");
           client.println("<a href=\"/?button1off\"\">Turn Off LED</a><br /><br />");   
           
           client.println("<a href=\"/?radioonset\"\">Turn On LED</a>");
           client.println("<a href=\"/?radiooffset\"\">Turn Off LED</a><br /><br />");
           
           
           client.println("<a href=\"/?sleep1hour\"\">Sleep 1 hour</a>");
           client.println("<a href=\"/?sleep2hour\"\">Sleep 2 hours</a><br /><br />");
           client.println("<br /><h2>");     
           sprintf(inputBuffer, "%04d-%02d-%02d %02d:%02d:%02d",
            now.year(),  // Use sprintf() to pretty print
            now.month(), now.day(), now.hour(), now.minute(),
            now.second());                         // date/time with leading zeros
          client.println(inputBuffer);
           client.println("</h2><br />"); 
    
           client.println("<p>Created by Rui Santos. Visit https://randomnerdtutorials.com for more projects!</p>");  
           client.println("<br />"); 
           client.println("</BODY>");
           client.println("</HTML>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");

    if (readString.indexOf("?button1off") >0){
         digitalWrite(led, HIGH);
     }
     if (readString.indexOf("?button1on") >0){
         digitalWrite(led, LOW);
     }

     if (readString.indexOf("?sleep1hour") >0){
         digitalWrite(led, LOW);
         daySleep = 1;
         daySleepHour = now.hour()+1;
         if(daySleepHour>23) daySleepHour -= 24;
         daySleepMin = now.minute();
     }
     if (readString.indexOf("?sleep2hours") >0){
         digitalWrite(led, LOW);
         digitalWrite(led, LOW);
         daySleep = 1;
         daySleepHour = now.hour()+2;
         if(daySleepHour>23) daySleepHour -= 24;
         daySleepMin = now.minute();
     }
     
     if (readString.indexOf("?radioonset") >0){
         EEPROM.write(addr_on_godzina, 19);
         EEPROM.write(addr_on_minuta, 00);

     }
     if (readString.indexOf("?radiooffset") >0){
         EEPROM.write(addr_off_godzina, 19);
         EEPROM.write(addr_off_minuta, 59);
     }
     //clearing string for next read
     readString="";  
  }
  if(daySync != now.day() && now.hour() == 0)
    syncTime();
  //////////////////////////////////////////////
  //zalaczenie radia - budzika
  //

  if(dayAlarmOn != now.day() && now.hour() == (uint8_t)on_godzina && now.minute() == (uint8_t)on_minuta)
  {
    digitalWrite(led, LOW);
    dayAlarmOn = now.day();
  }

  //////////////////////////////////////////////
  //wylaczenie radia - budzika
  //
  if(dayAlarmOff != now.day() && now.hour() == (uint8_t)off_godzina && now.minute() == (uint8_t)off_minuta)
  {
    digitalWrite(led, HIGH);
    dayAlarmOff = now.day();
  }

  //////////////////////////////////////////////
  //wylaczenie radia - sleep
  //
  if(daySleep == 1 && now.hour() == daySleepHour && now.minute() == daySleepMin)
  {
    digitalWrite(led, HIGH);
    daySleep = 0;
  }

  
}

void syncTime()
{
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  delay(1000);
  
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    uint32_t highWord = word(packetBuffer[40], packetBuffer[41]);
    uint32_t lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    uint32_t secsSince1900 = highWord << 16 | lowWord;

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const uint32_t seventyYears = 2208988800UL;
    // subtract seventy years:
    uint32_t epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);
    DateTime       nowrtc(epoch);
    MCP7940.calibrateOrAdjust(nowrtc);

    daySync = nowrtc.day();
    Serial.print("aktualizacja odbyla sie dnia: ");
    Serial.println(daySync);
    
    
    
  }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(const char * address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void odczytBudzika()
{
budzik_conf = EEPROM.read(addr_conf);
on_godzina = EEPROM.read(addr_on_godzina);
on_minuta = EEPROM.read(addr_on_minuta);
off_godzina = EEPROM.read(addr_off_godzina);
off_minuta = EEPROM.read(addr_off_minuta);
}
