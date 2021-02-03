#if !defined(_FAT_SYSTEM_H_)
#define _FAT_SYSTEM_H_
#include "BaseSubsys.h"
#include <SPI.h>
#include <SD.h>

class FatSystem: public BaseSubsys
{
public:
  FatSystem();

  Opcode Read(Opcode opcode, byte &ioByte);
  Opcode Write(Opcode opcode, byte ioByte);
  
//private:
  Opcode SetPath(byte ioByte);
  Opcode SetSegment(byte ioByte);

  Opcode FileExists(byte &ioByte);
  Opcode ReadNextDir(byte &ioByte);
  Opcode ReadFile(byte &ioByte);
  Opcode DeleteFile(byte &ioByte);
  Opcode MakeDir(byte &ioByte);

  Opcode WriteFile(byte ioByte);
  
private:
  struct FileInfo
  {
    char name[13];
    unsigned long size;
    byte attrib;
  };

  void CopyFileInfo(File &file, byte *buffer);
  
  File openFile;
  String filePath;
  uint8_t dirCount;
  uint16_t segment;
  byte maxIoCount;
};

#endif
