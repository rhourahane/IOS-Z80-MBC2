#if !defined(_DRIVESUBSYS_H_)
#define _DRIVESUBSYS_H_
#include "BaseSubsys.h"
#include "DiskUtils.h"

class DriveSubsys : public BaseSubsys
{
public:
  DriveSubsys();
  Opcode Read(Opcode opcode, byte &ioByte);
  Opcode Write(Opcode opcode, byte ioByte);

  const byte MaxDiskNo = 99;
  void DiskSet(byte diskNo);
  byte DiskSet();

private:
  Opcode SelDisk(byte ioByte);
  Opcode SelTrack(byte ioByte);
  Opcode SelSect(byte ioByte);
  Opcode WriteSect(byte ioByte);

  Opcode ErrDisk(byte &ioByte);
  Opcode ReadSect(byte &ioByte);

private:
  byte diskSet;
  char diskName[11];  // String used for virtual disk file name
  word trackSel;      // Store the current track number [0..511]
  byte sectSel;       // Store the current sector number [0..31]
  byte diskError;
};

#endif
