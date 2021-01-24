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
    if (openFile)
    {
      openFile.close();
    }
    if (filePath.length() > 0)
    {
      openFile = SD.open(filePath.c_str(), FILE_WRITE);
    }

    lastOpCode = NO_OP;
  }
  else
  {
      filePath.concat((char)ioByte);
  }
  
  return lastOpCode;
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
    if (dirCount == 0)
    {
      openFile.rewindDirectory();
    }
    
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

byte FatSystem::SetSegment(byte ioByte)
{
  if (lastOpCode != SETSEGMENT)
  {
    ioCount = 1;
    segment = ioByte;
    lastOpCode = SETSEGMENT;
  }
  else
  {
    if (ioCount == 1)
    {
      segment += (uint16_t)ioByte << 8;
      lastOpCode = NO_OP;
    }
  }

  return lastOpCode;
}

byte FatSystem::ReadFile(byte &ioByte)
{
  if (lastOpCode != READFILE)
  {
    ioCount = 0;
    if (openFile)
    {
      if (openFile.seek(segment << 7))
      {
        maxIoCount = openFile.read(ioBuffer, sizeof(ioBuffer));
        Serial.printf(F("Read offset %d bytes read %d\n\r"), segment << 7, maxIoCount);
        if (maxIoCount > 0)
        {
          lastOpCode = READFILE;
        }
      }
      else
      {
        Serial.printf(F("Failed to seek to %d\n\r"), segment << 7);
        maxIoCount = 0;
      }
    }
    else
    {
      Serial.printf(F("File not open\n\r"));
      maxIoCount = 0;
    }
    ioByte = maxIoCount;
  }
  else
  {
    ioByte = ioBuffer[ioCount++];
    if (ioCount == maxIoCount)
    {
      Serial.printf(F("Finished reading segment of %d\n\r"), maxIoCount);
      lastOpCode = NO_OP;
    }
  }

  return lastOpCode;
}

byte FatSystem::WriteFile(byte ioByte)
{
  if (lastOpCode != WRITEFILE)
  {
    ioCount = 0;
    maxIoCount = ioByte;
  }
  else
  {
    ioByte = ioBuffer[ioCount++];
    if (ioCount == maxIoCount)
    {
      Serial.printf(F("Finished buffering segment of %d\n\r"), maxIoCount);
      lastOpCode = NO_OP;
      if (openFile)
      {
        if (openFile.seek(segment << 7))
        {
          auto written = openFile.write(ioBuffer, maxIoCount);
          Serial.printf(F("Written offset %d bytes written %d\n\r"), segment << 7, written);
          if (written != maxIoCount)
          {
            Serial.printf(F("Failed to write segment %d instead of %d\n\r"), written, maxIoCount);
          }
        }
        else
        {
          Serial.printf(F("Failed to seek to %d\n\r"), segment << 7);
        }
      }
      else
      {
        Serial.printf(F("File not open\n\r"));
      }
      lastOpCode = NO_OP;    }
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
