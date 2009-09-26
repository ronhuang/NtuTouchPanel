


void playComplete(char* fileName)
{ putstring("\nPlay complete: ");  
  Serial.print(fileName);
  mediaPlayer.play(fileName);
  while (mediaPlayer.isPlaying())
  { putstring(".");
    delay(100);
  }
}

void playComplete(const int fileNumber)
{ playComplete(mediaPlayer.fileName(fileNumber));
}

void printAvailableRAM() // 590 bytes
{ int size = 1024;
  byte *buf;
  while ((buf = (byte *) malloc(--size)) == NULL);
  free(buf);
  putstring("\nAvailable RAM: ");
  Serial.print(size);
}
 

