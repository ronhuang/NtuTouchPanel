#include <MsTimer2.h>
#include <WaveRP.h>
#include <SdFat.h>
#include <Sd2Card.h>
#include <avr/pgmspace.h>
#include "freeRam.h"
#include "PgmPrint.h"

#include "TpConf.h"
#include "TpConst.h"

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

#define SENSOR_PIN_A 14
#define SENSOR_PIN_B 15

#define MILLIS_TO_WAIT 5000 // unit in ms.

byte touchState = 0, newState=0, prevState=0;
int sensorA=0, sensorB=0; //0=no change state, 1= detect touch, 2=remove touch
int senAstate=LOW,senBstate=LOW;
int prevValueA=0, prevValueB=0;

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
// Global variables related to wave files.
#if TP_PANEL_NUMBER == 1
prog_char wave01[] PROGMEM = "HOK-01.WAV";
prog_char wave02[] PROGMEM = "HOK-02.WAV";
prog_char wave03[] PROGMEM = "HOK-03.WAV";
prog_char wave04[] PROGMEM = "HOK-04.WAV";
prog_char wave05[] PROGMEM = "HOK-05.WAV";
prog_char wave06[] PROGMEM = "HOK-06.WAV";
prog_char wave07[] PROGMEM = "HOK-07.WAV";
prog_char wave08[] PROGMEM = "HOK-08.WAV";
prog_char wave09[] PROGMEM = "HOK-09.WAV";
prog_char wave10[] PROGMEM = "HOK-10.WAV";
#elif TP_PANEL_NUMBER == 2
prog_char wave01[] PROGMEM = "HAK-01.WAV";
prog_char wave02[] PROGMEM = "HAK-02.WAV";
prog_char wave03[] PROGMEM = "HAK-03.WAV";
prog_char wave04[] PROGMEM = "HAK-04.WAV";
prog_char wave05[] PROGMEM = "HAK-05.WAV";
prog_char wave06[] PROGMEM = "HAK-06.WAV";
prog_char wave07[] PROGMEM = "HAK-07.WAV";
prog_char wave08[] PROGMEM = "HAK-08.WAV";
prog_char wave09[] PROGMEM = "HAK-09.WAV";
prog_char wave10[] PROGMEM = "HAK-10.WAV";
#elif TP_PANEL_NUMBER == 3
prog_char wave01[] PROGMEM = "TAM-01.WAV";
prog_char wave02[] PROGMEM = "TAM-02.WAV";
prog_char wave03[] PROGMEM = "TAM-03.WAV";
prog_char wave04[] PROGMEM = "TAM-04.WAV";
prog_char wave05[] PROGMEM = "TAM-05.WAV";
prog_char wave06[] PROGMEM = "TAM-06.WAV";
prog_char wave07[] PROGMEM = "TAM-07.WAV";
prog_char wave08[] PROGMEM = "TAM-08.WAV";
prog_char wave09[] PROGMEM = "TAM-09.WAV";
prog_char wave10[] PROGMEM = "TAM-10.WAV";
#elif TP_PANEL_NUMBER == 4
prog_char wave01[] PROGMEM = "HAI-01.WAV";
prog_char wave02[] PROGMEM = "HAI-02.WAV";
prog_char wave03[] PROGMEM = "HAI-03.WAV";
prog_char wave04[] PROGMEM = "HAI-04.WAV";
prog_char wave05[] PROGMEM = "HAI-05.WAV";
prog_char wave06[] PROGMEM = "HAI-06.WAV";
prog_char wave07[] PROGMEM = "HAI-07.WAV";
prog_char wave08[] PROGMEM = "HAI-08.WAV";
prog_char wave09[] PROGMEM = "HAI-09.WAV";
prog_char wave10[] PROGMEM = "HAI-10.WAV";
#elif TP_PANEL_NUMBER == 5
prog_char wave01[] PROGMEM = "TEL-01.WAV";
prog_char wave02[] PROGMEM = "TEL-02.WAV";
prog_char wave03[] PROGMEM = "TEL-03.WAV";
prog_char wave04[] PROGMEM = "TEL-04.WAV";
prog_char wave05[] PROGMEM = "TEL-05.WAV";
prog_char wave06[] PROGMEM = "TEL-06.WAV";
prog_char wave07[] PROGMEM = "TEL-07.WAV";
prog_char wave08[] PROGMEM = "TEL-08.WAV";
prog_char wave09[] PROGMEM = "TEL-09.WAV";
prog_char wave10[] PROGMEM = "TEL-10.WAV";
#elif TP_PANEL_NUMBER == 6
prog_char wave01[] PROGMEM = "BAB-01.WAV";
prog_char wave02[] PROGMEM = "BAB-02.WAV";
prog_char wave03[] PROGMEM = "BAB-03.WAV";
prog_char wave04[] PROGMEM = "BAB-04.WAV";
prog_char wave05[] PROGMEM = "BAB-05.WAV";
prog_char wave06[] PROGMEM = "BAB-06.WAV";
prog_char wave07[] PROGMEM = "BAB-07.WAV";
prog_char wave08[] PROGMEM = "BAB-08.WAV";
prog_char wave09[] PROGMEM = "BAB-09.WAV";
prog_char wave10[] PROGMEM = "BAB-10.WAV";
#elif TP_PANEL_NUMBER == 7
prog_char wave01[] PROGMEM = "MAL-01.WAV";
prog_char wave02[] PROGMEM = "MAL-02.WAV";
prog_char wave03[] PROGMEM = "MAL-03.WAV";
prog_char wave04[] PROGMEM = "MAL-04.WAV";
prog_char wave05[] PROGMEM = "MAL-05.WAV";
prog_char wave06[] PROGMEM = "MAL-06.WAV";
prog_char wave07[] PROGMEM = "MAL-07.WAV";
prog_char wave08[] PROGMEM = "MAL-08.WAV";
prog_char wave09[] PROGMEM = "MAL-09.WAV";
prog_char wave10[] PROGMEM = "MAL-10.WAV";
#elif TP_PANEL_NUMBER == 8
prog_char wave01[] PROGMEM = "TEO-01.WAV";
prog_char wave02[] PROGMEM = "TEO-02.WAV";
prog_char wave03[] PROGMEM = "TEO-03.WAV";
prog_char wave04[] PROGMEM = "TEO-04.WAV";
prog_char wave05[] PROGMEM = "TEO-05.WAV";
prog_char wave06[] PROGMEM = "TEO-06.WAV";
prog_char wave07[] PROGMEM = "TEO-07.WAV";
prog_char wave08[] PROGMEM = "TEO-08.WAV";
prog_char wave09[] PROGMEM = "TEO-09.WAV";
prog_char wave10[] PROGMEM = "TEO-10.WAV";
#else
#error TP_PANEL_NUMBER invalid.
#endif

PROGMEM const char *TP_WAVES[] = {
  wave01, wave02, wave03, wave04, wave05,
  wave06, wave07, wave08, wave09, wave10
};

//------------------------------------------------------------------------------
// Debug
#define DEBUG 0
#define DUMP(var) \
    do {                    \
        PgmPrint(#var ":"); \
        Serial.print(var);  \
        PgmPrint(",");      \
    } while(0)

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

  pinMode(SENSOR_PIN_A, INPUT);
  pinMode(SENSOR_PIN_B, INPUT);

  digitalWrite(SENSOR_PIN_A, HIGH);
  digitalWrite(SENSOR_PIN_B, HIGH);

  Serial.begin(9600); // set up baud rate for serial

  PgmPrintln("Initializing");

  PgmPrint("Using panel: ");
  Serial.println(TP_PANEL_NUMBER);

  //wait a tad for reboot
  delay(10);

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

  // if we recieve data we do nothing
  if (Serial.available() > 0) {
    // read the incoming byte:
    Serial.read();
  }

  // wait for stable
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
    return;
  }

  // read values
  valueA = digitalRead(SENSOR_PIN_A); // active low
  delay(50);
  valueB = digitalRead(SENSOR_PIN_B); // active low
  delay(50);

#if DEBUG
  // Dump all related values.
  DUMP(valueA);
  DUMP(valueB);
  DUMP(senAstate);
  DUMP(senBstate);
  DUMP(prevValueA);
  DUMP(prevValueB);
  DUMP(sensorA);
  DUMP(sensorB);
  DUMP(newState);
  DUMP(prevState);
  DUMP(touchState);
  DUMP(timerStarted);
  DUMP(actStart);
  DUMP(actEnd);
  PgmPrintln("dump");
#endif

  // touch classifier
  if (valueA == LOW && senAstate == LOW) {
    if (prevValueA == LOW) {
      sensorA = 1; // detect touch
      senAstate = HIGH;
    }
  }
  else if (valueA == HIGH && senAstate == HIGH) {
    if (prevValueA == HIGH) {
      sensorA = 2; // remove touch
      senAstate = LOW;
    }
  }
  else {
    sensorA = 0; // no change
  }

  if (valueB == LOW && senBstate == LOW) {
    if (prevValueB == LOW) {
      sensorB = 1; // detect touch
      senBstate = HIGH;
    }
  }
  else if (valueB == HIGH && senBstate == HIGH) {
    if (prevValueB == HIGH) {
      sensorB = 2; // remove touch
      senBstate = LOW;
    }
  }
  else {
    sensorB = 0; // no change
  }

  // Classify touch state.
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

  // Check if there are any new activities from the users.
  // If not, log it.
  if (!wave.isPlaying() && !timerStarted && actStart > 0) {
    timerStarted = true;
    MsTimer2::start();

    actEnd = millis();
  }

  prevValueA = valueA;
  prevValueB = valueB;
}
