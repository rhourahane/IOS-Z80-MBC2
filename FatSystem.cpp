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
    openFile = SD.open(filePath.c_str());

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

void FatSystem::CopyFileInfo(File &file, FileInfo &info)
{
  strncpy(info.name, file.name(), 12);
  info.name[12] = '\0';
  info.size = file.size();
  info.attrib = file.isDirectory();
}
