#include "TpConf.h"
#include "TpConst.h"
#include "TpValue.h"

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

//
//
void tpValueCalculateAverage(long valueA, long valueB, long &aveA, long &aveB)
{
  for(int i = TP_VALUE_COUNT - 1; i >= 1; i--){
    initialValueA[i] = initialValueA[i - 1];
    initialValueB[i] = initialValueB[i - 1];
  }
  initialValueA[0] = valueA;
  initialValueB[0] = valueB;

  aveA = (initialValueA[0] + initialValueA[1] + initialValueA[2] + initialValueA[3]) / 4;
  aveB = (initialValueB[0] + initialValueB[1] + initialValueB[2] + initialValueB[3]) / 4;
}
