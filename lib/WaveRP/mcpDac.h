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
/**
 *  inline functions for MCP4921 DAC
 */   
#ifndef mcpDac_h
#define mcpDac_h

#include <avr/io.h>

// use arduino pins 2,3,4,5 for DAC
#if defined(__AVR_ATmega1280__)
// Arduino Mega
#error pins defs not tested for Mega
#define MCP_DAC_CS_DDR  DDRE
#define MCP_DAC_CS_PORT PORTE
#define MCP_DAC_CS_BIT  4

#define MCP_DAC_SCK_DDR  DDRE
#define MCP_DAC_SCK_PORT PORTE
#define MCP_DAC_SCK_BIT  5

#define MCP_DAC_SDI_DDR  DDRG
#define MCP_DAC_SDI_PORT PORTG
#define MCP_DAC_SDI_BIT  5

#define MCP_DAC_LDAC_DDR  DDRE
#define MCP_DAC_LDAC_PORT PORTE
#define MCP_DAC_LDAC_BIT  3

#else //ATmega1280
// Arduino 168/328
#define MCP_DAC_CS_DDR  DDRD
#define MCP_DAC_CS_PORT PORTD
#define MCP_DAC_CS_BIT  2

#define MCP_DAC_SCK_DDR  DDRD
#define MCP_DAC_SCK_PORT PORTD
#define MCP_DAC_SCK_BIT  3

#define MCP_DAC_SDI_DDR  DDRD
#define MCP_DAC_SDI_PORT PORTD
#define MCP_DAC_SDI_BIT  4

#define MCP_DAC_LDAC_DDR  DDRD
#define MCP_DAC_LDAC_PORT PORTD
#define MCP_DAC_LDAC_BIT  5
#endif  //ATmega1280

#define mcpDacCsLow() MCP_DAC_CS_PORT &= ~_BV(MCP_DAC_CS_BIT)
#define mcpDacCsHigh() MCP_DAC_CS_PORT |= _BV(MCP_DAC_CS_BIT)

#define mcpDacSckLow() MCP_DAC_SCK_PORT &= ~_BV(MCP_DAC_SCK_BIT)
#define mcpDacSckHigh() MCP_DAC_SCK_PORT |= _BV(MCP_DAC_SCK_BIT)
#define mcpDacSckPulse() {mcpDacSckHigh();mcpDacSckLow();}

#define mcpDacSdiLow() MCP_DAC_SDI_PORT &= ~_BV(MCP_DAC_SDI_BIT)
#define mcpDacSdiHigh() MCP_DAC_SDI_PORT |= _BV(MCP_DAC_SDI_BIT)
#define mcpDacSdiSet(v) if(v){mcpDacSdiHigh();}else{mcpDacSdiLow();}

#define mcpDacLdacLow() MCP_DAC_LDAC_PORT &= ~_BV(MCP_DAC_LDAC_BIT)
#define mcpDacLdacHigh() MCP_DAC_LDAC_PORT |= _BV(MCP_DAC_LDAC_BIT)
// send bit b of d
#define mcpDacSendBit(d,b) {mcpDacSdiSet(d&_BV(b));mcpDacSckPulse();}
//------------------------------------------------------------------------------
// init dac I/O ports
inline void mcpDacInit(void)
{
  // set all to output mode
  MCP_DAC_CS_DDR |= _BV(MCP_DAC_CS_BIT);
  MCP_DAC_SCK_DDR |= _BV(MCP_DAC_SCK_BIT);
  MCP_DAC_SDI_DDR |= _BV(MCP_DAC_SDI_BIT);
  MCP_DAC_LDAC_DDR |= _BV(MCP_DAC_LDAC_BIT);
  
  mcpDacCsHigh();
  // LDAC low always - use unbuffered mode
  mcpDacLdacLow();
}
//------------------------------------------------------------------------------
// send 12 bits to dac
// trusted compiler to optimize and it does 
// csLow to csHigh takes 8 usec on a 16 MHz Arduino
inline void mcpDacSend(uint16_t data)
{
  mcpDacCsLow();
  // send DAC config bits
  mcpDacSdiLow();
  mcpDacSckPulse();  // DAC A
  mcpDacSckPulse();  // unbuffered
  mcpDacSdiHigh();
  mcpDacSckPulse();  // 1X gain
  mcpDacSckPulse();  // no SHDN
  // send 12 data bits
  mcpDacSendBit(data, 11);
  mcpDacSendBit(data, 10);
  mcpDacSendBit(data,  9);
  mcpDacSendBit(data,  8);  
  mcpDacSendBit(data,  7);
  mcpDacSendBit(data,  6);
  mcpDacSendBit(data,  5);
  mcpDacSendBit(data,  4);
  mcpDacSendBit(data,  3);
  mcpDacSendBit(data,  2);
  mcpDacSendBit(data,  1);
  mcpDacSendBit(data,  0);
  mcpDacCsHigh();
}
#endif //mcpDac_h