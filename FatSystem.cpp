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
    dirCount = 0;
    ioCount = 0;
  }

  if (openFile && ioCount == 0)
  {
    File entry = openFile.openNextFile();
    if (entry)
    {
      CopyFileInfo(entry, fileInfo);
      ++dirCount;
      ioCount = 0;
      lastOpCode = READDIR;
    }
    else
    {
      if (dirCount == 0)
      {
        CopyFileInfo(openFile, fileInfo);
        ioCount = 0;
        lastOpCode = READDIR;
        ++dirCount;
      }
      else
      {
        lastOpCode = NO_OP;
        ioByte = 0;
        return lastOpCode;
      }
    }
  }

  if (ioCount != sizeof(FileInfo))
  {
    ioByte = ((byte*)&fileInfo)[ioCount++];
    if (ioCount == sizeof(FileInfo))
    {
      ioCount = 0;
    }
  }

  return lastOpCode;
}

void FatSystem::CopyFileInfo(File &file, FileInfo &info)
{
  strncpy(info.name, file.name(), 12);
  info.name[12] = '\0';
  info.size = file.size();
  info.attrib = file.isDirectory();
}
