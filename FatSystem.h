#if !defined(_FAT_SYSTEM_H_)
#define _FAT_SYSTEM_H_
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "OpCodes.h"

class FatSystem
{
public:
  FatSystem();
  
  byte SetPath(byte ioByte);
  byte ReadNextDir(byte &ioByte);
  
private:
  struct FileInfo
  {
    char name[13];
    unsigned long size;
    byte attrib;
  };

  void CopyFileInfo(File &file, FileInfo &info);
  
  File openFile;
  String filePath;
  byte ioCount;
  byte lastOpCode;
  uint8_t dirCount;
  FileInfo fileInfo;
};

#endif
