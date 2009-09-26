void displayStatus() {
  unsigned char data[18];

  readRegisters(0,18,data);

  Serial.println();
  PgmPrintln("AD7746 Registers:");
  PgmPrint("Status (0x0): ");
  Serial.println(data[0],BIN);
  PgmPrint("Cap Data (0x1-0x3): ");
  Serial.print(data[1],BIN);
  PgmPrint(".");
  Serial.print(data[2],BIN);
  PgmPrint(".");
  Serial.println(data[3],BIN);
  PgmPrint("VT Data (0x4-0x6): ");
  Serial.print(data[4],BIN);
  PgmPrint(".");
  Serial.print(data[5],BIN);
  PgmPrint(".");
  Serial.println(data[6],BIN);
  PgmPrint("Cap Setup (0x7): ");
  Serial.println(data[7],BIN);
  PgmPrint("VT Setup (0x8): ");
  Serial.println(data[8],BIN);
  PgmPrint("EXC Setup (0x9): ");
  Serial.println(data[9],BIN);
  PgmPrint("Configuration (0xa): ");
  Serial.println(data[10],BIN);
  PgmPrint("Cap Dac A (0xb): ");
  Serial.println(data[11],BIN);
  PgmPrint("Cap Dac B (0xc): ");
  Serial.println(data[12],BIN);
  PgmPrint("Cap Offset (0xd-0xe): ");
  Serial.print(data[13],BIN);
  PgmPrint(".");
  Serial.println(data[14],BIN);
  PgmPrint("Cap Gain (0xf-0x10): ");
  Serial.print(data[15],BIN);
  PgmPrint(".");
  Serial.println(data[16],BIN);
  PgmPrint("Volt Gain (0x11-0x12): ");
  Serial.print(data[17],BIN);
  PgmPrint(".");
  Serial.println(data[18],BIN);
}
