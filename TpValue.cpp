#include "TpConf.h"
#include "TpConst.h"
#include "TpValue.h"
#include "WProgram.h" // for map()

long initialValueA[TP_VALUE_COUNT], initialValueB[TP_VALUE_COUNT];

//------------------------------------------------------------------------------
// tpValueInit
void tpValueInit()
{
  for(int i = 0; i < TP_VALUE_COUNT; i++) {
    initialValueA[i] = 0;
    initialValueB[i] = 0;
  }
}

//------------------------------------------------------------------------------
//
void tpValueCalculateAverage(long valueA, long valueB, long &aveA, long &aveB)
{
  for(int i = TP_VALUE_COUNT - 1; i >= 1; i--){
    initialValueA[i] = initialValueA[i - 1];
    initialValueB[i] = initialValueB[i - 1];
  }
  initialValueA[0] = valueA;
  initialValueB[0] = valueB;

  aveA = (initialValueA[0] + initialValueA[1] +
          initialValueA[2] + initialValueA[3]) / 4;
  aveB = (initialValueB[0] + initialValueB[1] +
          initialValueB[2] + initialValueB[3]) / 4;
}

//------------------------------------------------------------------------------
//
long tpValueMappingA(long value)
{
#if TP_PANEL_NUMBER == 1
  return map(value, 0XFL, 16000000L, 0, 255);
#elif TP_PANEL_NUMBER == 2
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 3
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 4
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 5
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 6
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 7
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 8
  return map(value, 0XFL, 16000000L, 0, 255);
#else
#error TP_PANEL_NUMBER invalid.
#endif
}

//------------------------------------------------------------------------------
//
long tpValueMappingB(long value)
{
#if TP_PANEL_NUMBER == 1
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 2
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 3
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 4
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 5
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 6
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 7
  return map(value, 0XFL, 16000000L, 0, 300);
#elif TP_PANEL_NUMBER == 8
  return -map(value, 0, 17000000L, 0, 255);
#else
#error TP_PANEL_NUMBER invalid.
#endif
}
