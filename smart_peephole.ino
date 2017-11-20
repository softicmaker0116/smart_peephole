// Cat Eye Project 
#include <Wire.h>
#include <SeeedOLED.h>
//  Updates:
//    2016.11.01    Basic function
//    2016.11.13    Arduino Nano - Test pass
//    2016.11.21    LinkIt 7688 Duo - Test pass
#include <arduino.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

#define gas_sensorPin A0
#define pingPin 4
#define buzzer 7
/*************************camera start************************/
#define PIC_PKT_LEN           128              //data length of each read, dont set this too big because ram is limited
//#define PIC_JPEG_RESO_LOW     0x01             // JPEG resolution: 80*64
#define PIC_JPEG_RESO_QQVGA   0x03             // JPEG resolution: 160*128
#define PIC_JPEG_RESO_QVGA    0x05             // JPEG resolution: 320*240
#define PIC_JPEG_RESO_VGA     0x07             // JPEG resolution: 640*480
//#define PIC_COLOER_GRAY       0x03             // 8-bits Gray-Scale
#define PIC_COLOER_JPEG       0x07             // JPEG

#define CAM_ADDR       0
#define CAM_SERIAL     softSerial

#define PIC_JPEG_RESO         PIC_JPEG_RESO_QQVGA
#define PIC_COLOER            PIC_COLOER_JPEG

File myFile;
//SoftwareSerial softSerial(2, 3);  //rx,tx for UART for Arduino
SoftwareSerial softSerial(11, 12);  //rx,tx for UART for LinkIt 7688

const byte cameraAddr = (CAM_ADDR << 5);  // addr
const int buttonPin = A5;                 // the number of the pushbutton pin
unsigned long picTotalLen = 0;            // picture length
//const int Camera_CS = 10;                 // Camera CS for Arduino
const int Camera_CS = 17;                 // Camera CS for LinkIt 7688
char picName[] = "pic******.jpg";
int picNameNum = 0;
/*************************camera end************************/
//Motion detection
/************************ UltraSound Pin Connection Setup Start ********************************/
//  The circuit:
//  * +V connection of the PING))) attached to +5V
//  * GND connection of the PING))) attached to ground
//  * Trigger connection of the PING))) attached to digital pin 5.
//  * Output  connection of the PING))) attached to digital pin 4.
/************************ UltraSound Pin Connection Setup End ********************************/
/************************ Gas Connection Setup Start ********************************/
// Plug Gas-detector in the A0
/************************ Gas Connection Setup End ********************************/
/************************ Speaker Connection Setup Start ********************************/
// Plug one speaker in the digital pin 7
// Plug another speaker in the digital pin 8

/************************ Speaker Connection Setup End ********************************/
void CAM_sync()
{
    char cmd[] = {0xaa,0x0d|cameraAddr,0x00,0x00,0x00,0x00} ;
    unsigned char resp[6];

    Serial.println("Sync with camera...");
  
  while (1) 
  {
    //clearRxBuf();
    sendCmd(cmd,6);
    
    if (readBytes((char *)resp, 6,1000) != 6) {continue;}
    else {debug_S2M(resp);}
    
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x0d && resp[4] == 0 && resp[5] == 0) {
      //Serial.println("\nACK received");
      if (readBytes((char *)resp, 6, 500) != 6) {continue;}
      else {debug_S2M(resp);}
      
      if (resp[0] == 0xaa && resp[1] == (0x0d | cameraAddr) && resp[2] == 0 && resp[3] == 0 && resp[4] == 0 && resp[5] == 0) {
        //Serial.println("SYNC received");
        break;
      }
    }
  }  
  cmd[1] = 0x0e | cameraAddr;
  cmd[2] = 0x0d;
    //Serial.println("Start sending ACK...");
    sendCmd(cmd, 6);
    //Serial.println("done");
    Serial.println("Camera synchronization success!\n");
}

/*********************************************************************/
void clearRxBuf()
{
  while (CAM_SERIAL.available()) 
  {
    CAM_SERIAL.read(); 
  }
}
/*********************************************************************/
void sendCmd(char cmd[], int cmd_len)
{
  Serial.print("[M->>S] "); // For debug: debug_M2S()
  for (char i = 0; i < cmd_len; i++) {
    CAM_SERIAL.write(cmd[i]);
    Serial.print((unsigned char)cmd[i],HEX); // For debug: debug_M2S()
    Serial.print("-"); // For debug: debug_M2S()
  }
  Serial.println(""); // For debug: debug_M2S()
}
/*********************************************************************/
void debug_S2M(unsigned char resp[]){ // print cmds from slave to master
    Serial.print("[M<<-S] ");
    for(char i = 0; i < 6; i++){
      Serial.print((unsigned char)resp[i],HEX);
      Serial.print("-");
    }
    Serial.println("");
}

void int2str(int i, char *s) {
  sprintf(s,"pic%d.jpg",i);
}
/*********************************************************************/
int readBytes(char *dest, int len, unsigned int timeout)
{
  int read_len = 0;
  unsigned long t = millis();
  while (read_len < len)
  {
    while (CAM_SERIAL.available()<1)
    {
      if ((millis() - t) > timeout)
      {
        Serial.println("[S->>M]\tTimeout reach");
        return read_len;
      }
    }
    *(dest+read_len) = CAM_SERIAL.read();
    //Serial.print((unsigned char) *(dest+read_len),HEX);
    read_len++;
  }
  //Serial.println();
  return read_len;
}
/*********************************************************************/

/*********************************************************************/
void CAM_init()
{
    char cmd[] = { 0xaa, 0x01 | cameraAddr, 0x00, PIC_COLOER, 0x00,  PIC_JPEG_RESO};
    unsigned char resp[6]; 
  
    Serial.println("Inititialing camera...");
  while (1)
  {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (readBytes((char *)resp, 6, 100) != 6) {continue;}
    else {debug_S2M(resp);}
    
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x01 && resp[4] == 0 && resp[5] == 0) {
      Serial.println("Camera inititialization success!\n");
      break;
    }
  }
}

void CAM_CaptMode()
{
  char cmd[] = { 0xaa, 0x06 | cameraAddr, 0x08, PIC_PKT_LEN & 0xff, (PIC_PKT_LEN>>8) & 0xff ,0}; 
  unsigned char resp[6];

  Serial.println("Set capture mode...");
    
  while (1)
  {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (readBytes((char *)resp, 6, 100) != 6) {continue;}
    else {debug_S2M(resp);}
    
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x06 && resp[4] == 0 && resp[5] == 0) break; 
  }
  
  // set snapshot - compressed picture
  cmd[1] = 0x05 | cameraAddr;
  cmd[2] = 0;
  cmd[3] = 0;
  cmd[4] = 0;
  cmd[5] = 0; 
  while (1)
  {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (readBytes((char *)resp, 6, 100) != 6) {continue;}
    else {debug_S2M(resp);}
    
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x05 && resp[4] == 0 && resp[5] == 0) break;
  }
  Serial.println("Capture mode setting completed!\n");
}
/*********************************************************************/
void CAM_Capture()
{
 
  char cmd[] = { 0xaa, 0x04 | cameraAddr, 0x01, 0x00, 0x00, 0x00 }; 
  unsigned char resp[6];
  
  Serial.println("Set image-get setting...");
  
  while (1) 
  {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (readBytes((char *)resp, 6, 100) != 6) {continue;}
    else {debug_S2M(resp);}
    
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x04 && resp[4] == 0 && resp[5] == 0)
    {
      if (readBytes((char *)resp, 6, 1000) != 6) {continue;}
      else {debug_S2M(resp);}
      
      if (resp[0] == 0xaa && resp[1] == (0x0a | cameraAddr) && resp[2] == 0x01)
      {
        picTotalLen = (resp[3]) | (resp[4] << 8) | (resp[5] << 16); 
        Serial.print("Image-get setting completed! picTotalLen: ");
        Serial.println(picTotalLen);
        Serial.println();
        break;
      }
    }
  }
  
  Serial.println("Start getting image...");
  
  unsigned int pktCnt = (picTotalLen) / (PIC_PKT_LEN - 6); 
  if ((picTotalLen % (PIC_PKT_LEN-6)) != 0) pktCnt += 1;
  
  cmd[1] = 0x0e | cameraAddr;  
  cmd[2] = 0x00;
  unsigned char pkt[PIC_PKT_LEN];

  int2str(picNameNum, picName);
  while(SD.exists(picName)){
    picNameNum++;
    int2str(picNameNum, picName);
  }
  
  myFile = SD.open(picName, FILE_WRITE); 
  //if(!myFile){
  //  Serial.println("myFile open fail...");
  //}else{
    for (unsigned int i = 0; i < pktCnt; i++)
    {
      cmd[4] = i & 0xff;
      cmd[5] = (i >> 8) & 0xff;
      
      int retry_cnt = 0;
    retry:
      delay(10);
      clearRxBuf(); 
      sendCmd(cmd, 6); 
      uint16_t cnt = readBytes((char *)pkt, PIC_PKT_LEN, 200);
      
      unsigned char sum = 0; 
      for (int y = 0; y < cnt - 2; y++)
      {
        sum += pkt[y];
      }
      if (sum != pkt[cnt-2])
      {
        if (++retry_cnt < 100) goto retry;
        else break;
      }
      
      myFile.write((const uint8_t *)&pkt[4], cnt-6); 
      //if (cnt != PIC_PKT_LEN) break;
    }
    cmd[4] = 0xf0;
    cmd[5] = 0xf0; 
    sendCmd(cmd, 6); 
  //}
  myFile.close();
  Serial.println("Capturing completed!");
  Serial.print("PIC name: ");
  Serial.println(picName);
  picNameNum ++;
}
long microsecondsToInches(long microseconds) {
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds) {
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}



void setup() {
  // initialize serial communication:
  Serial.begin(9600);
  /*************************camera start************************/
  CAM_SERIAL.begin(9600);       //cant be faster than 9600, maybe difference with diff board.
  
    pinMode(buttonPin, INPUT);    // initialize the pushbutton pin as an input
    pinMode(Camera_CS, OUTPUT);          // CS pin of SD Card Shield
   
    delay(3000);  // uart log failed if not add this line...
    SD_init();
    CAM_sync();
  /*************************camera end************************/
}

void SD_init()
{
    delay(2000);     // wait for serial port stable before printing log
    Serial.print("Initializing SD card....");
 
    while(!SD.begin(Camera_CS)){
        Serial.println("failed");
        delay(5000);
        Serial.print("Initializing SD card....");
    }
    Serial.println("success!\n");
}

// set initial value
int turn_on_camera_flag = 0;
int turn_on_speaker_flag = 0;
int Gas_Sensor = 0;
int user_manual_mode = 0;
long ultrasound_duration =0, ultrasound_distance_inches=0, ultrasound_distance_cm=0;
int ii=0, bee_length=0, star_length=0, toneNo=0;
int bee_duration=0,star_duration=0;
const int toneTable[7][5]={
    { 66, 131, 262, 523, 1046},  // C Do
    { 74, 147, 294, 587, 1175},  // D Re
    { 83, 165, 330, 659, 1318},  // E Mi
    { 88, 175, 349, 698, 1397},  // F Fa
    { 98, 196, 392, 784, 1568},  // G So
    {110, 220, 440, 880, 1760},  // A La
    {124, 247, 494, 988, 1976}   // B Si
};
char toneName[]="CDEFGAB";
char beeTone[]="GEEFDDCDEFGGGGEEFDDCEGGEDDDDDEFEEEEEFGGEEFDDCEGGC"; 
char starTone[]="CCGGAAGFFEEDDCGGFFEEDGGFFEEDCCGGAAGFFEEDDC";
int beeBeat[]={
    1,1,2, 1,1,2, 1,1,1,1,1,1,2,
    1,1,2, 1,1,2, 1,1,1,1,4,
    1,1,1,1,1,1,2, 1,1,1,1,1,1,2,
    1,1,2, 1,1,2, 1,1,1,1,4
};
int starBeat[]={
    1,1,1,1,1,1,2, 1,1,1,1,1,1,2,
    1,1,1,1,1,1,2, 1,1,1,1,1,1,2,
    1,1,1,1,1,1,2, 1,1,1,1,1,1,2
};

int getTone(char symbol) {
    int toneNo = 0;
    for ( int ii=0; ii<7; ii++ ) {
        if ( toneName[ii]==symbol ) {
            toneNo = ii;
            break;
        }
    }
    return toneNo;
}

void star_speaker() {
    // star
    star_length = sizeof(starTone)-1; 
    for ( ii=0; ii<star_length; ii++ ) {
        toneNo = getTone(starTone[ii]);
        star_duration = starBeat[ii]*333;
        tone(buzzer,toneTable[toneNo][3]);
        delay(star_duration);
        noTone(buzzer);
    }
    noTone(buzzer);
    //delay(20);
}
void bee_speaker() {
    // bee
    bee_length = sizeof(beeTone)-1; 
    for ( ii=0; ii<bee_length; ii++ ) {
        toneNo = getTone(beeTone[ii]);
        bee_duration = beeBeat[ii]*333;
        tone(buzzer,toneTable[toneNo][3]);
        delay(bee_duration);
        noTone(buzzer);
    }
    //delay(20);
}
void urgent_speaker() {
    // simulate phone call
    for ( int ii=0; ii<100; ii++ ) {
        tone(buzzer,1000);
        delay(50);
        tone(buzzer,500);
        delay(50);
    }
    noTone(buzzer);
    //delay(20);
}
void phone_speaker() {
    // simulate phone call
    for ( int ii=0; ii<10; ii++ ) {
        tone(buzzer,1000);
        delay(50);
        tone(buzzer,500);
        delay(50);
    }
    noTone(buzzer);
    //delay(20);
}
void loop() {
  /******************* UltraSound operation  Start ************************/ 
  // UltraSound operation
  // establish variables for duration of the ping,
  // and the distance result in inches and centimeters:
  
  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);
  // The same pin is used to read the signal from the PING))): a HIGH
  // pulse whose duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(pingPin, INPUT);
  ultrasound_duration = pulseIn(pingPin, HIGH);
  // convert the time into a distance
  // ultrasound_distance_inches = microsecondsToInches(ultrasound_duration);
  //Serial.print(ultrasound_distance_inches);
  //Serial.print("ultrasound_distance_inches, ");
  ultrasound_distance_cm = microsecondsToCentimeters(ultrasound_duration);
  /*
  Serial.print(ultrasound_distance_cm);
  Serial.print("ultrasound_distance_cm");
  Serial.println();
  delay(100);
  */
  /******************** UltraSound operation End **********************/ 
  /******************** Gas detection operation Start ********************/ 
  pinMode(gas_sensorPin, INPUT);
  Gas_Sensor = analogRead(gas_sensorPin);
  //Check Gas in port monitor
  //Serial.print("Gas_Sensor: ");
  //Serial.print(Gas_Sensor);
  //Serial.print(" \n");
  /******************** Gas detection operation End ********************/ 
  /******************** User manual operation Start ********************/ 
  //receive MCS signal and want to cateye to turn on the camera
  //user_manual_mode = 1;
  /******************** User manual operation End ********************/ 
  /******************** Speaker operation Start ********************/ 
  //pinMode(buzzer,OUTPUT);
  //noTone(buzzer);
  /******************** Speaker operation End ********************/ 
  /******************** Status Check Start *********************************/
  // status check on port monitor
  // UltraSound 
  Serial.print(ultrasound_distance_cm);
  Serial.print(" ultrasound_distance_cm \n");
 
  // Gas
  Serial.print("Gas_Sensor: ");
  Serial.print( Gas_Sensor);
  Serial.print(" \n");
  /******************** Status Check End *********************************/
  /******************** Cateye Stage 1 Start *******************************/
  // Motion detected or Gas detected or User confirmation
  if ((ultrasound_distance_cm > 10) && (ultrasound_distance_cm < 50) && (ultrasound_distance_cm != 0)) { // motion detected 
    bee_speaker();
    turn_on_camera_flag = 1;
  } else if ((ultrasound_distance_cm <= 10) && (ultrasound_distance_cm != 0)) { // too closed
    phone_speaker();
    turn_on_camera_flag = 1;
  } else if (Gas_Sensor > 300) { // gas detected
    urgent_speaker();
    turn_on_camera_flag = 1;
  } else if (user_manual_mode == 1) { // user manual mode 
    star_speaker();
    turn_on_camera_flag = 1;
  }

  int n=0;
    //while(1){
        Serial.println("[Info]\tPress the button to take a picture");
        //while (digitalRead(buttonPin) == LOW);
        //while (turn_on_camera_flag == 0);
        //if(digitalRead(buttonPin) == HIGH){
            //delay(20);                               //Debounce
            //if (digitalRead(buttonPin) == HIGH)
             Serial.print("turn_on_camera_flag = ");
             Serial.print(turn_on_camera_flag);
             Serial.print(" \n");
            if (turn_on_camera_flag == 1) 
            {
                Serial.println("Pressed-botton detected...\n");
                delay(200);
                if(n == 0) CAM_init();
                CAM_CaptMode();
                CAM_Capture();
            }
            Serial.print("Process completed ,number : ");
            Serial.println(n);
            Serial.println("\n");
            n++ ;
        //}
    //}
  if (turn_on_camera_flag == 1) {
    // camera operation

    // SD card operation

    // MCS operation
  }
  /******************** Cateye Stage 1 End *********************************/
  /******************** Cateye Stage 2 Start *******************************/ 
  // Motion not detected or Gas not detected or User not confirmation
  /******************** Cateye Stage 2 End *********************************/
  if (turn_on_camera_flag == 1) { // camera work done and return to initial stage
    turn_on_camera_flag = 0;
  }
  
  
}

