#include <Wire.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <stdio.h>
#include "util.h"
#include "MediaPlayer.h"
//AD7746 definitions
#define I2C_ADDRESS  0x48 //0x90 shift one to the rigth, 0x90 for write, 0x91 for read


#define REGISTER_STATUS 0x00
#define REGISTER_CAP_DATA 0x01
#define REGISTER_VT_DATA 0x04
#define REGISTER_CAP_SETUP 0x07
#define REGISTER_VT_SETUP 0x08
#define REGISTER_EXC_SETUP 0x09
#define REGISTER_CONFIGURATION 0x0A
#define REGISTER_CAP_DAC_A 0x0B
#define REGISTER_CAP_DAC_B 0x0B
#define REGISTER_CAP_OFFSET 0x0D
#define REGISTER_CAP_GAIN 0x0F
#define REGISTER_VOLTAGE_GAIN 0x11

#define RESET_ADDRESS 0xBF

#define VALUE_UPPER_BOUND 16000000L 
#define VALUE_LOWER_BOUND 0xFL
#define MAX_OUT_OF_RANGE_COUNT 3
#define CALIBRATION_INCREASE 1
byte calibration;
byte outOfRangeCount = 0, outOfRangeCountb=0;
byte touchState = 0, newState=0, prevState=0; 
int countA=0, countB=0;
unsigned long offset = 0;
int sensorA=0, sensorB=0; //0=no change state, 1= detect touch, 2=remove touch
int senAstate=LOW,senBstate=LOW;
int prevdxA=0, prevdxB=0;

long initialValueA[4], initialValueB[4];
long aveA,aveB=0;
int progCounter=0;

MediaPlayer mediaPlayer; // create only one object, must be global

uint8_t outbuf1[10];             // array to store nunchuck1 output
uint8_t outbuf2[10];		// array to store nunchuck2 output
int cnt = 0;                    // count to make sure 6 bytes are received from nunchuck
int nunPin1 = 8;                // Digital pin to select nunchuck1
int nunPin2 = 9;                // Digital pin to select nunchuck2

int state = HIGH;                // keeps tabs on which nunchuck is being read (Low = n1, High = n2)

void setup()
{
  progCounter=0;
  pinMode(nunPin1, OUTPUT);
  pinMode(nunPin2, OUTPUT);
  digitalWrite(nunPin1, HIGH);
  digitalWrite(nunPin2, HIGH);
  for(int i=0;i<4;i++){
    initialValueA[i]=0;
    initialValueB[i]=0;
  }
  Wire.begin(); // sets up i2c for operation
  Serial.begin(9600); // set up baud rate for serial

  Serial.println("Initializing");

  Wire.beginTransmission(I2C_ADDRESS); // start i2c cycle
  Wire.send(RESET_ADDRESS); // reset the device
  Wire.endTransmission(); // ends i2c cycle

  //wait a tad for reboot
  delay(10);

  writeRegister(REGISTER_EXC_SETUP,  _BV(3) | _BV(1) | _BV(0)); // EXC source A & source B

  writeRegister(REGISTER_CAP_SETUP,_BV(7)); // cap setup reg - cap enabled

  Serial.println("Getting offset");
  offset = ((unsigned long)readInteger(REGISTER_CAP_OFFSET)) << 8;  
  Serial.print("Factory offset: ");
  Serial.println(offset);

  writeRegister(REGISTER_CONFIGURATION, _BV(7) | _BV(6) | _BV(5) | _BV(4) | _BV(3) | _BV(2) | _BV(0));  // set configuration to calib. mode, slow sample

  //wait for calibration
  delay(100);

  displayStatus();
  Serial.print("Calibrated offset: ");
  offset = ((unsigned long)readInteger(REGISTER_CAP_OFFSET)) << 8;  
  Serial.println(offset);

  writeRegister(REGISTER_CAP_SETUP,_BV(7)); // cap setup reg - cap enabled

  writeRegister(REGISTER_EXC_SETUP, _BV(3)); // EXC source A

  writeRegister(REGISTER_CONFIGURATION, _BV(7) | _BV(6) | _BV(5) | _BV(4) | _BV(3) | _BV(0)); // continuous mode

  displayStatus();
 calibrate();
//digitalWrite(nunPin1, HIGH);
//  digitalWrite(nunPin2, LOW);
//readValue();
  delay(100);
  Serial.println("done");
  
}

void loop() // main program begins
{
long valueA, valueB;
int dxA, dxB;

          // if we recieve date we print out the status
        if (Serial.available() > 0) {
            // read the incoming byte:
           Serial.read();
           displayStatus();
        }
          
        state_A();
        delay(20);
        valueA = readValue();
        delay(50);
        if ((valueA<VALUE_LOWER_BOUND) or (valueA>VALUE_UPPER_BOUND)) {
          outOfRangeCount++;
        }
        if (outOfRangeCount>MAX_OUT_OF_RANGE_COUNT) {
          if (valueA < VALUE_LOWER_BOUND) {
            calibrate(-CALIBRATION_INCREASE);
          } 
          else {
            calibrate(CALIBRATION_INCREASE);
          }
          outOfRangeCount=0;
        }
        valueA = map(valueA, 0XFL, 16000000L, 0, 300);
     //   Serial.print("AverageA:");
     //   Serial.println(aveA);
       //   Serial.print("ValueA:");
       //   Serial.println(valueA);
      // valueA = map(valueA, 0XFL, 16000000L, 0, 300);
  
       state_B();
       delay(20);
       valueB = readValue();
       delay(50);
       if ((valueB<VALUE_LOWER_BOUND) or (valueB>VALUE_UPPER_BOUND)) {
        outOfRangeCountb++;
        }
       if (outOfRangeCountb>MAX_OUT_OF_RANGE_COUNT) {
          if (valueB < VALUE_LOWER_BOUND) {
            calibrate(-CALIBRATION_INCREASE);
        } 
        else {
          calibrate(CALIBRATION_INCREASE);
        }
        outOfRangeCountb=0;
        }
      valueB = map(valueB, 0XFL, 16000000L, 0, 300);  
    //  Serial.print("AverageB:");
    //  Serial.println(aveB);
    //  Serial.print("ValueB:");
     // Serial.println(valueB);
  
      dxA = aveA - valueA;
   // dxA=0;
      dxB = aveB - valueB;
   //   dxB=0;
      
   //    Serial.print("dxA values:");
   //    Serial.println(dxA);
      // Serial.print("dxB values:");
      // Serial.println(dxB);    
       
       if(progCounter<20)
         progCounter++;
      else{
      //touch classifier
      if( (abs(dxA)>4) && dxA>0 && senAstate==LOW)
      {
        if(prevdxA>3){
          sensorA=1;
          senAstate=HIGH;
        }
      }
      else if((abs(dxA)>4)&& dxA<0 && senAstate==HIGH)
      {
        if(prevdxA<-3){
        sensorA=2; //remove touch
        senAstate=LOW;
        }
      }
      else sensorA=0; //no change
      
      if( (abs(dxB)>4) && dxB>0 && senBstate==LOW)
      {
        if(prevdxB>3)
        {
          sensorB=1; //detect touch
          senBstate=HIGH;
        }
      }
      else if((abs(dxB)>3)&& dxB<0 && senBstate==HIGH)
      {
        if(prevdxB<-3)
        {
          sensorB=2; //remove touch
          senBstate=LOW;
        }
      }
      else sensorB=0; //no change
      
      if((sensorA==1 ||( sensorA==0 && senAstate==HIGH )) && 
      ((senBstate==LOW && sensorB==0) || (sensorB==2)))
         newState=1;
      else if((sensorB==1 ||( sensorB==0 && senBstate==HIGH )) && 
      ((senAstate==LOW && sensorA==0) || (sensorA==2))) 
        newState=2;
      else if((sensorA==1 || (senAstate==HIGH) && sensorA==0) &&
              (sensorB==1 || (senBstate==HIGH) && sensorB==0)) 
              newState=3;
      else {newState=0;
          //  Serial.println("No touch detected");
          }
      
      if((prevState==0)&&(newState==0)){
        //Serial.println("Transition state 0");
        touchState=0;
      }
      else if((prevState==0)&&(newState==1)){
        Serial.println("Transition to state 1");
        touchState=1;
      }
      else if((prevState==0)&&(newState==2)){
        Serial.println("Transition to state 2");
        touchState=2;
      }
      else if((prevState==0)&&(newState==3)){
        Serial.println("Transition to state 3");
        touchState=3;
      }
      else if((prevState==1)&&(newState==0)){
        Serial.println("Transition to state 4");
        touchState=4;
      }
      else if((prevState==2)&&(newState==0)){
        Serial.println("Transition to state 5");
        touchState=5;
      }
      else if((prevState==3)&&(newState==0)){
        Serial.println("Transition to state 6");
        touchState=6;
      }
      else if((prevState==1)&&(newState==3)){
        Serial.println("Transition to state 7");
        touchState=7;
      }
      else if((prevState==2)&&(newState==3)){
        Serial.println("Transition to state 8");
        touchState=8;
      }
      else if((prevState==3)&&(newState==1)){
        Serial.println("Transition to state 9");
        touchState=9;
      }
      else if((prevState==3)&&(newState==2)){
        Serial.println("Transition to state 10");
        touchState=10;
      }
      else if(prevState==newState){
       // Serial.println("No state change detected");
        touchState=0;
      }
     prevState=newState;
    
    switch (touchState){
      case 1: 
        mediaPlayer.stop();
      //  delay(100);
      //  Serial.println("Play HAI-01.wav");
        mediaPlayer.play("HAI-01.WAV");
      //playComplete("TEO-01.WAV");
        delay(20);
        break;
      case 2: 
        mediaPlayer.stop();
      //  delay(100);
      //  Serial.println("Play HAI-02.wav");
        mediaPlayer.play("HAI-02.WAV");
        delay(20);
        break;
      case 3:
        mediaPlayer.stop();
      //  delay(100);
      //  Serial.println("Play HAI-03.wav");
        mediaPlayer.play("HAI-03.WAV");
        delay(20);
        break;   
      case 4: 
        mediaPlayer.stop();
      //  delay(100);
      //  Serial.println("Play HAI-04.wav");
        mediaPlayer.play("HAI-04.WAV");
        delay(20);
        break;
      case 5:
        mediaPlayer.stop();
      //  delay(100);
      //  Serial.println("Play HAI-05.wav");
        mediaPlayer.play("HAI-05.WAV");
        delay(20);
        break;
      case 6:
        mediaPlayer.stop();
      //  delay(100);
      //  Serial.println("Play HAI-06.wav");
        mediaPlayer.play("HAI-06.WAV");
        delay(20);
        break;
      case 7:
        mediaPlayer.stop();
      //  delay(100);
      //  Serial.println("Play HAI-07.wav");
        mediaPlayer.play("HAI-07.WAV");
        delay(20);
        break;
      case 8:
        mediaPlayer.stop();
      //  delay(100);
      //  Serial.println("Play HAI-08.wav");
        mediaPlayer.play("HAI-08.WAV");
        delay(20);
        break;
      case 9:
        mediaPlayer.stop();
      //  delay(100);
      //  Serial.println("Play HAI-09.wav");
        mediaPlayer.play("HAI-09.WAV");
        delay(20);
        break;
      case 10:
        mediaPlayer.stop();
      //  delay(100);
      //  Serial.println("Play HAI-10.wav");
        mediaPlayer.play("HAI-10.WAV");
        delay(20);
        break;
      default:
        //Serial.println("Default case");
        delay(20);
        
    }
      }
  

     for(int i=3;i>=1;i--){
       initialValueA[i]=initialValueA[i-1];
       initialValueB[i]=initialValueB[i-1];
     }
     initialValueA[0]=valueA;
     initialValueB[0]=valueB; 
     prevdxA=dxA;
     prevdxB=dxB;
     aveA = (initialValueA[0]+initialValueA[1]+initialValueA[2]+initialValueA[3])/4;
     aveB = (initialValueB[0]+initialValueB[1]+initialValueB[2]+initialValueB[3])/4;
 
}

void calibrate (byte direction) {
  calibration += direction;
  //assure that calibration is in 7 bit range
  calibration &=0x7f;
  writeRegister(REGISTER_CAP_DAC_A, _BV(7) | calibration);
}

void calibrate() {
  calibration = 0;

  Serial.println("Calibrating CapDAC A");

  long value = readValue();

  while (value>VALUE_UPPER_BOUND && calibration < 128) {
    calibration++;
    writeRegister(REGISTER_CAP_DAC_A, _BV(7) | calibration);
    value = readValue();
  }
  Serial.println("done");
}

void
send_zero ()
{
  Wire.beginTransmission (I2C_ADDRESS);	// transmit to device
  Wire.send (0x00);		// sends one byte
  Wire.endTransmission ();	// stop transmitting
}

void
change_nunchuck()      // Swaps between nunchucks
{
  if (state == LOW)
  {
    digitalWrite(nunPin1, LOW);
    digitalWrite(nunPin2,HIGH);
    state = HIGH;
  }
  else
  {
    digitalWrite(nunPin1, HIGH);
    digitalWrite(nunPin2,LOW);
    state = LOW;
  }
}

void state_A()
{
  digitalWrite(nunPin1,HIGH);
  digitalWrite(nunPin2,LOW);
  state=LOW;
}

void state_B(){
  digitalWrite(nunPin1, LOW);
  digitalWrite(nunPin2, HIGH);
  state=HIGH;
}

long readValue() {
  long ret = 0;
//  uint8_t data[3];

  char status = 0;
  //wait until a conversion is done
  while (!(status & (_BV(0) | _BV(2)))) {
    //wait for the next conversion
    status= readRegister(REGISTER_STATUS);
  }

  unsigned long value =  readLong(REGISTER_CAP_DATA);

  value >>=8;
  //we have read one byte to much, now we have to get rid of it
  ret =  value;

  return ret;
}
