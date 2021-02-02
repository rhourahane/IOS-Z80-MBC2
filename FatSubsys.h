#if !defined(_FAT_SYSTEM_H_)
#define _FAT_SYSTEM_H_
#include "BaseSubsys.h"
#include <SPI.h>
#include <SD.h>

class FatSystem: public BaseSubsys
{
public:
  FatSystem();

  Opcodes Read(Opcodes opcode, byte &ioByte);
  Opcodes Write(Opcodes opcode, byte ioByte);
  void Reset(Opcodes opcode);
  
//private:
  Opcodes SetPath(byte ioByte);
  Opcodes SetSegment(byte ioByte);

  Opcodes FileExists(byte &ioByte);
  Opcodes ReadNextDir(byte &ioByte);
  Opcodes ReadFile(byte &ioByte);
  Opcodes DeleteFile(byte &ioByte);
  Opcodes MakeDir(byte &ioByte);

  Opcodes WriteFile(byte ioByte);
  
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
