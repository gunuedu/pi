#include <Servo.h>
#include <Adafruit_ESP8266.h>

#define FANCONTROL_IN_1 22
#define FANCONTROL_IN_2 24
#define FANCONTROL_SPEED 7

#define LEFT_WINDOW_PIN 8
#define RIGHT_WINDOW_PIN 9

#define HEATER_PIN 31
#define LED_PIN 33
#define PUMP_PIN 35
#define PUMP_LED_PIN_1 37
#define PUMP_LED_PIN_2 39
#define PUMP_LED_PIN_3 41

#define TIMEOUT 10000               // TimeOut 시간 설정
#define WINDOW_ANGLE_MAX 30         // 개폐기 최대 각도
#define WINDOW_ANGLE_MIN 10         // 개폐기 최소 각도

#define FAN_SPEED_MAX 255           // FAN의 최대속도
#define FAN_SPEED_MID 128           // FAN의 중간속도
#define FAN_SPEED_MIN 80            // FAN의 최소속도
#define FAN_SPEED_ZERO 0

#define ON 1                        // On 1 return 
#define OFF 2                       // Off 2 return

Servo leftWindow;
Servo rightWindow;

// 와이파이 ID 및 Password 설정
const String ssid = "PI_AP";
const String password = "12345678";

// 서버 IP 설정 및 Port 설정
const String ip = "192.168.1.1";
const String port = "80";

//Getter, 서버에서 데이터 읽어오기
const uint8_t sensingDataCount = 7; //getTargetTmp 추가
const uint8_t controlDataCount = 6; 
const uint8_t allDataCount = sensingDataCount + controlDataCount;
const String GetterType = "get";
char getterBuffer[60];
//char *getterBuffer;
String stringGetData[allDataCount];
String getDataType[allDataCount] = {"grd","tmp1","tmp2","hum1","hum2","lux","getTargetTmp",
                                    "getManualControl","getFanState","getFanSpeed",
                                    "getLeftWindow","getRightWindow","getHeaterState"};
int16_t getDataValue[allDataCount] = {0,0,0,0,0,0,0,1,0,0,0,0,0};

//Setter, 서버로 데이터 보내기
const uint8_t setterCount = 4;
const String SetterType = "set";
String setDataType[setterCount] = {"setFan","setHeat","setLeftWindow","setRightWindow"};
int16_t setDataValue[setterCount] = {0,0,0,0};

//fan 시작 시간 측정
uint64_t startFanSpeedControl;
uint64_t endFanSpeedControl;
const uint16_t waitFanSpeedControl = 3000;

// 개폐기 초기 열림 각도 : 최대값으로 설정
int8_t leftWindowAngle = WINDOW_ANGLE_MAX;
int8_t rightWindowAngle = WINDOW_ANGLE_MAX;

//목표 온도, 습도, 조도
uint8_t targetTemp; 
uint8_t targetMoist = 40;
uint8_t targetLux = 600; 

//개폐기 시간 측정
uint64_t leftStartWindowTime = millis();
uint64_t leftEndWindowTime = millis();
uint64_t rightStartWindowTime = millis();
uint64_t rightEndWindowTime = millis();

//히터 시간 측정
uint64_t  startHeaterControl = millis();
uint64_t  endHeaterControl = millis();
const uint16_t waitHeaterControl = 1000;

//pump, led 시간 측정 및 wait 시간
uint64_t startPumpTime = millis();
uint64_t endPumpTime = millis(); 
uint64_t startLedTime = millis();
uint64_t endLedTime = millis();

const uint16_t waitPumpTime = 20000;          // 펌프 가동시간
const uint16_t waitPumpLedStandard = 9000;    // led 가동 단위시간
const uint16_t waitPumpLedTime_1 = 4000;      // led1 가동시간
const uint16_t waitPumpLedTime_2 = 2000;      // led2 가동시간
const uint16_t waitPumpLedTime_3 = 6000;      // led3 가동시간

/////////////////////////////////////////////////////////////////////////////////
//wifi connection
void connectWifi() {
  uint8_t connectionStatus;
  
  while (1){
    //Wifi 초기화
    Serial.println(F("\r\n---------- AT+RST ----------\r\n"));
    Serial3.println(F("AT+RST"));
    responseSerial("IP");
  
    
    //모드 설정(1로 설정)
    Serial.println(F("\r\n---------- AT+CWMODE ----------\r\n"));
    Serial3.println(F("AT+CWMODE=1"));
    responseSerial("OK");

    while(1){
      //wifi 연결
      Serial.println(F("\r\n---------- AT+CWJAP_DEF ----------\r\n"));
      Serial3.print(F("AT+CWJAP_DEF=\""));
      Serial3.print(ssid);
      Serial3.print(F("\",\""));
      Serial3.print(password);
      Serial3.println(F("\""));
      responseSerial("OK");
      
      //연결된 ip 출력
      Serial.println(F("\r\n---------- AT+CIFSR ----------\r\n"));
      Serial3.println(F("AT+CIFSR"));
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
/////////////////////////////////////////////////////////////////////////////////
//Getter, Setter data 송수신부
void GetSetData(String GetSetType, String returnType, String url){
  uint8_t startStop = 1;

  while(1){
    Serial.print(F("\r\n----- AT+CIPMUX -----\r\n"));
    Serial3.println(F("AT+CIPMUX=0"));
    responseSerial("OK");

    while (1){
      Serial.print(F("\r\n----- AT+CIPSTART -----\r\n"));
      Serial3.print(F("AT+CIPSTART=\"TCP\",\""));
      Serial3.print(ip);
      Serial3.print(F("\","));
      Serial3.println(port);
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
    Serial3.print(F("AT+CIPSEND="));
    Serial3.println(url.length());
    responseSerial("OK\r\n>");
  
    Serial.print(F("\r\n---------- GET ----------\r\n"));
    
    //Getter가 들어오면 들어온 값을 넣어줘야하기때문에 구분지어 사용
    if(GetSetType == GetterType){
      Serial3.println(url);
      getterDataInput();
    }
    else if(GetSetType == SetterType){
      Serial3.println(url);
      responseSerial("CLOSED");
    }
    
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
/////////////////////////////////////////////////////////////////////////////////
// 현재 상태 출력
uint8_t nowStatus(){
  Serial.print(F("\r\n === Status === \r\n"));
  uint8_t connectionStatus;

  Serial.println(F("\r\n---------- AT+CIPSTATUS ----------\r\n"));
  Serial3.println(F("AT+CIPSTATUS"));
  // 'STATUS:' 가 들어오면 뒤에 숫자 값을 받아 리턴
  if(Serial3.find("STATUS:")){
    connectionStatus = Serial3.parseInt();
  }
  responseSerial("OK");

  return connectionStatus;
}
/////////////////////////////////////////////////////////////////////////////////

void getterDataInput(){
  int i = 0;
  int16_t number;

  while(Serial3.available()){
    String bufferData = Serial3.readStringUntil('@');
    Serial.print(F("\r\n bufferData : "));
    Serial.println(bufferData);
    
    number = bufferData.toInt();

    if(number != 0){
      getDataValue[i] = number;
      i++;
    }

    if(bufferData == "END"){
      return;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////
//Serial 출력 및 Command 정상으로 들어갔나 확인
void responseSerial(String keyword){
  uint8_t cutChar = 0;
  uint8_t keywordLength = keyword.length();
  unsigned long startTime;
  int i;

  startTime = millis();
  while((millis() - startTime) < TIMEOUT){
    if(Serial3.available()){
      char ch = Serial3.read();
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
/////////////////////////////////////////////////////////////////////////////////
// Moter Control
void moterControl(uint8_t moterStatus, uint8_t pin_In_1, uint8_t pin_In_2){
  //정방향 작동시 입력값 1, 정지 2, 기타 역방향
  if(moterStatus == ON){
    Serial.print(F("\r\n === Moter forward === \r\n "));
    digitalWrite(pin_In_1, HIGH);
    digitalWrite(pin_In_2, LOW);
  }
  else if(moterStatus == OFF){
    Serial.print(F("\r\n === Moter off === \r\n "));
    digitalWrite(pin_In_1, LOW);
    digitalWrite(pin_In_2, LOW);
  }
  else {
    Serial.print(F("\r\n === Moter reverse === \r\n"));
    digitalWrite(pin_In_1, LOW);
    digitalWrite(pin_In_2, HIGH);
  }
}
/////////////////////////////////////////////////////////////////////////////////
// LOW 가 on, HIGH가 off
void relayControl(uint8_t pinNumber, uint8_t relayOnOff){
  if(relayOnOff == ON){
    Serial.print(F("\r\n=== Relay On ===\r\n"));
    digitalWrite(pinNumber, LOW);
  }
  else {
    Serial.print(F("\r\n=== Relay Off ===\r\n"));
    digitalWrite(pinNumber, HIGH);
  }
}
/////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  Serial3.begin(9600);

  Serial.println(F("===== START ====="));

  //wifi connection
  connectWifi();

  // Servo Moter 핀 설정
  leftWindow.attach(LEFT_WINDOW_PIN);
  rightWindow.attach(RIGHT_WINDOW_PIN);

  // Fan Moter 핀 설정
  pinMode(FANCONTROL_IN_1, OUTPUT);
  pinMode(FANCONTROL_IN_2, OUTPUT);
  pinMode(FANCONTROL_SPEED, OUTPUT);

  // Relay 핀 설정
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(PUMP_LED_PIN_1, OUTPUT);
  pinMode(PUMP_LED_PIN_2, OUTPUT);
  pinMode(PUMP_LED_PIN_3, OUTPUT);

  // Relay 초기화
  digitalWrite(HEATER_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(PUMP_PIN, HIGH);
  digitalWrite(PUMP_LED_PIN_1, HIGH);
  digitalWrite(PUMP_LED_PIN_2, HIGH);
  digitalWrite(PUMP_LED_PIN_3, HIGH);
}
/////////////////////////////////////////////////////////////////////////////////
void loop() {
  uint8_t manualControl;
  uint8_t oldFanSpeed = 0;
  uint8_t fanState,fanSpeed,LeftControl,RightControl,heater;
  //자동제어시 펌프상태 및 led1, led2, led3 저장변수
  uint8_t pumpState,ledState_1,ledState_2,ledState_3 ;
  // 생장LED 상태 저장변수
  uint8_t ledBarState;
  // 토양습도, 온도1, 온도2, 습도1, 습도2 값 저장받을 변수
  int16_t grdData,tmp1Data,tmp2Data,hum1Data,hum2Data,luxData;
  int i,j;
  String url;

  //센싱 값 입력 받기
  ///////////////////////////////////////////////////////////////////////////////
    url = "GET /getAll HTTP/1.1\r\nHost: " + ip +"\r\n\r\n";
    GetSetData(GetterType, getDataType[i], url);
  ///////////////////////////////////////////////////////////////////////////////

  // 전달 받은 센싱값 및 제어값 가져오기
  for(i=0; i<allDataCount; i++){
    if(getDataType[i] == "grd"){
      grdData = getDataValue[i];
    }
    else if(getDataType[i] == "tmp1"){
      tmp1Data = getDataValue[i];
    }
    else if(getDataType[i] == "tmp2"){
      tmp2Data = getDataValue[i];
    }
    else if(getDataType[i] == "hum1"){
      hum1Data = getDataValue[i];
    }
    else if(getDataType[i] == "hum2"){
      hum2Data = getDataValue[i];
    }
    else if(getDataType[i] == "lux"){
      luxData = getDataValue[i];
      Serial.println(luxData);
    }
    else if(getDataType[i] == "getTargetTmp"){
      targetTemp = getDataValue[i];
    }
    else if(getDataType[i] == "getManualControl"){
      manualControl = getDataValue[i];
    }
    else if(getDataType[i] == "getFanState"){
      fanState = getDataValue[i];
    }
    else if(getDataType[i] == "getFanSpeed"){
      fanSpeed = getDataValue[i];
    }
    else if(getDataType[i] == "getLeftWindow"){
      LeftControl = getDataValue[i];

    }
    else if(getDataType[i] == "getRightWindow"){
      RightControl = getDataValue[i];
    }
    else if(getDataType[i] == "getHeaterState"){
      heater = getDataValue[i];
    }
    else {
      Serial.print(F("\r\n\r\n === err === \r\n\r\n"));
    }
  }
/////////////////////////////////////////////////////////////////////////////////
  //수동 제어
  if(manualControl == OFF){
    //Fan Control
    if(fanState == OFF){
      //fan 정지
      if(digitalRead(FANCONTROL_IN_1) == LOW){
        //현재 fan 정지 상태
        Serial.print(F("\r\n\r\n === 이미 정지 === \r\n\r\n"));
      }
      else {
        //가동중이면 정지
        moterControl(fanState, FANCONTROL_IN_1, FANCONTROL_IN_2);
      }
    }
    else if(fanState == ON){
      //fan 가동
      if(digitalRead(FANCONTROL_IN_1) == HIGH){
        //가동중 속도 조절
        if(oldFanSpeed != fanSpeed){
          if((endFanSpeedControl - startFanSpeedControl) > waitFanSpeedControl){
            if(fanSpeed < FAN_SPEED_MIN){
              //Fan Speed 최소값 설정(80)
              fanSpeed = FAN_SPEED_MIN;
              Serial.print(F("\r\n\r\n === Speed MIN === \r\n\r\n"));
            }
            else if (fanSpeed > FAN_SPEED_MAX){
              fanSpeed = FAN_SPEED_MAX;
              Serial.print(F("\r\n\r\n === Speed MAX === \r\n\r\n"));
            }
            else {
              Serial.print(F("\r\n\r\n === Speed START === \r\n\r\n"));
            }
            analogWrite(FANCONTROL_SPEED, fanSpeed);
          }
          else {
            endFanSpeedControl = millis();
          }
        }
        else {
          Serial.print(F("\r\n\r\n === 이미 가동 === \r\n\r\n"));
        }
      }
      else {
        //정지 -> 가동
        moterControl(fanState, FANCONTROL_IN_1, FANCONTROL_IN_2);
        analogWrite(FANCONTROL_SPEED, FAN_SPEED_MAX);
        oldFanSpeed = FAN_SPEED_MAX;
        startFanSpeedControl = millis();
        endFanSpeedControl = millis();
      }
    }
/////////////////////////////////////////////////////////////////////////////////
    // Left Window 개폐
    if(LeftControl == OFF){
      //close window control
      if(leftWindowAngle == WINDOW_ANGLE_MIN){
        Serial.print(F("\r\n\r\n === 이미 닫혀 있음 === \r\n\r\n"));
      }
      else {
        //안되면 while 사용해야됨 
        if((leftEndWindowTime - leftStartWindowTime) > (WINDOW_ANGLE_MAX *10)){
          leftStartWindowTime = millis();
          leftWindowAngle = WINDOW_ANGLE_MIN;
          leftWindow.write(leftWindowAngle);
        }
        else {
          Serial.print(F("\r\n\r\n === 작동 대기중 === \r\n\r\n"));
          leftEndWindowTime = millis();
        }
      }
    }
    else if(LeftControl == ON){
      //open window control
      if(leftWindowAngle == WINDOW_ANGLE_MAX){
        Serial.print(F("\r\n\r\n === 이미 열려 있음 === \r\n\r\n"));
      }
      else {
        if((leftEndWindowTime - leftStartWindowTime) > (WINDOW_ANGLE_MAX *10)){
          leftStartWindowTime = millis();
          leftWindowAngle = WINDOW_ANGLE_MAX;
          leftWindow.write(leftWindowAngle);
        }
        else {
          Serial.print(F("\r\n\r\n === 작동 대기중 === \r\n\r\n"));
          leftEndWindowTime = millis();
        }
      }
    }
    else {
      Serial.print(F("\r\n === LeftWindow err, return value err === \r\n"));
    }
/////////////////////////////////////////////////////////////////////////////////
    //Right Window 개폐
    if(RightControl == OFF){
      //close window control
      if(rightWindowAngle == WINDOW_ANGLE_MIN){
        Serial.print(F("\r\n\r\n === 이미 닫혀 있음 === \r\n\r\n"));
      }
      else {
        //안되면 while 사용해야됨 
        if((rightEndWindowTime - rightStartWindowTime) > (WINDOW_ANGLE_MAX *10)){
          rightStartWindowTime = millis();
          rightWindowAngle = WINDOW_ANGLE_MIN;
          rightWindow.write(rightWindowAngle);
        }
        else {
          Serial.print(F("\r\n\r\n === 작동 대기중 === \r\n\r\n"));
          rightEndWindowTime = millis();
        }
      }
    }
    else if(RightControl == ON){
      //open window control
      if(rightWindowAngle == WINDOW_ANGLE_MAX){
        Serial.print(F("\r\n\r\n === 이미 열려 있음 === \r\n\r\n"));
      }
      else {
        if((rightEndWindowTime - rightStartWindowTime) > (WINDOW_ANGLE_MAX *10)){
          rightStartWindowTime = millis();
          rightWindowAngle = WINDOW_ANGLE_MAX;
          rightWindow.write(rightWindowAngle);
        }
        else {
          Serial.print(F("\r\n\r\n === 작동 대기중 === \r\n\r\n"));
          rightEndWindowTime = millis();
        }
      }
    }
    else {
      Serial.print(F("\r\n === RightWindow err, return value err === \r\n"));
    }
/////////////////////////////////////////////////////////////////////////////////
    //Heater Control On Off
    if(heater == OFF){
      //작동 중지 명령
      if(digitalRead(HEATER_PIN) == HIGH){
        //이미 정지 상태
        Serial.print(F("\r\n === Heater 이미 정지 상태 === \r\n"));
      }
      else {
        //가동을 정지로 변경
        heater = OFF;
      }
    }
    else if(heater == ON){
      // 작동 가동 명령
      if(digitalRead(HEATER_PIN) == LOW){
        // 이미 가동 상태
        Serial.print(F("\r\n === Heater 이미 가동 상태 === \r\n"));
      }
      else {
        // 정지를 가동으로 변경
        heater = ON;
      }
    }
    relayControl(HEATER_PIN, heater);
/////////////////////////////////////////////////////////////////////////////////
  }
  //자동 제어
  else if(manualControl == ON){ 
    uint8_t tempAvg;
    tempAvg = (tmp1Data + tmp2Data)/2;
    
    //Fan Control(Auto)
    //온도 앞 뒤 차이가 5도 이상이면 팬을 가동하여 공기 순환
    if(tmp1Data > tmp2Data){
      if((tmp1Data - tmp2Data) > 5){
        fanState = ON;
        fanSpeed = FAN_SPEED_MAX;
      }
      else if((tmp1Data - tmp2Data) > 3){
        fanState = ON;
        fanSpeed = FAN_SPEED_MID;
      }
      else if((tmp1Data - tmp2Data) > 1) {
        fanState = ON;
        fanSpeed = FAN_SPEED_MIN;
      }
      else {
        fanState = OFF;
        fanSpeed = FAN_SPEED_ZERO;
      }
      moterControl(fanState, FANCONTROL_IN_1, FANCONTROL_IN_2);
      analogWrite(FANCONTROL_SPEED,fanSpeed);
    }
/////////////////////////////////////////////////////////////////////////////////
    //히터(Auto)
    //목표온도가 현재온도와 3도이상 차이날 경우 히터 On 
    if(targetTemp > tempAvg){
      // 설정온도가 평균온도보다 높다면
      if(targetTemp - tempAvg > 3) {
        heater = ON;
      }
      else {
        heater = OFF;
      }
      relayControl(HEATER_PIN, heater);
    }
///////////////////////////////////////////////////////////////////////////////// 
    //개폐기(Auto)
    //목표온도가 현재온도와 3도이상 차이날 경우 개폐기 On 
    if(tempAvg < targetTemp){
      if((tempAvg - targetTemp) > 3){
        //타겟온도보다 평균온도가 3도보다 높으면 열어줌
        leftWindowAngle = WINDOW_ANGLE_MAX;
        rightWindowAngle = WINDOW_ANGLE_MAX;
        LeftControl = ON;
        RightControl = ON;
      }
      else {
        leftWindowAngle = WINDOW_ANGLE_MIN;
        rightWindowAngle = WINDOW_ANGLE_MIN;
        LeftControl = OFF;
        RightControl = OFF;
      }
      leftWindow.write(leftWindowAngle);
      rightWindow.write(rightWindowAngle);
    } 
  }
/////////////////////////////////////////////////////////////////////////////////
  //토양습도센서, 목표습도보다 낮은 경우 워터펌프 가동
  if(grdData <= targetMoist) {
    //10초동안 pump 가동
    if(digitalRead(PUMP_PIN) == LOW) {
      if((endPumpTime - startPumpTime) > waitPumpTime) {
        //10초동안 pump 가동 여부 확인 후 10초 이상이면 pump 및 led 종료 
        pumpState = OFF;
        ledState_1 = OFF;
        ledState_2 = OFF;
        ledState_3 = OFF;

        relayControl(PUMP_PIN, pumpState);
        relayControl(PUMP_LED_PIN_1, ledState_1);
        relayControl(PUMP_LED_PIN_2, ledState_2);
        relayControl(PUMP_LED_PIN_3, ledState_3);
      }
      else {
        //10초가 안지났으면 pump 종료를 위해 endTime 업데이트
        endPumpTime = millis();
        pumpState = ON;

        //led1 2초, led2 1초, led3 3초 이후 종료
        if((endLedTime - startLedTime) > waitPumpLedStandard) {
          //5초 후 led On으로 바꿔 준 후 start, end 시간 초기화
          startLedTime = millis();
          endLedTime = millis();
          ledState_1 = ON;
          ledState_2 = ON;
          ledState_3 = ON;
          relayControl(PUMP_LED_PIN_1, ledState_1);
          relayControl(PUMP_LED_PIN_2, ledState_2);
          relayControl(PUMP_LED_PIN_3, ledState_3);
        }
        else if((endLedTime - startLedTime) > waitPumpLedTime_3) {
          ledState_3 = OFF;
          endLedTime = millis();
          relayControl(PUMP_LED_PIN_3, ledState_3);
        }
        else if((endLedTime - startLedTime) > waitPumpLedTime_1) {
          ledState_1 = OFF;
          endLedTime = millis();
          relayControl(PUMP_LED_PIN_1, ledState_1);
        }
        else if((endLedTime - startLedTime) > waitPumpLedTime_2) {
          ledState_2 = OFF;
          endLedTime = millis();
          relayControl(PUMP_LED_PIN_2, ledState_2);
        }
        else {
          //아무것도 못들어갔다면 end만 초기화
          endLedTime = millis();
        }
        
      }
    }
    else {
      //가동 중이 아니므로 펌프 및 led 가동 정보 입력
      pumpState = ON;
      ledState_1 = ON;
      ledState_2 = ON;
      ledState_3 = ON;

      //시간 계산을 위해 pump와 led start, end 초기화
      startPumpTime = millis();
      endPumpTime = millis();
      startLedTime = millis();
      endLedTime = millis();

        relayControl(PUMP_PIN, pumpState);
        relayControl(PUMP_LED_PIN_1, ledState_1);
        relayControl(PUMP_LED_PIN_2, ledState_2);
        relayControl(PUMP_LED_PIN_3, ledState_3);
    }
  }
  else {
    //수분이 넘었다면 그냥 종료 
    pumpState = OFF;
    ledState_1 = OFF;
    ledState_2 = OFF;
    ledState_3 = OFF;

    relayControl(PUMP_PIN, pumpState);
    relayControl(PUMP_LED_PIN_1, ledState_1);
    relayControl(PUMP_LED_PIN_2, ledState_2);
    relayControl(PUMP_LED_PIN_3, ledState_3);
  }
/////////////////////////////////////////////////////////////////////////////////
  //조도센서
  //목표 조도보다 높은 경우 생장LED센서 작동
  if(luxData < targetLux) {
    relayControl(LED_PIN, OFF);
  }
  else if(luxData >= targetLux){
    relayControl(LED_PIN, ON);
  }

///////////////////////////////////////////////////////////////////////////////////
  //Setter, 현재 정보 서버에 송부
  //     /controllerSetter/우측/좌측/열풍기/팬
  url = "GET /controllerSetter/" + String(LeftControl) + "/" + String(RightControl) + "/" + String(heater) + "/" + String(fanState) + " HTTP/1.1\r\nHost: " + ip +"\r\n\r\n";
  GetSetData(SetterType, setDataType[i], url);
///////////////////////////////////////////////////////////////////////////////////
}
