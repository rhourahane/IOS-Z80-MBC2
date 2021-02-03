#include "DriveSubsys.h"
#include "DiskUtils.h"

DriveSubsys::DriveSubsys() : trackSel(0), sectSel(0), diskError(NO_ERROR), diskName("DSxNyy.DSK")
{
  
}

Opcode DriveSubsys::Read(Opcode opcode, byte &ioByte)
{
  switch (opcode)
  {
    case ERRDISK:
      lastOpcode = ErrDisk(ioByte);
      break;

    case READSECT:
      lastOpcode = ReadSect(ioByte);
      break;
  }

  return lastOpcode;
}

Opcode DriveSubsys::Write(Opcode opcode, byte ioByte)
{
  switch (opcode)
  {
    case SELDISK:
      lastOpcode = SelDisk(ioByte);
      break;

    case SELTRACK:
      lastOpcode = SelTrack(ioByte);
      break;

    case SELSECT:
      lastOpcode = SelSect(ioByte);
      break;

    case WRITESECT:
      lastOpcode = WriteSect(ioByte);
      break;

    case SDMOUNT:
      ioByte = mountSD();
      lastOpcode = NO_OP;
  }

  return lastOpcode;
}

void DriveSubsys::DiskSet(byte diskNo)
{
  diskSet = diskNo;
}

byte DriveSubsys::DiskSet()
{
  return diskSet;
}

Opcode DriveSubsys::SelDisk(byte ioByte)
{
  if (ioByte <= MaxDiskNo)               // Valid disk number
  // Set the name of the file to open as virtual disk, and open it
  {
    diskName[2] = diskSet + 48;           // Set the current Disk Set
    diskName[4] = (ioByte / 10) + 48;     // Set the disk number
    diskName[5] = ioByte - ((ioByte / 10) * 10) + 48;

    diskError = openSD(diskName);           // Open the "disk file" corresponding to the given disk number
  }
  else
  {
    // Illegal disk number
    diskError = BAD_DISK_NO;
  }

  return NO_OP;
}

Opcode DriveSubsys::SelTrack(byte ioByte)
{
  if (lastOpcode != SELTRACK)
  {
    ioCount = 0;
    lastOpcode = SELTRACK;
  }
  
  if (ioCount == 0)
  {
    trackSel = ioByte;
  }
  else
  {
    trackSel = (((word) ioByte) << 8) | lowByte(trackSel);
    if ((trackSel < TRACK_COUNT) &&
        (sectSel < SECTOR_COUNT))
    {
      diskError = NO_ERROR;
    }
    else
    {
      if (sectSel < SECTOR_COUNT)
      {
        diskError = BAD_TRACK_NO;
      }
      else
      {
        diskError = BAD_SECTOR_NO;
      }
    }
    
    lastOpcode = NO_OP;
  }
  
  ++ioCount;

  return lastOpcode;
}

Opcode DriveSubsys::SelSect(byte ioByte)
{
  sectSel = ioByte;
  if ((trackSel < TRACK_COUNT) &&
      (sectSel < SECTOR_COUNT))
  {
    diskError = NO_ERROR;
  }
  else
  {
    if (sectSel < SECTOR_COUNT)
    {
      diskError = BAD_TRACK_NO;
    }
    else
    {
      diskError = BAD_SECTOR_NO;
    }
  }

  return NO_OP;
}

Opcode DriveSubsys::WriteSect(byte ioByte)
{
  if (lastOpcode != WRITESECT)
  {
    ioCount = 0;
    lastOpcode = WRITESECT;
  }
  
  if (ioCount == 0)
  {
    // First byte of 512, so set the right file pointer to the current emulated track/sector first
    if ((trackSel < TRACK_COUNT) &&
        (sectSel < SECTOR_COUNT) &&
        (diskError == NO_ERROR))
    {
      // Sector and track numbers valid and no previous error; set the LBA-like logical sector
      // Set the starting point inside the "disk file"
      // generating a 14 bit "disk file" LBA-like 
      // logical sector address created as TTTTTTTTTSSSSS
      diskError = seekSD((trackSel << 5) | sectSel);
    }
  }
  
  // Sector and track numbers valid and no previous error; set the LBA-like logical sector
  if (diskError == NO_ERROR)
  {
    // [0..SEGMENT_SIZE]
    byte tempByte = ioCount % SEGMENT_SIZE;
    ioBuffer[tempByte] = ioByte;
    
    // Buffer full. Write all the buffer content (SEGMENT_SIZE bytes) into the "disk file"
    if (tempByte == (SEGMENT_SIZE - 1))
    {
      byte numWritten;
      diskError = writeSD(ioBuffer, &numWritten);
      if (numWritten < SEGMENT_SIZE)
      {
        diskError = UNEXPECTED_EOF;
      }
      
      // Finalize write operation and check result (if no previous error occurred)
      if (ioCount >= (BLOCK_SIZE - 1))
      {
        if (diskError == NO_ERROR)
        {
          diskError = writeSD(NULL, &numWritten);
        }
        lastOpcode = NO_OP;
      }
    }
  }
  ioCount++;
  
  return lastOpcode;
}

Opcode DriveSubsys::ErrDisk(byte &ioByte)
{
  ioByte = diskError;
  return NO_OP;
}

Opcode DriveSubsys::ReadSect(byte &ioByte)
{
  if (lastOpcode != READSECT)
  {
    ioCount = 0;
    lastOpcode = READSECT;
  }
  
  // First byte of 512, so set the right file pointer to the current emulated track/sector first
  if (ioCount == 0)
  {
    if ((trackSel < TRACK_COUNT) &&
        (sectSel < SECTOR_COUNT) &&
        (diskError == NO_ERROR))
    {
      // Set the starting point inside the "disk file"
      // generating a 14 bit "disk file" LBA-like 
      // logical sector address created as TTTTTTTTTSSSSS
      diskError = seekSD((trackSel << 5) | sectSel);
    }
  }
  
  if (diskError == NO_ERROR)
  {
    byte tempByte = ioCount % SEGMENT_SIZE;          // [0..SEGMENT_SIZE -1]
    if (tempByte == 0)
    {
      byte numRead;
      diskError = readSD(ioBuffer, &numRead); 
      if (numRead < SEGMENT_SIZE)
      {
        diskError = UNEXPECTED_EOF;
      }
    }
    
    if (diskError == NO_ERROR)
    {
      ioByte = ioBuffer[tempByte];
    }
  }
  
  if (ioCount >= (BLOCK_SIZE - 1)) 
  {
    lastOpcode = NO_OP;
  }
  ++ioCount;

  return lastOpcode;
}
