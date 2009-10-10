#ifndef PgmPrint_h
#define PgmPrint_h
#include <WProgram.h>
#include <avr/pgmspace.h>

#define PgmPrint(x) PgmSerialPrint(PSTR(x))
#define PgmPrintln(x) PgmSerialPrintln(PSTR(x))

static void PgmSerialPrint(const char *str) {
  for (uint8_t c; (c = pgm_read_byte(str)); str++) Serial.print(c);
}

static void PgmSerialPrintln(const char *str) {
  PgmSerialPrint(str);
  Serial.println();
}
#endif // PgmPrint_h
