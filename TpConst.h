#ifndef __TP_CONST_H__
#define __TP_CONST_H__

#include "TpConf.h"

#define TP_VALUE_COUNT 4

#if TP_PANEL_NUMBER == 1
#define TP_THRESHOLD_DXA_U 4
#define TP_THRESHOLD_PREVDXA_U 0
#define TP_THRESHOLD_DXA_L 2
#define TP_THRESHOLD_PREVDXA_L (0)
#define TP_THRESHOLD_DXB_U 4
#define TP_THRESHOLD_PREVDXB_U 3
#define TP_THRESHOLD_DXB_L 2
#define TP_THRESHOLD_PREVDXB_L (-2)
#elif TP_PANEL_NUMBER == 2
#define TP_THRESHOLD_DXA_U 5
#define TP_THRESHOLD_PREVDXA_U 3
#define TP_THRESHOLD_DXA_L 3
#define TP_THRESHOLD_PREVDXA_L (-4)
#define TP_THRESHOLD_DXB_U 4
#define TP_THRESHOLD_PREVDXB_U 3
#define TP_THRESHOLD_DXB_L 2
#define TP_THRESHOLD_PREVDXB_L (-2)
#elif TP_PANEL_NUMBER == 3
#define TP_THRESHOLD_DXA_U 3
#define TP_THRESHOLD_PREVDXA_U 3
#define TP_THRESHOLD_DXA_L 3
#define TP_THRESHOLD_PREVDXA_L (-3)
#define TP_THRESHOLD_DXB_U 3
#define TP_THRESHOLD_PREVDXB_U 4
#define TP_THRESHOLD_DXB_L 4
#define TP_THRESHOLD_PREVDXB_L (-3)
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
#define TP_THRESHOLD_DXA_U 9
#define TP_THRESHOLD_PREVDXA_U 7
#define TP_THRESHOLD_DXA_L 6
#define TP_THRESHOLD_PREVDXA_L (-4)
#define TP_THRESHOLD_DXB_U 8
#define TP_THRESHOLD_PREVDXB_U 3
#define TP_THRESHOLD_DXB_L 3
#define TP_THRESHOLD_PREVDXB_L (-3)
#elif TP_PANEL_NUMBER == 6
#define TP_THRESHOLD_DXA_U 12
#define TP_THRESHOLD_PREVDXA_U 9
#define TP_THRESHOLD_DXA_L 9
#define TP_THRESHOLD_PREVDXA_L (-12)
#define TP_THRESHOLD_DXB_U 5
#define TP_THRESHOLD_PREVDXB_U 5
#define TP_THRESHOLD_DXB_L 6
#define TP_THRESHOLD_PREVDXB_L (-5)
#elif TP_PANEL_NUMBER == 7
#define TP_THRESHOLD_DXA_U 4
#define TP_THRESHOLD_PREVDXA_U 4
#define TP_THRESHOLD_DXA_L 3
#define TP_THRESHOLD_PREVDXA_L (-3)
#define TP_THRESHOLD_DXB_U 4
#define TP_THRESHOLD_PREVDXB_U 3
#define TP_THRESHOLD_DXB_L 3
#define TP_THRESHOLD_PREVDXB_L (-3)
#elif TP_PANEL_NUMBER == 8
#define TP_THRESHOLD_DXA_U 9
#define TP_THRESHOLD_PREVDXA_U 5
#define TP_THRESHOLD_DXA_L 9
#define TP_THRESHOLD_PREVDXA_L (-5)
#define TP_THRESHOLD_DXB_U 9
#define TP_THRESHOLD_PREVDXB_U 9
#define TP_THRESHOLD_DXB_L 9
#define TP_THRESHOLD_PREVDXB_L (-9)
#else
#error TP_PANEL_NUMBER invalid.
#endif

#endif // __TP_CONST_H__