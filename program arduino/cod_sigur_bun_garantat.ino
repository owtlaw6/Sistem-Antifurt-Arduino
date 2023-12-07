#include<SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
SoftwareSerial comm(8, 9); //setting Tx and Rx pins
#include <Adafruit_Fingerprint.h>

//fingerprint sensor
#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial mySerial(13, 11);

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white

#endif

//lcd
LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte green = 10; //btn

int redLed = A0; //led
int greenLed = A1; //led
String stringToSend="";
String nume;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial); //fingerprint sensor 

String server = ""; //variable for sending data to webpage
boolean No_IP = false; //variable to check for ip Address
String IP = ""; //variable to store ip Address
void setup()
{
  //lcd init
  lcd.init();
  lcd.backlight();
  lcd.clear();

  //leds + buttons
  pinMode(green, INPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, LOW);
  Serial.begin(9600); 

  //fingerprint sensor
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
  comm.begin(115200);
  wifi_init();
  Serial.println("System Ready..");
}

void loop()
{
    while (comm.available())
    {
      if (comm.find("0,CONNECT"))
      {
        Serial.println("Starting");
        sendToServer();
        Serial.println("Finished");
        delay(1000);
      }
    }
  
  if(digitalRead(green) == LOW){
    finger.begin(57600);
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, LOW);
    lcd.clear();
    lcd.print("CHECKING");
    lcd.setCursor(4,1);
    lcd.print("FINGERPRINT");
    for(int i=0; i<50; i++){
      int b;
      b = getFingerprintID();
      if(b == 2) nume = "Fabian";
      if(!b){
        digitalWrite(greenLed, HIGH);
        digitalWrite(redLed, LOW);
        lcd.clear();
        lcd.print(nume);
        lcd.setCursor(4,1);
        lcd.print("ENTER!");
        stringToSend += "<h2>" + nume +  " entered!</h2>";
        comm.begin(115200);
        wifi_reinit();
        digitalWrite(greenLed, LOW);
        lcd.clear();
        return;
      }
    }
    digitalWrite(redLed, HIGH);
    digitalWrite(greenLed, LOW);
    lcd.clear();
    lcd.print("FINGERPRINT");
    lcd.setCursor(4,1);
    lcd.print("NOT OKAY!");
    stringToSend += "<h2>Attempt failed!</h2>";
    delay(1550);
    digitalWrite(redLed, LOW);
    comm.begin(115200);
    wifi_reinit();
    lcd.clear();
  }
}


void findIp(int time1) //check for the availability of IP Address
{
  int time2 = millis();
  while (time2 + time1 > millis())
  {
    while (comm.available() > 0)
    {
      if (comm.find("IP has been read"))
      {
        No_IP = true;
      }
    }
  }
}

void showIP()//Display the IP Address
{
  IP = "";
  char ch = 0;
  while (1)
  {
    comm.println("AT+CIFSR");
    while (comm.available() > 0)
    {
      if (comm.find("STAIP,"))
      {
        delay(1000);
        Serial.print("IP Address:");
        while (comm.available() > 0)
        {
          ch = comm.read();
          if (ch == '+')
            break;
          IP += ch;
        }
      }
      if (ch == '+')
        break;
    }
    if (ch == '+')
      break;
    delay(1000);
  }
  Serial.print(IP);
  Serial.print("Port:");
  Serial.println(80);
}

void establishConnection(String command, int timeOut) //Define the process for sending AT commands to module
{
  int q = 0;
  while (1)
  {
    Serial.println(command);
    comm.println(command);
    while (comm.available())
    {
      if (comm.find("OK"))
        q = 8;
    }
    delay(timeOut);
    if (q > 5)
      break;
    q++;
  }
  if (q == 8)
    Serial.println("OK");
  else
    Serial.println("Error");
}

void wifi_reinit(){
  establishConnection("AT+CWJAP=\"Orange-ad53\",\"AFNHr8uc\"", 7000); //provide your WiFi username and password here
  
  establishConnection("AT+CIPMUX=1", 100);
  establishConnection("AT+CIPSERVER=1,80", 100);
}

void wifi_init() //send AT commands to module
{
  establishConnection("AT", 100);
  delay(1000);
  establishConnection("AT+CWMODE=3", 100);
  delay(1000);
  establishConnection("AT+CWQAP", 100);
  delay(1000);
  establishConnection("AT+RST", 5000);
  delay(1000);
  findIp(5000);
  if (!No_IP)
  {
    Serial.println("Connecting Wifi....");
    establishConnection("AT+CWJAP=\"Orange-ad53\",\"AFNHr8uc\"", 7000); //provide your WiFi username and password here
  }
  else
  {
  }
  Serial.println("Wifi Connected");
  showIP();
  establishConnection("AT+CIPMUX=1", 100);
  establishConnection("AT+CIPSERVER=1,80", 100);
}

void sendData(String server1)//send data to module
{
  int p = 0;
  while (1)
  {
    unsigned int l = server1.length();
    Serial.print("AT+CIPSEND=0,");
    comm.print("AT+CIPSEND=0,");
    Serial.println(l + 2);
    comm.println(l + 2);
    delay(100);
    Serial.println(server1);
    comm.println(server1);
    while (comm.available())
    {
      //Serial.print(Serial.read());
      if (comm.find("OK"))
      {
        p = 11;
        break;
      }
    }
    if (p == 11)
      break;
    delay(100);
  }
}

void sendToServer()//send data to webpage
{
  server = "<h1>Lock System!</h1>";
  sendData(server);
  sendData("<h2>Here you can see who entered:</h2>");
  sendData(stringToSend);
  delay(1000);
  comm.println("AT+CIPCLOSE=0");
}

//fingerprint sensor function to check fingerprint
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  //return finger.fingerID;
  return 0;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}