#ifndef __TP_CONST_H__
#define __TP_CONST_H__

#include "TpConf.h"

#define TP_VALUE_COUNT 4

#if TP_PANEL_NUMBER == 1
#elif TP_PANEL_NUMBER == 2
#elif TP_PANEL_NUMBER == 3
#elif TP_PANEL_NUMBER == 4
#define TP_THRESHOLD_DXA_U 4
#define TP_THRESHOLD_PREVDXA_U 3
#define TP_THRESHOLD_DXA_L 4
#define TP_THRESHOLD_PREVDXA_L (-3)
#define TP_THRESHOLD_DXB_U 4
#define TP_THRESHOLD_PREVDXB_U 3
#define TP_THRESHOLD_DXB_L 3
#define TP_THRESHOLD_PREVDXB_L (-3)
#elif TP_PANEL_NUMBER == 5
#elif TP_PANEL_NUMBER == 6
#elif TP_PANEL_NUMBER == 7
#elif TP_PANEL_NUMBER == 8
#else
#error TP_PANEL_NUMBER invalid.
#endif

#endif // __TP_CONST_H__
