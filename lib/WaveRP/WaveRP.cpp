/* Arduino WaveRP Library
 * Copyright (C) 2009 by William Greiman
 *  
 * This file is part of the Arduino WaveRP Library
 *  
 * This Library is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with the Arduino WaveRP Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <avr/interrupt.h>
#include "WProgram.h"
#include "WaveRP.h"
#include "WaveStructs.h"
#include "PgmPrint.h"
#include "mcpDac.h"

// ISR static variables

#define BUF_LENGTH 512 // must be 512, the SD block size
#define RECORD_BUFFER_LENGTH BUF_LENGTH
#define PLAY_BUFFER_LENGTH (BUF_LENGTH/2)
// For play this buffer is divided into two 256 byte buffers.
// For record this is one of two 512 byte buffers.  The other is the SD cache.
// WaveRP::trim() uses this as the SD block buffer
static uint8_t buf[BUF_LENGTH];
// ADC/DAC buffer
static uint8_t *rpBuffer;
static uint16_t rpIndex;
// SDbuffer
static uint8_t *sdBuffer;
static uint16_t sdIndex;
//------------------------------------------------------------------------------
// static ISR variables shared with WaveRP class
uint8_t WaveRP::adcMax;
uint8_t WaveRP::adcMin;
uint8_t WaveRP::bitsPerSample;
uint8_t WaveRP::busyError;
uint8_t WaveRP::Channels;  
uint8_t WaveRP::rpPause = false;
uint8_t WaveRP::rpState = RP_STATE_STOPPED;
uint32_t WaveRP::sampleRate;  
uint8_t WaveRP::sdStatus = SD_STATUS_END_OF_DATA;
SdFile *WaveRP::sdFile;
uint32_t WaveRP::sdEndPosition;
uint32_t WaveRP::sdWritePosition;
//------------------------------------------------------------------------------
// stop record or play - to be called with interrupts disabled
static void isrStop(void)
{
  ADCSRA &= ~(1 << ADIE); // disable ADC interrupt
  TIMSK1 &= ~_BV(OCIE1B); // disable DAC timer interrupt
  WaveRP::rpState = RP_STATE_STOPPED;
}
//------------------------------------------------------------------------------
// timer interrupt for DAC
ISR(TIMER1_COMPB_vect) {
  if (WaveRP::rpState != RP_STATE_PLAYING) {
    TIMSK1 &= ~_BV(OCIE1B); // disable DAC timer interrupt
    return;
  }
  // pause has not been tested
  if (WaveRP::rpPause) return;
  if (rpIndex >= PLAY_BUFFER_LENGTH) {
    if (WaveRP::sdStatus == SD_STATUS_BUFFER_READY) {
      // swap buffers
      uint8_t *tmp = rpBuffer;
      rpBuffer = sdBuffer;
      sdBuffer = tmp;
      rpIndex = sdIndex;
      WaveRP::sdStatus = SD_STATUS_BUFFER_BUSY;
      // cause interrupt to start SD read
      TIMSK1 |= _BV(OCIE1A);
    }
    else if (WaveRP::sdStatus == SD_STATUS_BUFFER_BUSY){
      if (WaveRP::busyError != 0XFF) WaveRP::busyError++;
      return;    
    }
    else {
      isrStop();
      return;
    }
  }
  uint16_t data;
  if (WaveRP::bitsPerSample == 8) {
    // 8-bit is unsigned
    data = rpBuffer[rpIndex] << 4;
    rpIndex++;
  }
  else {
    // 16-bit is signed - keep high 12 bits for DAC
    data = ((rpBuffer[rpIndex + 1] ^ 0X80) << 4) | (rpBuffer[rpIndex] >> 4);
    rpIndex += 2;
  }
  mcpDacSend(data);
}
//------------------------------------------------------------------------------
// ADC done interrupt
ISR(ADC_vect)
{
  // read data
  uint8_t sample = ADCH;
  // clear timer campare B flag
  TIFR1 &= _BV(OCF1B);  
  if (WaveRP::rpState != RP_STATE_RECORDING) {
    // disable ADC interrupt
    ADCSRA &= ~(1 << ADIE); 
    return;
  }
  // set min and max before pause 
  if(sample > WaveRP::adcMax) WaveRP::adcMax = sample;
  if(sample < WaveRP::adcMin) WaveRP::adcMin = sample;
  // pause has not been tested
  if (WaveRP::rpPause) return;
  if (rpIndex >= RECORD_BUFFER_LENGTH) {
    if (WaveRP::sdStatus == SD_STATUS_BUFFER_READY) {
     // swap buffers
      uint8_t *tmp = sdBuffer;
      sdBuffer = rpBuffer;
      rpBuffer = tmp;
      WaveRP::sdStatus= SD_STATUS_BUFFER_BUSY;
      rpIndex = 0;
      // cause interrupt to start SD write
      TIMSK1 |= _BV(OCIE1A);
    }
    else if (WaveRP::sdStatus == SD_STATUS_BUFFER_BUSY) {
      if (WaveRP::busyError != 0XFF) WaveRP::busyError++;
      return;
    }
    else {
      isrStop();
      return;
    }
  }
  rpBuffer[rpIndex++] = sample;
}
//------------------------------------------------------------------------------
// Interrupt for SD read/write
ISR(TIMER1_COMPA_vect) {
  // disable interrupt
  TIMSK1 &= ~_BV(OCIE1A);   
  if (WaveRP::sdStatus != SD_STATUS_BUFFER_BUSY) return;
  sei();
  if (WaveRP::rpState == RP_STATE_RECORDING) {
    Sd2Card *card = WaveRP::sdFile->sdCard();
    // write 512 byte data block to SD
    if(!card->writeData(sdBuffer)) {
      WaveRP::sdStatus = SD_STATUS_IO_ERROR;
      return;
    }
    WaveRP::sdWritePosition += 512;
    // check for end of preallocated file
    if (WaveRP::sdWritePosition >= WaveRP::sdEndPosition) {    
      WaveRP::sdStatus = SD_STATUS_END_OF_DATA;
      // terminate SD multiple block write
      card->writeStop();
      return;
    }
  }
  else if (WaveRP::rpState == RP_STATE_PLAYING) {
    uint16_t len = PLAY_BUFFER_LENGTH;
    uint32_t dataRemaining = WaveRP::sdEndPosition 
                               - WaveRP::sdFile->curPosition();
    if (len > dataRemaining) {
      len = dataRemaining;
      if (len == 0) {
        WaveRP::sdStatus = SD_STATUS_END_OF_DATA;
        return;
      }
    }
    sdIndex = PLAY_BUFFER_LENGTH - len;
    if (WaveRP::sdFile->read(sdBuffer + sdIndex, len) != (int16_t)len) {
      WaveRP::sdStatus = SD_STATUS_IO_ERROR;
      return;
    }
  }
  cli();
  WaveRP::sdStatus = SD_STATUS_BUFFER_READY;
}
//==============================================================================
// static helper functions
//------------------------------------------------------------------------------
// start ADC - should test results of prescaler algorithm.  Does it help?
// The idea is to allow max time for ADC to minimize noise.
static void adcInit(uint16_t rate)
{
  // Set ADC reference to AVCC
  // Left adjust ADC result to allow easy 8 bit reading
  // Set low three bits of analog pin number
  ADMUX = (1 << REFS0) | (1 << ADLAR) | (ADC_PIN & 7);
  // trigger on timer/counter 1 compare match B
  ADCSRB = (1 << ADTS2) | (1 << ADTS0);  
#if defined(__AVR_ATmega1280__)
	// the MUX5 bit of ADCSRB selects whether we're reading from channels
	// 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
  if (ADC_PIN > 7) ADCSRB |= (1 << MUX5);
#endif
  // Each sample requires 13 ADC clock cycles
  // prescaler must be smaller than preMax
  uint16_t preMax = F_CPU/(rate*13UL);
  uint8_t adps; // prescaler bits for ADCSRA
  if (preMax > 100) {
    // use prescaler of 64
    adps = (1 << ADPS2) | (1 << ADPS1);
  }
  else if (preMax > 50) {
    // use prescaler of 32
    adps = (1 << ADPS2) | (1 << ADPS0);
  }
  else if (preMax > 25) {
    // use prescaler of 16
    adps = (1 << ADPS2);
  }
  else {
    // use prescaler of 8
    adps = (1 << ADPS1) | (1 << ADPS0);
  }
  // Enable ADC, Auto trigger mode, Enable ADC Interrupt, Start A2D Conversions
  ADCSRA = (1 << ADEN) | (1 << ADATE) | (1 << ADIE) | (1 << ADSC) | adps;
}
//------------------------------------------------------------------------------
// setup 16 bit timer1
static void setRate(uint16_t rate)
{
  // no pwm
  TCCR1A = 0;     
  // no clock div, CTC mode         
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); 
  // set TOP for timer reset
  ICR1 = F_CPU/(uint32_t)rate;
  // compare for SD interrupt
  OCR1A =  10;
  // compare for DAC interrupt
  OCR1B = 1;
}
//------------------------------------------------------------------------------
// do a binary search to find recorded part of a wave file
// assumes file was pre-erased using SD erase command
static uint8_t waveSize(SdFile &file, uint32_t &size)
{
  uint32_t hi;
  uint32_t lo;
  uint16_t n;
  Sd2Card *card = file.sdCard();
  // get block range for file
  if (!file.contiguousRange(lo, hi)) return false;
  // remember first
  uint32_t first = lo;
  //find last block of recorded data
  while (lo < hi) {
    // need to add one to round up mid - matches way interval is divided below  
    uint32_t mid = (hi + lo + 1)/2;
    // read block
    if (!card->readBlock(mid, buf)) {
      return false;
    }
    // look for data - SD erase may result in all zeros or all ones
    for (n = 0; n < 512; n++) {
      if(0 < buf[n] && buf[n] < 0XFF) break;
    }
    if (n == 512) {
      // no data search lower half of interval
      hi = mid - 1; 
    }
    else {
      // search upper half of interval keeping found data in interval
      lo = mid;
    }
  }
  // read last block with data to get count for last block
  if (!card->readBlock(lo, buf)) return false;
  for ( n = 512; n > 0; n--) {
    if (0 < buf[n-1] && buf[n-1] < 0XFF) break;
  }
  size = 512*(lo - first) + n;
  return true;
}
//==============================================================================
// WaveRP member functions
//------------------------------------------------------------------------------
/** play a wave file */
uint8_t WaveRP::play(SdFile &file)
{
  // read header into buffer
  WaveHeaderExtra *header = (WaveHeaderExtra *)buf;
  // compiler will optimize these out - makes program more readable
  WaveRiff *riff = &header->riff;
  WaveFmtExtra *fmt = &header->fmt;
  WaveData *data = &header->data;
  file.rewind();
  // next two lines optimize for streaming spi read
  file.setUnbufferedRead();
  file.sdCard()->partialBlockRead(true);
  // file must start with RIFF chunck
  if (file.read((uint8_t *)riff, 12) != 12
      || strncmp_P(riff->id, PSTR("RIFF"), 4)
      || strncmp_P(riff->type, PSTR("WAVE"), 4)) {
        return false;
  }
  // fmt chunck must be next
  if (file.read((uint8_t *)riff, 8) != 8
     || strncmp_P(riff->id, PSTR("fmt "), 4)) {
        return false;
  }
  uint16_t size = riff->size;
  if (size == 16 || size == 18) {
    if (file.read((uint8_t *)&fmt->formatTag, size) != (int16_t)size) {
      return false;
    }
  }
  else {
    // don't read fmt chunk - must be compressed so cause an error
    fmt->formatTag = WAVE_FORMAT_UNKNOWN;
  }
  if (fmt->formatTag != WAVE_FORMAT_PCM) {
    PgmPrintln("Compression not supported");
    return false;
  }
  Channels = fmt->channels;
  if (Channels > 2) {
    PgmPrintln("Not mono/stereo!");
    return false;
  }
  sampleRate = fmt->sampleRate;
  bitsPerSample = fmt->bitsPerSample;
  if (bitsPerSample > 16) {
    PgmPrintln("More than 16 bits per sample!");
    return false;
  }
  uint8_t tooFast = 0; // flag
  if (sampleRate > 22050) {
    // ie 44khz
    if ((bitsPerSample > 8) || (Channels > 1)) tooFast = 1;
  } else if (sampleRate > 16000) {
    // ie 22khz. can only do 16-bit mono or 8-bit stereo
    if ((bitsPerSample > 8) && (Channels > 1)) tooFast = 1;
  }
  if (tooFast) {
    PgmPrintln("Sample rate too high!");
    return false;
  }
  // find the data chunck
  while (1) {
    // read chunk ID
    if (file.read((uint8_t *)data, 8) != 8) return false;
    if (!strncmp_P(data->id, PSTR("data"), 4)) {
      sdEndPosition = file.curPosition() + data->size;
      break;
    }
    // if not "data" then skip it!
    if (!file.seekCur(data->size)) return false;
  }
  // divide buf in two
  rpBuffer = buf;
  sdBuffer = buf + PLAY_BUFFER_LENGTH;
  // fill the buffers so SD blocks line up with buffer boundaries.
  rpIndex = file.curPosition()%PLAY_BUFFER_LENGTH;
  int16_t len = BUF_LENGTH - rpIndex;
  if (file.read(rpBuffer + rpIndex, len) != len) return false;
  if (file.read(sdBuffer, PLAY_BUFFER_LENGTH) != PLAY_BUFFER_LENGTH) return false;
  sdIndex = 0;
  busyError = 0;
  sdFile = &file;  
  sdStatus = SD_STATUS_BUFFER_READY;
  // clear pasue
  rpPause = false;
  // set ADC/DAC state
  rpState = RP_STATE_PLAYING;
  // setup timer
  setRate(sampleRate*Channels);
  // initialize DAC pins
  mcpDacInit(); 
  // enable timer interrupt for DAC
  TIMSK1 |= _BV(OCIE1B); 
  return true;
}
//------------------------------------------------------------------------------
/** record to a preallocated contiguous file */
uint8_t WaveRP::record(SdFile &file, uint16_t rate)
{
  uint32_t sdBgnBlock;
  uint32_t sdEndBlock;
  if (rate < 4000 || 44100 < rate) {
    PgmPrintln("rate not in range 4000 to 44100");
    return false;
  }
  sdFile = &file;
  Sd2Card *card = file.sdCard();
  // get raw location of file's data blocks
  if (!file.contiguousRange(sdBgnBlock, sdEndBlock)) return false;
  // tell SD card to erase all of the file's data
  if (!card->erase(sdBgnBlock, sdEndBlock)) return false;
  // setup timer
  setRate(rate);
  sdBuffer = buf;
  sdStatus = SD_STATUS_BUFFER_READY;
  // use SdVolume's cache as second buffer to save RAM
  // can't access files while recording
  if(!(rpBuffer = file.volume()->cacheClear())) return false;
  // fill in wave file header
  WaveHeader *header = (WaveHeader *)rpBuffer;
  // RIFF chunck
  strncpy_P(header->riff.id, PSTR("RIFF"), 4);
  header->riff.size = file.fileSize() - 8;
  strncpy_P(header->riff.type, PSTR("WAVE"), 4);
  // fmt chunck
  strncpy_P(header->fmt.id, PSTR("fmt "), 4);
  header->fmt.size = sizeof(WaveFmt) - 8;
  header->fmt.formatTag = WAVE_FORMAT_PCM;
  header->fmt.channels = 1;
  header->fmt.sampleRate = rate;
  header->fmt.bytesPerSecond = rate;
  header->fmt.blockAlign = 1;
  header->fmt.bitsPerSample = 8;
  //data chunck
  strncpy_P(header->data.id, PSTR("data"), 4);
  header->data.size = file.fileSize() - sizeof(WaveHeader);
  // begin position for ISR write handler
  sdWritePosition = 0;
  // end position for ISR write handler
  sdEndPosition = file.fileSize();
  // location for first sample
  rpIndex = sizeof(WaveHeader);
  busyError = 0;
  // tell the SD card where we plan to write
  if (!card->writeStart(sdBgnBlock, sdEndBlock - sdBgnBlock + 1)) return false;
  // clear pasue
  rpPause = false;
  // set ADC/DAC state
  rpState = RP_STATE_RECORDING; 
  // start the ADC
  adcInit(rate);
  return true;
}
//------------------------------------------------------------------------------
/** stop player or recorder */
void WaveRP::stop(void) {
  cli();
  uint8_t tmp = rpState;
  isrStop();
  sei();
  if (tmp == RP_STATE_RECORDING) {
    sdFile->sdCard()->writeStop();
  }
}
//------------------------------------------------------------------------------
/** trim unused space from contiguous wave file - must have been pre-erased */
uint8_t WaveRP::trim(SdFile &file)
{
  uint32_t fsize;
  // if error getting size of recording
  if (!waveSize(file, fsize)) return false;
  // if too short to have header
  if (fsize < sizeof(WaveHeader)) return false;
  if (!file.seekSet(0)) return false;
  // read header
  if (file.read(buf, sizeof(WaveHeader)) != sizeof(WaveHeader)) return false;
  WaveHeader *header = (WaveHeader *)buf;
  // check for correct size header
  if (strncmp_P(header->data.id, PSTR("data"), 4)) return false;
  // update size fields
  header->riff.size = fsize - 8;
  header->data.size = fsize - sizeof(WaveHeader);
  if (!file.seekSet(0)) return false;
  // write header back
  if (file.write(buf, sizeof(WaveHeader)) != sizeof(WaveHeader)) return false;
  // truncate file
  return file.truncate(fsize);
}
//==============================================================================
// unimplemented functions - need to decide if pause/resume works for recorder
#if COMPILE_UNTESTED
//------------------------------------------------------------------------------
/** seek - only allow if player and paused.
 *  Not debugged - do not use
 */ 
void WaveRP::seek(uint32_t pos)
{
  pos -= pos % PLAY_BUFFER_LENGTH;
  if (pos < PLAY_BUFFER_LENGTH) pos = PLAY_BUFFER_LENGTH; //don't play metadata
  if (pos > sdEndPosition) pos = sdEndPosition;
  if (isPlaying() && isPaused()) sdFile->seekSet(pos);
}
//------------------------------------------------------------------------------
/** Set sample rate - only allow for player.
 *  Not debugged - do not use
 */
void WaveRP::setSampleRate(uint32_t samplerate) 
{
  if (isPlaying()) {
    while (TCNT1 != 0);
    ICR1 = F_CPU / samplerate;
  }
}
#endif // COMPILE_UNTESTED

