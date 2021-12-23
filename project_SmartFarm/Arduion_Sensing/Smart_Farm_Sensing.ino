#include <Adafruit_ESP8266.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Emotion_Farm.h>
#include <DHT.h>

//센서 핀 설정
#define cdsPin A1                   // 조도센서 모듈 핀
#define soilmoisturePin A0          // 토양수분센서 핀
#define DHTPIN 4                    // 온습도센서 모듈 핀 1
#define DHTPIN2 7                   // 온습도센서 모듈 핀 2
#define DHTTYPE DHT11               // 온습도 센서타입 설정
#define TIMEOUT 10000               // TimeOut 시간 설정
#define LCD_X 16                    // LCD X축
#define LCD_Y 2                     // LCD Y축
#define WIFI_TX 2                   // WIFI TX
#define WIFI_RX 3                   // WIFI RX

//와이파이 통신을 위해 객체생성?
SoftwareSerial mySerial(WIFI_TX,WIFI_RX);

//LCD 및 온습도 센서 객체 생성 
LiquidCrystal_I2C lcd(0x27, LCD_X, LCD_Y);
DHT dht(DHTPIN, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

//와이파이 ID 및 Password 설정
const String ssid = "PI_AP";
const String password = "12345678";

//서버 IP 설정 및 Port 설정
const String ip = "192.168.1.1";
const String port = "80";

//LCD에 온습도 출력을 변환시키기 위해 선언
uint64_t startPrint = millis();

//wifi connection
void connectWifi() {
  
  while (1){
    //Wifi 초기화
    Serial.println(F("\r\n---------- AT+RST ----------\r\n"));
    mySerial.println(F("AT+RST"));
    responseSerial("IP");
  
    
    //모드 설정(1로 설정)
    Serial.println(F("\r\n---------- AT+CWMODE ----------\r\n"));
    mySerial.println(F("AT+CWMODE=1"));
    responseSerial("OK");

    while(1){
      //wifi 연결
      Serial.println(F("\r\n---------- AT+CWJAP_DEF ----------\r\n"));
      mySerial.print(F("AT+CWJAP_DEF=\""));
      mySerial.print(ssid);
      mySerial.print(F("\",\""));
      mySerial.print(password);
      mySerial.println(F("\""));
      responseSerial("OK");
      
      //연결된 ip 출력
      Serial.println(F("\r\n---------- AT+CIFSR ----------\r\n"));
      mySerial.println(F("AT+CIFSR"));
      responseSerial("OK");

      //2 반환시 ip할당 성공, 5 반환시 ip할당 실패로 연결 재시도
      if(nowStatus() == 5){
        continue;
      }
      else {
        return;
      }
    }
  }
}
// 현재 상태 출력
uint8_t nowStatus(){
  uint8_t connectionStatus;

  Serial.println(F("\r\n---------- AT+CIPSTATUS ----------\r\n"));
  mySerial.println(F("AT+CIPSTATUS"));
  // ':' 가 들어오면 뒤에 숫자 값을 받아 리턴
  if(mySerial.find(":")){
    connectionStatus = mySerial.parseInt();
  }

  return connectionStatus;
}

//Sensor data send
void dataSend(String url){
  uint8_t startStop = 1;

  while(1){
    Serial.print(F("\r\n----- AT+CIPMUX -----\r\n"));
    mySerial.println(F("AT+CIPMUX=0"));
    responseSerial("OK");

    while (1){
      Serial.print(F("\r\n----- AT+CIPSTART -----\r\n"));
      mySerial.print(F("AT+CIPSTART=\"TCP\",\""));
      mySerial.print(ip);
      mySerial.print(F("\","));
      mySerial.println(port);
      responseSerial("OK");

      //TCP가 연결 되었으면 while문 break; 아니면 다시 연결 시도
      if(nowStatus() == 3){
        break;
      }
      else {
        continue;
      }
    }
  
    Serial.print(F("\r\n----- AT+CIPSEND -----\r\n"));
    mySerial.print(F("AT+CIPSEND="));
    mySerial.println(url.length());
    responseSerial("OK\r\n>");
  
    Serial.print(F("\r\n---------- GET ----------\r\n"));
    mySerial.println(url);
    responseSerial("CLOSED");

    startStop = nowStatus();

    //5: ip할당 실패, 2: ip할당 성공했으나, 중간에 끊긴것임으로 함수 종료
    //3, send 실패 다시 시도, 4 : 정상 send 후 종료
    if(startStop == 5){
      return;
    }
    else if(startStop == 2) {
      return;
    }
    else if(startStop == 3){
      continue;
    }
    else {
      break;
    }
  }
}

//Serial 출력 및 Command 정상으로 들어갔나 확인
void responseSerial(char* keyword){
  uint8_t cutChar = 0;
  uint8_t keywordLength = 0;
  unsigned long startTime;
  int i;

  for(i=0; keyword[i] != NULL; i++){
    keywordLength++;
  }

  startTime = millis();
  while((millis() - startTime) < TIMEOUT){
    if(mySerial.available()){
      char ch = mySerial.read();
      Serial.write(ch);

      if(ch == keyword[cutChar]){
        cutChar++;
        if(cutChar == keywordLength){
          return;
        }
      }
      else {
        cutChar = 0;
      }
    }
  }
  return;
}

//현재 온도 출력
void printTemp(int8_t temp1, int8_t temp2){
  //temp 출력을 위한 문자열
  char temp_print[5];

  lcd.setCursor(1,1);
  lcd.write(0);
  sprintf(temp_print, "%02d", temp1);
  lcd.setCursor(3,1);
  lcd.print(temp_print);
  lcd.write(1);

  lcd.setCursor(7,1);
  lcd.write(0);
  sprintf(temp_print, "%02d", temp2);
  lcd.setCursor(9,1);
  lcd.print(temp_print);
  lcd.write(1);

}

//현재 습도 출력
void printHumi(uint8_t humi1, uint8_t humi2){
  //humi 출력을 위한 문자열
  char humi_print[5];

  lcd.setCursor(1,1);
  lcd.write(2);
  sprintf(humi_print, "%02d", humi1);
  lcd.setCursor(3,1);
  lcd.print(humi_print);
  lcd.print("%");
  
  lcd.setCursor(7,1);
  lcd.write(2);
  sprintf(humi_print, "%02d", humi2);
  lcd.setCursor(9,1);
  lcd.print(humi_print);
  lcd.print("%");
  
}

//현재 토양수분 출력
void printMoist(uint16_t moist){
  //moist 수분 출력을 위한 문자열
  char moist_print[5];
  
  //토양 수분 출력
  lcd.setCursor(1,0);
  lcd.print("MOIST:");
  lcd.setCursor(8,0);
  sprintf(moist_print, "%03d", moist);
  lcd.print(moist_print);
  lcd.setCursor(11,0);
  lcd.print("%");

  //토양습도 값에 따른 LCD에 이모티콘 띄우기
  if(moist >= 0 && moist < 30){
    lcd.setCursor(13,0);
    lcd.write(3);
    lcd.setCursor(14,0);
    lcd.write(4);
  }
  else if(moist >= 30 && moist < 70){
    lcd.setCursor(13,0);
    lcd.print(" ");
    lcd.setCursor(14,0);
    lcd.write(5);
  }
  else if(moist >= 70){
    lcd.setCursor(13,0);
    lcd.write(3);
    lcd.setCursor(14,0);
    lcd.write(6);
  }
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  Serial.println(F("----- STRAT -----"));

  //LCD 초기화
  lcd.begin();

  //wifi connection
  connectWifi();
 
  pinMode(cdsPin, INPUT);                   //조도 센서
  pinMode(soilmoisturePin, INPUT);          //토양수분센서

  // 라이브러리로 추가한 특수 문자 및 이모티콘 추가
  lcd.createChar(0, temp);
  lcd.createChar(1, C);
  lcd.createChar(2, humi);  
  lcd.createChar(3, Qmark);
  lcd.createChar(4, water);
  lcd.createChar(5, good);
  lcd.createChar(6, wind);
}

void loop() {
  //센서값 측정
  uint16_t cdsValue = analogRead(cdsPin);                                  // 조도센서 값 측정: 0(밝음) ~ 1023(어두움)
  uint16_t soilmoistureValue = analogRead(soilmoisturePin);                // 토양수분 값 측정: 0(습함) ~ 1023(건조)
  uint8_t moist_per = map(soilmoistureValue, 170, 1023, 100, 0);           // 센서 값을 퍼센트로 변경
  //공기 중 습도 값 측정
  uint8_t humiValue1 = (uint8_t)dht.readHumidity();
  uint8_t humiValue2 = (uint8_t)dht2.readHumidity();
  //공기 중 온도 값 측정
  int8_t tempValue1 = (int8_t)dht.readTemperature();
  int8_t tempValue2 = (int8_t)dht2.readTemperature();
  // 데이터 전송할 url
  String url;
  uint64_t endPrint;

  //LCD에 현재 온습도 상태 표시
  printMoist(moist_per);

  endPrint = millis();
  //5초 동안 LCD에 온도 표시
  if((endPrint - startPrint) <= 5000){
    printTemp(tempValue1, tempValue2);
  }
  //4초 동안 LCD에 습도 표시
  else if((endPrint - startPrint) <= 9000){
    printHumi(humiValue1, humiValue2);
  }
  //습도 표시 이후에 온도 표기를 위해 startPrint에 endPrint 시간 data 저장
  else {
    startPrint = endPrint;
  }

  //Sensing data 전송
  ///////////////////////////////////////////////////////////////////////////////
  url = "GET /"+String(moist_per)+"/"+tempValue1+"/"+tempValue2+"/"+humiValue1+
      "/"+humiValue2+"/"+String(cdsValue)+" HTTP/1.1\r\nHost: " + ip +"\r\n\r\n";
  dataSend(url);
  ///////////////////////////////////////////////////////////////////////////////

  //ip 할당 실패시(wifi 끊겼을시) 다시 연결 시도
  if(nowStatus() == 5){
    connectWifi();
  }
}
