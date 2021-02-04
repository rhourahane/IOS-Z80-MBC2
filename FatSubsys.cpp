#include <Arduino.h>

#include "FatSubsys.h"

FatSystem::FatSystem()
{
}

Opcode FatSystem::Read(Opcode opcode, byte &ioByte)
{
  switch (opcode)
  {
    case READDIR:
      lastOpcode = ReadNextDir(ioByte);
      break;
      
    case READFILE:
      lastOpcode = ReadFile(ioByte);
      break;
      
    case FILEEXISTS:
      lastOpcode = FileExists(ioByte);
      break;
      
    case MKDIR:
      lastOpcode = MakeDir(ioByte);
      break;

    case DELFILE:
      lastOpcode = DeleteFile(ioByte);
      break;
  }

  return lastOpcode;
}

Opcode FatSystem::Write(Opcode opcode, byte ioByte)
{
  switch (opcode)
  {
    case SETPATH:
      lastOpcode = SetPath(ioByte);
      break;

    case SETSEGMENT:
      lastOpcode = SetSegment(ioByte);
      break;

    case WRITEFILE:
      lastOpcode = WriteFile(ioByte);
      break;
  }

  return lastOpcode;
}
  
Opcode FatSystem::SetPath(byte ioByte)
{
  if (lastOpcode != SETPATH)
  {
    filePath = "";
    lastOpcode = SETPATH;
    if (openFile)
    {
      openFile.close();
    }
  }
  
  if (ioByte == 0)
  {
    lastOpcode = NO_OP;
  }
  else
  {
      filePath.concat((char)ioByte);
  }
  
  return lastOpcode;
}

Opcode FatSystem::SetSegment(byte ioByte)
{
  if (lastOpcode != SETSEGMENT)
  {
    ioCount = 1;
    segment = ioByte;
    lastOpcode = SETSEGMENT;
  }
  else
  {
    if (ioCount == 1)
    {
      segment += (uint16_t)ioByte << 8;
      lastOpcode = NO_OP;
    }
  }

  return lastOpcode;
}

Opcode FatSystem::FileExists(byte &ioByte)
{
  ioByte = SD.exists(filePath.c_str());

  return NO_OP;
}

Opcode FatSystem::ReadNextDir(byte &ioByte)
{
  if (lastOpcode != READDIR)
  {
    if (filePath.length() > 0)
    {
      openFile = SD.open(filePath.c_str());
    }
    
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
      CopyFileInfo(entry, ioBuffer);
      ++dirCount;
      ioCount = 0;
      lastOpcode = READDIR;
    }
    else
    {
      if (dirCount == 0)
      {
        CopyFileInfo(openFile, ioBuffer);
        ioCount = 0;
        lastOpcode = READDIR;
        ++dirCount;
      }
      else
      {
        lastOpcode = NO_OP;
        ioByte = 0;
        return lastOpcode;
      }
    }
  }

  if (ioCount != sizeof(FileInfo))
  {
    ioByte = ioBuffer[ioCount++];
    if (ioCount == sizeof(FileInfo))
    {
      ioCount = 0;
    }
  }

  return lastOpcode;
}

Opcode FatSystem::ReadFile(byte &ioByte)
{
  if (lastOpcode != READFILE)
  {
    ioCount = 0;
    if (filePath.length() > 0)
    {
      openFile = SD.open(filePath.c_str());
    }
    if (openFile)
    {
      if (openFile.seek(segment << 7))
      {
        maxIoCount = openFile.read(ioBuffer, sizeof(ioBuffer));
        if (maxIoCount > 0)
        {
          lastOpcode = READFILE;
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
      lastOpcode = NO_OP;
    }
  }

  return lastOpcode;
}

Opcode FatSystem::DeleteFile(byte &ioByte)
{
  ioByte = SD.remove(filePath.c_str());
  
  return NO_OP;
}

Opcode FatSystem::MakeDir(byte &ioByte)
{
  ioByte = SD.mkdir(filePath.c_str());
  
  return NO_OP;
}

Opcode FatSystem::WriteFile(byte ioByte)
{
  if (lastOpcode != WRITEFILE)
  {
    if (filePath.length() > 0)
    {
      openFile = SD.open(filePath.c_str(), FILE_WRITE);
    }
    ioCount = 0;
    maxIoCount = ioByte <= sizeof(ioBuffer) ? ioByte : sizeof(ioBuffer);
    lastOpcode = WRITEFILE;
  }
  else
  {
    ioBuffer[ioCount++] = ioByte;
    if (ioCount == maxIoCount)
    {
      lastOpcode = NO_OP;
      if (openFile)
      {
        if (openFile.seek(segment << 7))
        {
          auto written = openFile.write(ioBuffer, maxIoCount);
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
      lastOpcode = NO_OP;    }
  }

  return lastOpcode;
}


void FatSystem::CopyFileInfo(File &file, byte *buffer)
{
  FileInfo *info = (FileInfo *)buffer;
  strncpy(info->name, file.name(), 12);
  info->name[12] = '\0';
  info->size = file.size();
  info->attrib = file.isDirectory();
}
