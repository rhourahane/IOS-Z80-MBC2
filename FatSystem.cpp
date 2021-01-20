#include <Arduino.h>

#include "FatSystem.h"

FatSystem::FatSystem() : ioCount(0), lastOpCode(NO_OP)
{
}

byte FatSystem::SetPath(byte ioByte)
{
  if (lastOpCode != SETPATH)
  {
    filePath = "";
    lastOpCode = SETPATH;
  }
  
  if (ioByte == 0)
  {
    Serial.printf(F("Set path to %s\n\r"), filePath.c_str());
    openFile = SD.open(filePath.c_str());
    if (openFile)
    {
      Serial.printf(F("Opened file name [%s] size %d dir %d\n\r"), openFile.name(), openFile.size(), openFile.isDirectory());
    }

    lastOpCode = NO_OP;
    return NO_OP;
  }
  else
  {
      filePath.concat((char)ioByte);
  }
}

byte FatSystem::ReadNextDir(byte &ioByte)
{
  if (lastOpCode != READDIR)
  {
    memset(&fileInfo, 0, sizeof(fileInfo));
    lastOpCode = READDIR;
    if (openFile)
    {
      File entry = openFile.openNextFile();
      if (entry)
      {
        strncpy(fileInfo.name, entry.name(), 12);
        fileInfo.name[12] = '\0';
        fileInfo.size = entry.size();
        fileInfo.attrib = entry.isDirectory();
        ioCount = 0;
      }
      else
      {
        lastOpCode = NO_OP;
        return lastOpCode;
      }
    }
  }

  if (ioCount != sizeof(FileInfo))
  {
    ioByte = ((byte*)&fileInfo)[ioCount];
  }
  return lastOpCode;
}
