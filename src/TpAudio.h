#ifndef __TP_AUDIO_H__
#define __TP_AUDIO_H__

#include "TpConf.h"

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

#endif // __TP_AUDIO_H__
