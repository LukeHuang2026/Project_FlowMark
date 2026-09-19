#include <SoftwareSerial.h>
#include <avr/wdt.h>

#define accuracy 50
#define outlierCutOff 0.1  //decimal, 0-1

#define calibratePH 10  //Put in data for calibration!!!
#define calibrateEC 10
#define calibrateUV 10
#define calibrateIR 10

#define sensorTURB 16
#define sensorDOM 17
#define sensorPH 14
#define sensorEC 15
#define toggleCellular 10
#define toggleUVLED 3
#define Switch 2
#define button 4
#define toggleSignalLED 7

#define timeout 20000
#define serverURL "https://poking-tremble-affirm.ngrok-free.dev"


SoftwareSerial mySerial(12, 11);

String input;
long currentTime;
long startTime;
int offsetIR;
int offsetUV;
int offsetPH;
int offsetEC;
int loopCount;
unsigned long ellapsedTime;
int cycleDelay;
bool isOn = false;

String readLine() {
  String line = "";
  while (mySerial.available()) {
    char c = mySerial.read();
    if (c == '\n') {
      return line;
    } else if (c != '\r') {
      line += c;
    }
  }
  return "";
}

void shutDown1() {
  digitalWrite(10, LOW);
  wdt_enable(WDTO_1S);
  while (1) { delay(100); }
}

void shutDown2() {
  mySerial.flush();
  mySerial.println(F("AT+CPOF"));
  Serial.println(F("AT+CPOF"));
  flashLong(5);
  digitalWrite(10, LOW);
  wdt_enable(WDTO_1S);
  while (1) { delay(100); }
}

void endProgramme() {
  if (!digitalRead(button)) {
    delay(100);
    if (!digitalRead(button)) {
      sendHTTP("Done");
      Serial.println(F("-----------------Done------------------"));
      while (1) {
        delay(100);
        endProgramme();
      }
    }
  }
  if (digitalRead(Switch) == 1 && isOn) {
    isOn = false;
    Serial.println(F("OFF"));
    shutDown2();
  }
}

bool timeOut() {
  int currentTime = millis() - startTime;
  if (currentTime > timeout) {
    Serial.println(F("Timed out"));
    return true;
  }
  return false;
}

String commandAT(String command, String keyWord) {
  startTime = millis();
  mySerial.println(command);
  while (1) {
    if (timeOut()) {
      Serial.println(F("CommandAT response error"));
      while (1) {
        delay(100);
        endProgramme();
      }
    }
    if (mySerial.available() > 0) {
      input = mySerial.readString();
      Serial.println("A7672SA says: " + String(input));
      if (input.indexOf(keyWord) != -1) {
        Serial.println(F("CommandAT successful"));
        return input;
      }
    }
  }
}

bool successAT(String input, String keyWord) {
  if (input.indexOf(keyWord) != -1) {
    Serial.println(F("CommandAT successful 2"));
    return true;
  } else {
    Serial.println(F("CommandAT response error2"));
    return false;
  }
}

bool sendHTTP(String payload) {
  String payloadLength = String(payload.length());
  String temp5 = "AT+HTTPDATA=" + payloadLength + ',' + "15000";
  while (1) {
    Serial.println(F("AT+HTTPDATA"));
    if (successAT(commandAT(temp5, "DOWNLOAD"), "DOWNLOAD")) {
      Serial.println(F("DOWNLOAD"));
      Serial.println(F("Payload"));
      if (successAT(commandAT(payload, "OK"), "OK")) {
        Serial.println(F("OK"));
        Serial.println(F("AT+HTTPACTION=1"));
        if (successAT(commandAT("AT+HTTPACTION=1", "OK"), "OK")) {
          Serial.println(F("HTTP SENT"));
          return true;
        } else {
          Serial.println(F("AT+HTTPACTION failed"));
          while (1) {
            delay(100);
            endProgramme();
          }
        }
      } else {
        Serial.println(F("payload error"));
        while (1) {
          delay(100);
          endProgramme();
        }
      }
    } else {
      Serial.println(F("AT+HTTPDATA failed"));
      while (1) {
        delay(100);
        endProgramme();
      }
    }
  }
}

String excerptSection(String input, char startChar, char endChar, int occurance) {
  int startIndex = -1;
  for (int i = 0; i < occurance; i++) {
    startIndex = input.indexOf(startChar, startIndex + 1);
    if (startIndex == -1) {
      Serial.println(F("Excerpt failed"));
      return "-1";
    }
  }
  int endIndex = input.indexOf(endChar, startIndex + 1);
  if (endIndex <= startIndex) {
    Serial.println(F("Excerpt failed 2"));
    return "-1";
  }
  return input.substring(startIndex + 1, endIndex);
}

String latFixGPS() {
  Serial.println("latFixGPS");
  delay(100);
  String temp7 = commandAT("AT+CGPSINFO", ",");
  Serial.println("Raw fix: " + temp7 + "///");
  temp7.replace(" ", "");
  String lat = excerptSection(temp7, ':', ',', 1);
  String latNW = excerptSection(temp7, ',', ',', 1);
  Serial.println(lat + latNW);
  return lat + latNW;
}

String LONFixGPS() {
  Serial.println("LonFixGPS");
  delay(100);
  String temp8 = commandAT("AT+CGPSINFO", ",");
  Serial.println("Raw fix: " + temp8 + "///");
  String LON = excerptSection(temp8, ',', ',', 2);
  String LONEW = excerptSection(temp8, ',', ',', 3);
  Serial.println(LON + LONEW);
  return LON + LONEW;
}

float sensorMeasure(int sensorPin, String sensorType) {
  float currMeasurement;
  float masterAverage = float(analogRead(sensorPin));
  for (int i = 0; i <= accuracy; i++) {
    currMeasurement = analogRead(sensorPin);
    //if (i == 0) {
    //masterAverage = currMeasurement;
    //}
    //else if (masterAverage / (currMeasurement + 1) > 1 - outlierCutOff && masterAverage / (currMeasurement + 1) < 1 + outlierCutOff) {
    masterAverage = ((masterAverage * i) + currMeasurement) / (i + 1);
    delay(10);
    //}
    //else {
    //Serial.println(F("Outlier detected, number " + String(i));
    //}
  }
  Serial.println("Done " + sensorType + ", " + masterAverage);
  return masterAverage;
}

void setupCellular() {
  //Connection
  mySerial.println("AT");
  while (1) {
    if (mySerial.available() != 0) {
      mySerial.readString();
      break;
    }
  }

  delay(500);

  while (1) {
    if (successAT(commandAT("AT", "OK"), "OK")) {
      Serial.println(F("AT: OK"));
      break;
    } else {
      Serial.println(F("AT: error"));
      while (1) {
        delay(100);
        endProgramme();
      }
    }
  }

  //SIM
  while (1) {
    Serial.println(F("AT+CPIN?"));
    if (successAT(commandAT("AT+CPIN?", "READY"), "READY")) {
      Serial.println(F("AT+CPIN?: READY"));
      break;
    } else {
      Serial.println(F("AT+CPIN?: NO SIM"));
      while (1) {
        delay(100);
        endProgramme();
      }
    }
  }

  //Signal strength
  while (1) {
    Serial.println(F("AT+CSQ"));
    String Temp1 = commandAT("AT+CSQ", ":");
    if (Temp1 == "-1") {
      Serial.println(F("AT+CSQ: ERROR"));
    }
    int signalStrength = excerptSection(Temp1, ':', ',', 1).toInt();
    if (signalStrength < 11) {
      Serial.println(F("signalStrength < 11"));
      while (1) {
        delay(100);
        endProgramme();
      }
    }
    break;
  }
}

void setupNetwork() {
  //Open network
  while (1) {
    Serial.println(F("AT+CGATT"));
    if (successAT(commandAT("AT+CGATT=1", "OK"), "OK")) {
      Serial.println(F("AT+CGATT: OK"));
      break;
    } else {
      Serial.println(F("AT+CGATT: ERROR"));
      while (1) {
        delay(100);
        endProgramme();
      }
    }
  }

  //HTTP setup
  while (1) {
    Serial.println(F("AT+HTTPINIT"));
    if (successAT(commandAT("AT+HTTPINIT", "OK"), "OK")) {
      Serial.println(F("AT+HTTPINIT: OK"));
      break;
    } else {
      Serial.println(F("AT+HTTPINIT: ERROR"));
      while (1) {
        delay(100);
        endProgramme();
      }
    }
  }

  //Setup URL
  while (1) {
    Serial.println(F("AT+HTTPPARA"));
    if (successAT(commandAT((String("AT+HTTPPARA=\"URL\",") + serverURL), "OK"), "OK")) {
      Serial.println(F("AT+HTTPPARA: OK"));
      break;
    } else {
      Serial.println(F("AT+HTTPPARA: ERROR"));
      while (1) {
        delay(100);
        endProgramme();
      }
    }
  }

  //Test server conenction
  while (1) {
    Serial.println(F("HTTP sennt OK?"));
    if (sendHTTP("OK?")) {
      if (successAT(commandAT("", "200"), "200")) {
        Serial.println(F("HTTP Server conencted"));
        break;
      } else {
        Serial.println(F("ERROR: 5xx"));
        Serial.println(F("HTTP Server Error"));
      }
    } else {
      while (1) {
        delay(100);
        endProgramme();
      }
    }
  }
}

void setupGNSS() {
  //Power on GNSS
  while (1) {
    Serial.println(F("AT+GNSSPWR=1"));
    if (successAT(commandAT("AT+CGNSSPWR=1", "READY"), "READY")) {
      Serial.println(F("AT+GNSSPWR=1: READY"));
      break;
    } else {
      Serial.println(F("AT+CGNSSPWR=1: Error"));
      while (1) {
        delay(100);
        endProgramme();
      }
    }
  }

  //GPS quality
  String temp6 = commandAT("AT+CGNSSINFO", ",");
  int countGPS = excerptSection(temp6, ',', ',', 1).toInt();
  if (countGPS == -1) {
    Serial.println(F("AT+CGNSSINFO: Error"));
    while (1) {
      delay(100);
      endProgramme();
    }
  }

  if (countGPS < 4) {
    Serial.println(F("Low satelite count"));
  }
}

void calibrate() {
  while (digitalRead(button));
  offsetPH = calibratePH - analogRead(sensorPH);
  offsetUV = calibrateUV - analogRead(sensorDOM);
  offsetIR = calibrateIR - analogRead(sensorTURB);
  offsetEC = calibrateEC - analogRead(sensorEC);
  while (digitalRead(button));
  sendHTTP("Local water stats PH:" + String(analogRead(sensorPH)) + ", EC:" + String(analogRead(sensorEC)) + ",");
}

void setPinMode() {
  pinMode(sensorTURB, INPUT);
  pinMode(sensorDOM, INPUT);
  pinMode(sensorPH, INPUT);
  pinMode(sensorEC, INPUT);
  pinMode(toggleCellular, OUTPUT);
  pinMode(toggleUVLED, OUTPUT);
  pinMode(toggleSignalLED, OUTPUT);
  pinMode(Switch, INPUT_PULLUP);
  pinMode(button, INPUT_PULLUP);
}

void flash(int flashTimes) {
  for (int i = 0; i < flashTimes; i++) {
    Serial.println("Flash");
    digitalWrite(toggleSignalLED, HIGH);
    delay(500);
    digitalWrite(toggleSignalLED, LOW);
    delay(500);
  }
}

void flashLong(int flashTimes) {
  for (int i = 0; i < flashTimes; i++) {
    Serial.println("Flash");
    digitalWrite(toggleSignalLED, HIGH);
    delay(1000);
    digitalWrite(toggleSignalLED, LOW);
    delay(500);
  }
}

void setup() {
  pinMode(Switch, INPUT_PULLUP);
  pinMode(10, OUTPUT);
  if (digitalRead(Switch) == 1) {
    shutDown1();
  }
  Serial.begin(9600);
  mySerial.begin(115200);
  while (mySerial.available() != 0) {
    mySerial.read();
  }
  flashLong(1);
  digitalWrite(toggleCellular, HIGH);
  digitalWrite(toggleUVLED, LOW);
  isOn = true;
  Serial.println(F("ON"));
  flashLong(1);
  delay(15000);
  mySerial.println(F("AT+IPR=1200"));
  delay(100);
  Serial.println(F("Asked a7672sa about 9600"));
  delay(5000);
  mySerial.begin(1200);
  flash(1);
  setPinMode();
  Serial.println(F("setPinMode done"));
  flash(1);
  setupCellular();
  Serial.println(F("setupCellular done"));
  flash(1);
  setupNetwork();
  Serial.println(F("setupNetwork done"));
  flash(1);
  setupGNSS();
  Serial.println(F("setupGNSS done"));
  flash(1);
  calibrate();
  Serial.println(F("calibrate done"));
  flashLong(2);
  Serial.println(F("Put in measurement delay"));
  sendHTTP("Measurement Delay?");
  startTime = millis();
  while (1) {
    ellapsedTime = millis();
    if (mySerial.available() != 0) {
      Serial.println(mySerial.readString());
      delay(500);
      mySerial.println("AT+HTTPREAD=0,3");
      while (1) {
        if (mySerial.available() != 0 and (mySerial.readString()).indexOf("HTTPREAD") != -1) {
          readLine();
          input = readLine();
          Serial.println("Raw delay: " + input);
          cycleDelay = (input.substring(input.indexOf(':') + 1, input.indexOf(',')).toInt()) * 10000;
          Serial.println("cycleDelay: " + String(cycleDelay));
          cycleDelay = 5000;  //as the recieving isn't working
          break;
        }
      }
    }
    if (ellapsedTime - startTime >= timeout) {
      break;
    }
  }
  flash(3);
}

void loop() {
  Serial.println(F("Main Loop-----"));
  mySerial.begin(112500);
  mySerial.println("AT+IPR=1200");
  delay(100);
  mySerial.begin(1200);
  flash(2);
  digitalWrite(toggleUVLED, HIGH);
  digitalWrite(toggleSignalLED, HIGH);
  mySerial.println("+++");
  delay(1000);
  mySerial.readString();
  sendHTTP("TURB:" + String(sensorMeasure(sensorTURB, "IR") + offsetIR) + "," + "DOM:" + String(sensorMeasure(sensorDOM, "UV") + offsetUV) + "," + "PH:" + String(sensorMeasure(sensorPH, "PH") + offsetPH) + "," + "EC:" + String(sensorMeasure(sensorEC, "EC") + offsetEC) + "," + "LAT:" + latFixGPS() + "," + "LON:" + LONFixGPS() + ",");
  delay(100);
  digitalWrite(toggleUVLED, LOW);
  digitalWrite(toggleSignalLED, LOW);
  loopCount = loopCount + 1;
  Serial.println("Data sent " + String(loopCount) + " times.");
  ellapsedTime = 0;
  startTime = millis();

  Serial.println("Start cycle delay------");
  while (1) {
    ellapsedTime = millis();
    endProgramme();
    if (ellapsedTime - startTime >= cycleDelay) {
      break;
    }
    delay(10);
    Serial.println(ellapsedTime - startTime);
  }
}