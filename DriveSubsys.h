#if !defined(_DRIVESUBSYS_H_)
#define _DRIVESUBSYS_H_
#include "BaseSubsys.h"
#include "DiskUtils.h"

class DriveSubsys : public BaseSubsys
{
public:
  DriveSubsys();
  Opcodes Read(Opcodes opcode, byte &ioByte);
  Opcodes Write(Opcodes opcode, byte ioByte);
  void Reset(Opcodes opcode);

  const byte MaxDiskNo = 99;
  void DiskSet(byte diskNo);
  byte DiskSet();

private:
  Opcodes SelDisk(byte ioByte);
  Opcodes SelTrack(byte ioByte);
  Opcodes SelSect(byte ioByte);
  Opcodes WriteSect(byte ioByte);

  Opcodes ErrDisk(byte &ioByte);
  Opcodes ReadSect(byte &ioByte);

private:
  byte diskSet;
  char diskName[11];  // String used for virtual disk file name
  word trackSel;      // Store the current track number [0..511]
  byte sectSel;       // Store the current sector number [0..31]
  byte diskError;
  byte bufferSD[SEGMENT_SIZE];
};

#endif
