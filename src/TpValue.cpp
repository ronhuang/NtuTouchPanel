#include "TpConf.h"
#include "TpConst.h"
#include "TpValue.h"

//------------------------------------------------------------------------------
// tpValueInit
void tpValueInit(long (&initialValueA)[TP_VALUE_COUNT],
                 long (&initialValueB)[TP_VALUE_COUNT])
{
  for(int i = 0; i < TP_VALUE_COUNT; i++) {
    initialValueA[i] = 0;
    initialValueB[i] = 0;
  }
}
