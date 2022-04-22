//  IR LED SIGNAL => ESP/GPIO_4
//  MIC SIGNAL => ESP/GPIO36/ADC1 CH0
//todo: 
  //listen for a short beep or a long beep
  //need to create a microphone input to listen for the beeps
  //need to create an dhcp redirect to get more pages up
  //default the web page selections to the last "on" values make the on and off buttons submit buttons using "selected"

#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

DNSServer dnsServer;
AsyncWebServer server(80);

String HVAC_Mode;
String HVAC_Temp;
int HVAC_Temp_i;
String HVAC_Fan_Mode;
String HVAC_Vane_Mode;
String HVAC_On;
bool HVAC_On_b;
bool HVAC_Mode_received = false;
bool HVAC_Temp_received = false;
bool HVAC_Fan_Mode_received = false;
bool HVAC_Vane_Mode_received = false;
bool HVAC_On_received = false;

int halfPeriodicTime;
int IRpin;
int khz;
int ONBOARD_LED;
const int microphonePin = 36;
int microphoneValue = 0; //0-4095
const int microphoneAverageLimit = 1000;
int microphoneAverageCount = 0;
long microphoneAverage = 0;

typedef enum HvacMode {
  HVAC_HOT,
  HVAC_COLD,
  HVAC_DRY,
  HVAC_AUTO
} HvacMode_t; // HVAC  MODE
HvacMode HvacMode_1;

typedef enum HvacFanMode {
  FAN_SPEED_1,
  FAN_SPEED_2,
  FAN_SPEED_3,
  FAN_SPEED_4,
  FAN_SPEED_5,
  FAN_SPEED_AUTO,
  FAN_SPEED_SILENT
} HvacFanMode_t;  // HVAC  FAN MODE
HvacFanMode HvacFanMode_1;

typedef enum HvacVanneMode {
  VANNE_AUTO,
  VANNE_H1,
  VANNE_H2,
  VANNE_H3,
  VANNE_H4,
  VANNE_H5,
  VANNE_AUTO_MOVE
} HvacVanneMode_t;  // HVAC  VANNE MODE
HvacVanneMode HvacVanneMode_1;

// HVAC MITSUBISHI_
#define HVAC_MITSUBISHI_HDR_MARK    3400
#define HVAC_MITSUBISHI_HDR_SPACE   1750
#define HVAC_MITSUBISHI_BIT_MARK    450
#define HVAC_MITSUBISHI_ONE_SPACE   1300
#define HVAC_MISTUBISHI_ZERO_SPACE  420
#define HVAC_MITSUBISHI_RPT_MARK    440
#define HVAC_MITSUBISHI_RPT_SPACE   17100 // Above original iremote limit

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>HVAC Captive Portal</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h3>Mitsubishi Inverter HVAC Web Remote</h3>
  <form action="/get">
    <label for="HVAC_Mode">Mode: </label>
    <select name = "HVAC_Mode">
      <option value=HVAC_AUTO>Auto</option>
      <option value=HVAC_HOT>Heat</option>
      <option value=HVAC_COLD>Cool</option>
      <option value=HVAC_DRY>Dehumidify</option>
    </select>
    <br>
    <label for="HVAC_Temp">Temp: </label>
    <select name = "HVAC_Temp">
      <option value=15>15</option>
      <option value=16>16</option>
      <option value=17>17</option>
      <option value=18>18</option>
      <option value=19>19</option>
      <option value=20 selected>20</option>
      <option value=21>21</option>
      <option value=22>22</option>
      <option value=23>23</option>
      <option value=24>24</option>
      <option value=25>25</option>
      <option value=26>26</option>
      <option value=27>27</option>
      <option value=28>28</option>
      <option value=29>29</option>
      <option value=30>30</option>
      <option value=31>31</option>
      <option value=32>32</option>
    </select>
    <br>
    <label for="HVAC_Fan_Mode">Fan: </label>
    <select name = "HVAC_Fan_Mode">
      <option value=FAN_SPEED_AUTO>Auto</option>
      <option value=FAN_SPEED_1>1</option>
      <option value=FAN_SPEED_2>2</option>
      <option value=FAN_SPEED_3>3</option>
      <option value=FAN_SPEED_4>4</option>
      <option value=FAN_SPEED_5>5</option>
      <option value=FAN_SPEED_SILENT>Silent</option>
    </select>
    <br>
    <label for="HVAC_Vane_Mode">Vane: </label>
    <select name = "HVAC_Vane_Mode">
      <option value=VANE_AUTO>Auto</option>
      <option value=VANE_H1>1</option>
      <option value=VANE_H2>2</option>
      <option value=VANE_H3>3</option>
      <option value=VANE_H4>4</option>
      <option value=VANE_H5>5</option>
      <option value=VANE_AUTO_MOVE>Oscillate</option>
    </select>
    <br>
    <input type="submit" id="HVAC_On" name="HVAC_On" value="On">
    <label for="HVAC_On">On</label>
    <br>
    <input type="submit" id="HVAC_On" name="HVAC_On" value="Off">
    <label for="HVAC_Off">Off</label>
    <br>
  </form>
</body></html>)rawliteral";

const char response_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>HVAC Captive Portal</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="10; url='/'">
  </head><body>
  <h3>The values entered by you have been successfully sent to the device. 
  <br><a href="/">Return to Home Page</a></h3>
  <br> Page will refresh to home page in 10 seconds.
</body></html>)rawliteral";

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  }
};

void setupServer(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html); 
      Serial.println("Client Connected");
  });
    
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String inputMessage;
      String inputParam;
  
      if (request->hasParam("HVAC_Mode")          ) {
        inputMessage = request->getParam("HVAC_Mode")->value();
        inputParam = "HVAC_Mode";
        HVAC_Mode = inputMessage;
        Serial.print("HVAC_Mode = ");
        Serial.println(inputMessage);
        HVAC_Mode_received = true;
      }

      if (request->hasParam("HVAC_Temp")) {
        inputMessage = request->getParam("HVAC_Temp")->value();
        inputParam = "HVAC_Temp";
        HVAC_Temp = inputMessage;
        Serial.print("HVAC_Temp = ");
        Serial.println(inputMessage);
        HVAC_Temp_received = true;
      }

      if (request->hasParam("HVAC_Fan_Mode")) {
        inputMessage = request->getParam("HVAC_Fan_Mode")->value();
        inputParam = "HVAC_Fan_Mode";
        HVAC_Fan_Mode = inputMessage;
        Serial.print("HVAC_Fan_Mode = ");
        Serial.println(inputMessage);
        HVAC_Fan_Mode_received = true;
      }

      if (request->hasParam("HVAC_Vane_Mode")) {
        inputMessage = request->getParam("HVAC_Vane_Mode")->value();
        inputParam = "HVAC_Vane_Mode";
        HVAC_Vane_Mode = inputMessage;
        Serial.print("HVAC_Vane_Mode = ");
        Serial.println(inputMessage);
        HVAC_Vane_Mode_received = true;
      }

      if (request->hasParam("HVAC_On")) {
        inputMessage = request->getParam("HVAC_On")->value();
        inputParam = "HVAC_On";
        HVAC_On = inputMessage;
        Serial.print("HVAC_On = ");
        Serial.println(inputMessage);
        HVAC_On_received = true;
      }
      
      request->send_P(200, "text/html", response_html);

  });
}


void setup(){
  //your other setup stuff...
  Serial.begin(115200);
  Serial.println();
  Serial.println("Setting up AP Mode");
  WiFi.mode(WIFI_AP); 
  WiFi.softAP("Master-Bedroom-HVAC");
  Serial.print("AP IP address: ");Serial.println(WiFi.softAPIP());
  Serial.println("Setting up Async WebServer");
  setupServer();
  Serial.println("Starting DNS Server");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  //more handlers...
  server.begin();

    // put your setup code here, to run once:
  IRpin=4;
  ONBOARD_LED=2;
  khz=38;
  halfPeriodicTime = 500/khz;
  pinMode(IRpin, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);

  Serial.println("All Done!");
}

/****************************************************************************
/* Send IR command to Mitsubishi HVAC - sendHvacMitsubishi
/***************************************************************************/
void sendHvacMitsubishi(
  HvacMode                HVAC_Mode,           // Example HVAC_HOT  HvacMitsubishiMode
  int                     HVAC_Temp,           // Example 21  (°c)
  HvacFanMode             HVAC_FanMode,        // Example FAN_SPEED_AUTO  HvacMitsubishiFanMode
  HvacVanneMode           HVAC_VanneMode,      // Example VANNE_AUTO_MOVE  HvacMitsubishiVanneMode
  int                     OFF                  // Example false
)
{

//#define  HVAC_MITSUBISHI_DEBUG;  // Un comment to access DEBUG information through Serial Interface

  byte mask = 1; //our bitmask
  byte data[18] = { 0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x08, 0x06, 0x30, 0x45, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F };
  // data array is a valid trame, only byte to be chnaged will be updated.

  byte i;

#ifdef HVAC_MITSUBISHI_DEBUG
  Serial.println("Packet to send: ");
  for (i = 0; i < 18; i++) {
    Serial.print("_");
    Serial.print(data[i], HEX);
  }
  Serial.println(".");
#endif

  // Byte 6 - On / Off
  if (OFF) {
    data[5] = (byte) 0x0; // Turn OFF HVAC
  } else {
    data[5] = (byte) 0x20; // Tuen ON HVAC
  }

  // Byte 7 - Mode
  switch (HVAC_Mode)
  {
    case HVAC_HOT:   data[6] = (byte) 0x08; break;
    case HVAC_COLD:  data[6] = (byte) 0x18; break;
    case HVAC_DRY:   data[6] = (byte) 0x10; break;
    case HVAC_AUTO:  data[6] = (byte) 0x20; break;
    default: break;
  }

  // Byte 8 - Temperature
  // Check Min Max For Hot Mode
  byte Temp;
  if (HVAC_Temp > 31) { Temp = 31;}
  else if (HVAC_Temp < 16) { Temp = 16; } 
  else { Temp = HVAC_Temp; };
  data[7] = (byte) Temp - 16;

  // Byte 10 - FAN / VANNE
  switch (HVAC_FanMode)
  {
    case FAN_SPEED_1:       data[9] = (byte) B00000001; break;
    case FAN_SPEED_2:       data[9] = (byte) B00000010; break;
    case FAN_SPEED_3:       data[9] = (byte) B00000011; break;
    case FAN_SPEED_4:       data[9] = (byte) B00000100; break;
    case FAN_SPEED_5:       data[9] = (byte) B00000100; break; //No FAN speed 5 for MITSUBISHI so it is consider as Speed 4
    case FAN_SPEED_AUTO:    data[9] = (byte) B10000000; break;
    case FAN_SPEED_SILENT:  data[9] = (byte) B00000101; break;
    default: break;
  }

  switch (HVAC_VanneMode)
  {
    case VANNE_AUTO:        data[9] = (byte) data[9] | B01000000; break;
    case VANNE_H1:          data[9] = (byte) data[9] | B01001000; break;
    case VANNE_H2:          data[9] = (byte) data[9] | B01010000; break;
    case VANNE_H3:          data[9] = (byte) data[9] | B01011000; break;
    case VANNE_H4:          data[9] = (byte) data[9] | B01100000; break;
    case VANNE_H5:          data[9] = (byte) data[9] | B01101000; break;
    case VANNE_AUTO_MOVE:   data[9] = (byte) data[9] | B01111000; break;
    default: break;
  }

  // Byte 18 - CRC
  data[17] = 0;
  for (i = 0; i < 17; i++) {
    data[17] = (byte) data[i] + data[17];  // CRC is a simple bits addition
  }

#ifdef HVAC_MITSUBISHI_DEBUG
  Serial.println("Packet to send: ");
  for (i = 0; i < 18; i++) {
    Serial.print("_"); Serial.print(data[i], HEX);
  }
  Serial.println(".");
  for (i = 0; i < 18; i++) {
    Serial.print(data[i], BIN); Serial.print(" ");
  }
  Serial.println(".");
#endif

  enableIROut(38);  // 38khz
  space(0);
  for (int j = 0; j < 2; j++) {  // For Mitsubishi IR protocol we have to send two time the packet data
    // Header for the Packet
    mark(HVAC_MITSUBISHI_HDR_MARK);
    space(HVAC_MITSUBISHI_HDR_SPACE);
    for (i = 0; i < 18; i++) {
      // Send all Bits from Byte Data in Reverse Order
      for (mask = 00000001; mask > 0; mask <<= 1) { //iterate through bit mask
        if (data[i] & mask) { // Bit ONE
          mark(HVAC_MITSUBISHI_BIT_MARK);
          space(HVAC_MITSUBISHI_ONE_SPACE);
        }
        else { // Bit ZERO
          mark(HVAC_MITSUBISHI_BIT_MARK);
          space(HVAC_MISTUBISHI_ZERO_SPACE);
        }
        //Next bits
      }
    }
    // End of Packet and retransmission of the Packet
    mark(HVAC_MITSUBISHI_RPT_MARK);
    space(HVAC_MITSUBISHI_RPT_SPACE);
    space(0); // Just to be sure
  }
}

/****************************************************************************
/* enableIROut : Set global Variable for Frequency IR Emission
/***************************************************************************/ 
void enableIROut(int khz) {
  // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
  halfPeriodicTime = 500/khz; // T = 1/f but we need T/2 in microsecond and f is in kHz
}

/****************************************************************************
/* mark ( int time) 
/***************************************************************************/ 
void mark(int time) {
  // Sends an IR mark for the specified number of microseconds.
  // The mark output is modulated at the PWM frequency.
  long beginning = micros();
  while(micros() - beginning < time){
    digitalWrite(IRpin, HIGH);
    delayMicroseconds(halfPeriodicTime);
    digitalWrite(IRpin, LOW);
    delayMicroseconds(halfPeriodicTime); //38 kHz -> T = 26.31 microsec (periodic time), half of it is 13
  }
}

/****************************************************************************
/* space ( int time) 
/***************************************************************************/ 
/* Leave pin off for time (given in microseconds) */
void space(int time) {
  // Sends an IR space for the specified number of microseconds.
  // A space is no output, so the PWM output is disabled.
  digitalWrite(IRpin, LOW);
  if (time > 0) delayMicroseconds(time);
}

/****************************************************************************
/* sendRaw (unsigned int buf[], int len, int hz)
/***************************************************************************/ 
void sendRaw (unsigned int buf[], int len, int hz)
{
  enableIROut(hz);
  for (int i = 0; i < len; i++) {
    if (i & 1) {
      space(buf[i]);
    } 
    else {
      mark(buf[i]);
    }
  }
  space(0); // Just to be sure
}

void loop(){
  dnsServer.processNextRequest();
  if(HVAC_Mode_received && HVAC_Temp_received && HVAC_Fan_Mode_received && HVAC_Vane_Mode_received && HVAC_On_received){
      Serial.println("HVAC Settings Received:");
      Serial.println(HVAC_Mode);
      if (HVAC_Mode == "HVAC_AUTO") HvacMode_1 = HVAC_AUTO;
      if (HVAC_Mode == "HVAC_HOT") HvacMode_1 = HVAC_HOT;
      if (HVAC_Mode == "HVAC_COLD") HvacMode_1 = HVAC_COLD;
      if (HVAC_Mode == "HVAC_DRY") HvacMode_1 = HVAC_DRY;
      Serial.println(HVAC_Temp);
      HVAC_Temp_i = HVAC_Temp.toInt();
      Serial.println(HVAC_Fan_Mode);
      if (HVAC_Fan_Mode == "FAN_SPEED_AUTO") HvacFanMode_1 = FAN_SPEED_AUTO;
      if (HVAC_Fan_Mode == "FAN_SPEED_1") HvacFanMode_1 = FAN_SPEED_1;
      if (HVAC_Fan_Mode == "FAN_SPEED_2") HvacFanMode_1 = FAN_SPEED_2;
      if (HVAC_Fan_Mode == "FAN_SPEED_3") HvacFanMode_1 = FAN_SPEED_3;
      if (HVAC_Fan_Mode == "FAN_SPEED_4") HvacFanMode_1 = FAN_SPEED_4;
      if (HVAC_Fan_Mode == "FAN_SPEED_5") HvacFanMode_1 = FAN_SPEED_5;
      if (HVAC_Fan_Mode == "FAN_SPEED_SILENT") HvacFanMode_1 = FAN_SPEED_SILENT;
      Serial.println(HVAC_Vane_Mode);
      if (HVAC_Vane_Mode == "VANE_AUTO") HvacVanneMode_1 = VANNE_AUTO_MOVE;
      if (HVAC_Vane_Mode == "VANE_H1") HvacVanneMode_1 = VANNE_H1;
      if (HVAC_Vane_Mode == "VANE_H2") HvacVanneMode_1 = VANNE_H2;
      if (HVAC_Vane_Mode == "VANE_H3") HvacVanneMode_1 = VANNE_H3;
      if (HVAC_Vane_Mode == "VANE_H4") HvacVanneMode_1 = VANNE_H4;
      if (HVAC_Vane_Mode == "VANE_H5") HvacVanneMode_1 = VANNE_H5;
      if (HVAC_Vane_Mode == "VANE_AUTO_MOVE") HvacVanneMode_1 = VANNE_AUTO;
      Serial.println(HVAC_On);
      if (HVAC_On == "On") HVAC_On_b = true;
      if (HVAC_On == "Off") HVAC_On_b = false;
      HVAC_Mode_received = false;
      HVAC_Temp_received = false;
      HVAC_Fan_Mode_received = false;
      HVAC_Vane_Mode_received = false;
      HVAC_On_received = false;
      
      digitalWrite(ONBOARD_LED, HIGH);
      sendHvacMitsubishi(HvacMode_1, HVAC_Temp_i, FAN_SPEED_AUTO, HvacVanneMode_1, HVAC_On_b);
      digitalWrite(ONBOARD_LED, LOW);
    }
    microphoneValue = analogRead(microphonePin);
    microphoneAverage = microphoneAverage + microphoneValue;
    microphoneAverageCount++;
    if (microphoneAverageCount == microphoneAverageLimit) {
      microphoneAverageCount = 0;
      microphoneAverage = microphoneAverage / microphoneAverageLimit;
      Serial.println(microphoneAverage);
    }
}
