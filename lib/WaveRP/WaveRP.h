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
#ifndef WaveRP_h
#define WaveRP_h
#include "SdFat.h"

// analog pin for recorder
#define ADC_PIN 0
// compile functions that have not been tested
#define COMPILE_UNTESTED 1
// Values for rpState
#define RP_STATE_STOPPED 0
#define RP_STATE_PLAYING 1
#define RP_STATE_RECORDING 2
// values for sdStatus
#define SD_STATUS_BUFFER_BUSY 0
#define SD_STATUS_BUFFER_READY 1
#define SD_STATUS_END_OF_DATA  2
#define SD_STATUS_IO_ERROR 3   
//------------------------------------------------------------------------------
/**
 * \class WaveRP
 * \brief Record and play Wave files on SD and SDHC cards.
 */
class WaveRP {
public:
  // static variable shared with ISR
  /** Minimum sample value since last adcClearRange() call */
  static uint8_t adcMin;
  /** Maximum sample value since last adcClearRange() call */
  static uint8_t adcMax;
  /** number of SD busy error in ISR for ADC or DAC for current file */
  static uint8_t busyError;
  /** Number of bits per sample for file being played.  Not set for recorder.*/
  static uint8_t bitsPerSample;
  /** Number of channels, one or two, for file being played. 
   *  Not used by recorder. */
  static uint8_t Channels;
  /** recorder or player pause control - has not been tested */
  static uint8_t rpPause;
  /** state of ADC or DAC ISR */
  static uint8_t rpState;
  /** Sample rate for file being played.  Not set recorder. */
  static uint32_t sampleRate;
  /** Status of SD ISR routine. */
  static uint8_t sdStatus;
  /** SdFile for current record or play file */
  static SdFile *sdFile;
  /** File position for end of play data or max record position. */
  static uint32_t sdEndPosition;
  /** current write position for ISR raw write to SD */
  static uint32_t sdWritePosition;
  //----------------------------------------------------------------------------
  /** Clears min and max sample values. */
  void adcClearRange(void) {adcMin = 0XFF; adcMax = 0;}
  /** Returns the range of sample values since the last call to adcClearRange.*/
  uint8_t adcGetRange(void) {return adcMax < adcMin ? 0 : adcMax - adcMin;}
  /** Returns number of SD busy errors in ISRs for the ADC or DAC. */
  uint8_t errors(void) {return busyError;}
  /** Returns the pause status. */
  uint8_t isPaused(void) {return rpPause;}
  /** Returns true if the player is active. */
  uint8_t isPlaying(void) {return rpState == RP_STATE_PLAYING;}
  /** Returns true if the recorder is active. */
  uint8_t isRecording(void) {return rpState == RP_STATE_RECORDING;}
  /** Pause recorder or player. */   
  void pause(void) {rpPause = true;} 
  uint8_t play(SdFile &file);
  uint8_t record(SdFile &file, uint16_t rate);
  /** Resume recorder or player. */ 
  void resume(void) {rpPause = false;}  
  void stop(void);
  uint8_t trim(SdFile &file);
  //----------------------------------------------------------------------------
#if COMPILE_UNTESTED
  void seek(uint32_t pos);
  void setSampleRate(uint32_t samplerate);
#endif //COMPILE_UNTESTED
};
#endif // WaveRP_h