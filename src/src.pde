#include <MsTimer2.h>
#include <WaveRP.h>
#include <SdFat.h>
#include <Sd2Card.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include "freeRam.h"
#include "PgmPrint.h"

#include "TpConf.h"
#include "TpAudio.h"

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

#define nunPin1 8 // Digital pin to select nunchuck1
#define nunPin2 9 // Digital pin to select nunchuck2

#define MILLIS_TO_WAIT 5000 // unit in ms.

byte calibration;
byte outOfRangeCount = 0, outOfRangeCountb=0;
byte touchState = 0, newState=0, prevState=0;
unsigned long offset = 0;
int sensorA=0, sensorB=0; //0=no change state, 1= detect touch, 2=remove touch
int senAstate=LOW,senBstate=LOW;
int prevdxA=0, prevdxB=0;

long initialValueA[4], initialValueB[4];
long aveA=0,aveB=0;
int progCounter=0;

int state = HIGH; // keeps tabs on which nunchuck is being read (Low = n1, High = n2)

//------------------------------------------------------------------------------
// Global variables related to faking sensor data.
#define HAVE_SENSOR 1
#if !HAVE_SENSOR
prog_uchar fakeEventSignals[] PROGMEM = {
  1, 2, 3, 4, 5, 6, 7, 8, 9
};
prog_uint32_t fakeEventIntervals[] PROGMEM = {
  500, 3000, 3000, 3000, 8000, 3000, 3000, 3000, 3000
};
long fakeEventTimeout = -1;
long fakePreviousMillis = -1;
unsigned char fakeEventIndex = 0;
#endif

//------------------------------------------------------------------------------
// Global variables related to log.
SdFile logger; // log file

//------------------------------------------------------------------------------
// Global variables related to file system and wave playback.
Sd2Card card; // SD/SDHC card with support for version 2.00 features
SdVolume vol; // FAT16 or FAT32 volume
SdFile root; // volume's root directory
SdFile file; // current file
WaveRP wave; // wave file recorder/player

//------------------------------------------------------------------------------
// Global variables related to timer.
bool timerStarted = false;
long actStart = -1;
long actEnd = -1;

//------------------------------------------------------------------------------
// print error message and halt
void error(char *str)
{
  PgmPrint("error: ");
  Serial.println(str);
  if (card.errorCode()) {
    PgmPrint("sdError: ");Serial.println(card.errorCode(), HEX);
    PgmPrint("sdData: ");Serial.println(card.errorData(), HEX);
  }
  while(1);
}

//------------------------------------------------------------------------------
// play a file
void playFile(char *name)
{
  wave.stop();
  file.close();

  if (!file.open(root, name)) {
    PgmPrint("Can't open: ");
    Serial.println(name);
    return;
  }
  if (!wave.play(file)) {
    PgmPrint("Can't play: ");Serial.println(name);
    file.close();
    return;
  }
  PgmPrint("Playing: ");Serial.println(name);
}

//------------------------------------------------------------------------------
// play a wave by index
void playWave(int index)
{
  char name[13] = "";

  if (index < 1 || index > 10) return;

  strncpy_P(name, (char *)pgm_read_word(&(TP_WAVES[index - 1])), sizeof(name));
  playFile(name);
}

//------------------------------------------------------------------------------
// Timer callback
void timerCb()
{
  PgmPrint("Logging: ");
  Serial.print(actStart, DEC);
  PgmPrint(", ");
  Serial.print(actEnd, DEC);
  PgmPrint(", ");
  Serial.println(actEnd - actStart, DEC);

  logger.print(actStart, DEC);
  logger.print(", ");
  logger.print(actEnd, DEC);
  logger.print(", ");
  logger.println(actEnd - actStart, DEC);
  logger.sync();

  // Reset
  MsTimer2::stop();
  timerStarted = false;
  actStart = actEnd = -1;
}

//------------------------------------------------------------------------------
// setup
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

  PgmPrintln("Initializing");

  PgmPrint("Using panel: ");
  Serial.println(PANEL_NUMBER);

  Wire.beginTransmission(I2C_ADDRESS); // start i2c cycle
  Wire.send(RESET_ADDRESS); // reset the device
  Wire.endTransmission(); // ends i2c cycle

  //wait a tad for reboot
  delay(10);

  writeRegister(REGISTER_EXC_SETUP,  _BV(3) | _BV(1) | _BV(0)); // EXC source A & source B

  writeRegister(REGISTER_CAP_SETUP,_BV(7)); // cap setup reg - cap enabled

  PgmPrintln("Getting offset");
#if HAVE_SENSOR
  offset = ((unsigned long)readInteger(REGISTER_CAP_OFFSET)) << 8;
#else
  offset = 0;
#endif
  PgmPrint("Factory offset: ");
  Serial.println(offset);

  writeRegister(REGISTER_CONFIGURATION, _BV(7) | _BV(6) | _BV(5) | _BV(4) | _BV(3) | _BV(2) | _BV(0));  // set configuration to calib. mode, slow sample

  //wait for calibration
  delay(100);

#if HAVE_SENSOR
  displayStatus();
#endif
  PgmPrint("Calibrated offset: ");
#if HAVE_SENSOR
  offset = ((unsigned long)readInteger(REGISTER_CAP_OFFSET)) << 8;
#else
  offset = 0;
#endif
  Serial.println(offset);

  writeRegister(REGISTER_CAP_SETUP,_BV(7)); // cap setup reg - cap enabled

  writeRegister(REGISTER_EXC_SETUP, _BV(3)); // EXC source A

  writeRegister(REGISTER_CONFIGURATION, _BV(7) | _BV(6) | _BV(5) | _BV(4) | _BV(3) | _BV(0)); // continuous mode

#if HAVE_SENSOR
  displayStatus();
#endif
  calibrate();
  //digitalWrite(nunPin1, HIGH);
  //  digitalWrite(nunPin2, LOW);
  //readValue();
  delay(100);

  // Initialize card, FAT and root.
  if (!card.init()) error("ci");
  if (!vol.init(card)) error("vi");
  if (!root.openRoot(vol)) error("or");

  // Search for the filename to log.
  // Pick the largest number.
  char name[] = "TS000.LOG";
  for (int i = 0; i < 1000; i++) {
    name[2] = (i / 100) + '0';
    name[3] = (i / 10) % 10 + '0';
    name[4] = i % 10 + '0';
    if (logger.create(root, name)) break;
  }
  if (!logger.isOpen()) error("lc");
  PgmPrint("Log filename for this session: ");
  Serial.println(name);

  // Initialize timer.
  MsTimer2::set(MILLIS_TO_WAIT, timerCb);

  // Added header for the log file.
  const char *str = PSTR("Start, End, Duration");
  for (uint8_t c; (c = pgm_read_byte(str)); str++) logger.print(c);
  logger.println();
  logger.sync();

  PgmPrint("FreeRam: ");
  Serial.println(freeRam());

  PgmPrintln("done");
}

//------------------------------------------------------------------------------
// loop
void loop() // main program begins
{
  long valueA, valueB;
  int dxA, dxB;

  // if we recieve date we print out the status
  if (Serial.available() > 0) {
    // read the incoming byte:
    Serial.read();
#if HAVE_SENSOR
    displayStatus();
#endif
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
  // PgmPrint("AverageA:");
  // Serial.println(aveA);
  // PgmPrint("ValueA:");
  // Serial.println(valueA);

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
  // PgmPrint("AverageB:");
  // Serial.println(aveB);
  // PgmPrint("ValueB:");
  // Serial.println(valueB);

  dxA = aveA - valueA;
  // dxA=0;
  dxB = aveB - valueB;
  // dxB=0;

  // PgmPrint("dxA values:");
  // Serial.println(dxA);
  // PgmPrint("dxB values:");
  // Serial.println(dxB);

  if(progCounter<20) {
    if (progCounter == 0) {
      PgmPrint("Waiting to be stable");
    }
    else if (progCounter == 19) {
      PgmPrintln("done");
    }
    else {
      PgmPrint(".");
    }
    progCounter++;
  }
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
      //  PgmPrintln("No touch detected");
    }

    if((prevState==0)&&(newState==0)){
      //Serial.println("Transition state 0");
      touchState=0;
    }
    else if((prevState==0)&&(newState==1)){
      touchState=1;
    }
    else if((prevState==0)&&(newState==2)){
      touchState=2;
    }
    else if((prevState==0)&&(newState==3)){
      touchState=3;
    }
    else if((prevState==1)&&(newState==0)){
      touchState=4;
    }
    else if((prevState==2)&&(newState==0)){
      touchState=5;
    }
    else if((prevState==3)&&(newState==0)){
      touchState=6;
    }
    else if((prevState==1)&&(newState==3)){
      touchState=7;
    }
    else if((prevState==2)&&(newState==3)){
      touchState=8;
    }
    else if((prevState==3)&&(newState==1)){
      touchState=9;
    }
    else if((prevState==3)&&(newState==2)){
      touchState=10;
    }
    else if(prevState==newState){
      touchState=0;
    }
    prevState=newState;

#if !HAVE_SENSOR
    if (fakeEventTimeout < 0) {
      fakeEventTimeout = millis() + pgm_read_dword_near(fakeEventIntervals);
    }

    long currentMillis = millis();
    if ((fakePreviousMillis < fakeEventTimeout) && (fakeEventTimeout <= currentMillis)) {
      touchState = pgm_read_byte_near(fakeEventSignals + fakeEventIndex);
      fakeEventIndex++;
      fakeEventIndex %= (sizeof(fakeEventSignals) / sizeof(prog_uchar));

      fakeEventTimeout += pgm_read_dword_near(fakeEventIntervals + fakeEventIndex);
    }
    else {
      touchState = 0;
    }
    fakePreviousMillis = currentMillis;

    if (touchState) {
      Serial.print(currentMillis, DEC);
      PgmPrint("YYY");
      Serial.println(touchState, DEC);
    }
#endif

    if (touchState) {
      // Stop timer if new audio is about to play.
      timerStarted = false;
      MsTimer2::stop();

      if (actStart < 0) {
        actStart = millis();
      }

      playWave(touchState);
      delay(20);
    }
  }

  // Check if there are any new activities from the users.
  // If not, log it.
  if (!wave.isPlaying() && !timerStarted && actStart > 0) {
    timerStarted = true;
    MsTimer2::start();

    actEnd = millis();
  }

  // Calculate average.
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

//------------------------------------------------------------------------------
// calibrate
void calibrate(byte direction) {
  calibration += direction;
  //assure that calibration is in 7 bit range
  calibration &=0x7f;
  writeRegister(REGISTER_CAP_DAC_A, _BV(7) | calibration);
}

//------------------------------------------------------------------------------
// calibrate
void calibrate() {
  calibration = 0;

  PgmPrint("Calibrating CapDAC A");

  long value = readValue();

  while (value>VALUE_UPPER_BOUND && calibration < 128) {
    PgmPrint(".");
    calibration++;
    writeRegister(REGISTER_CAP_DAC_A, _BV(7) | calibration);
    value = readValue();
  }
  PgmPrintln("done");
}

//------------------------------------------------------------------------------
// state_A
void state_A()
{
  digitalWrite(nunPin1,HIGH);
  digitalWrite(nunPin2,LOW);
  state=LOW;
}

//------------------------------------------------------------------------------
// state_B
void state_B(){
  digitalWrite(nunPin1, LOW);
  digitalWrite(nunPin2, HIGH);
  state=HIGH;
}

//------------------------------------------------------------------------------
// readValue
long readValue() {
  long ret = 0;

#if HAVE_SENSOR
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
#endif

  return ret;
}
