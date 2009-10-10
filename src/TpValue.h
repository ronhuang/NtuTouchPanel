#ifndef __TP_VALUE_H__
#define __TP_VALUE_H__

#include "TpConf.h"
#include "TpConst.h"

extern long initialValueA[TP_VALUE_COUNT], initialValueB[TP_VALUE_COUNT];

void tpValueInit();
void tpValueCalculateAverage(long valueA, long valueB, long &aveA, long &aveB);

#endif // __TP_VALUE_H__
