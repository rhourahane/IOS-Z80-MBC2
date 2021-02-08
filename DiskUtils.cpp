#include "HwDefines.h"
#include "DiskUtils.h"

File openSDFile;

byte mountSD()
// Mount a volume on SD:
// The returned value is the resulting status (0 = ok, otherwise see printErrSD())
{
  if (SD.begin(SS_))
  {
    return NO_ERROR;
  }
  Serial.println(F("Failed to mounted SD Card"));
  return DISK_ERR;
}


byte openSD(const char* fileName)
{
  return openSD(fileName, false);
}

byte openSD(const char* fileName, bool create)
// Open an existing file on SD:
// *  "fileName" is the pointer to the string holding the file name (8.3 format)
// The returned value is the resulting status (0 = ok, otherwise see printErrSD())
{
  //
  if ((openSDFile.name() != NULL) &&
      (strcmp(openSDFile.name(), fileName) == 0))
  {
    return NO_ERROR;
  }
  
  if (create || SD.exists(fileName))
  {
    openSDFile = SD.open(fileName, FILE_WRITE);
    if (openSDFile)
    {
      if (openSDFile.seek(0))
      {
        return NO_ERROR;
      }
      Serial.printf(F("Failed to seek to start of %s\n\r"), fileName);
      return NOT_OPENED;
    }
    Serial.printf(F("Failed to open %s\n\r"), fileName);
    return NOT_OPENED;
  }

  return NO_FILE;
}

// ------------------------------------------------------------------------------

byte readSD(void* buffSD, byte* numReadBytes)
// Read one "segment" (SEGMENT_SIZE bytes) starting from the current sector (512 bytes) of the opened file on SD:
// *  "BuffSD" is the pointer to the segment buffer;
// *  "numReadBytes" is the pointer to the variables that store the number of read bytes;
//     if < SEGMENT_SIZE (including = 0) an EOF was reached).
// The returned value is the resulting status (0 = ok, otherwise see printErrSD())
//
// NOTE1: Each SD sector (512 bytes) is divided into 16 segments (SEGMENT_SIZE bytes each); to read a sector you need to
//        to call readSD() 16 times consecutively
//
// NOTE2: Past current sector boundary, the next sector will be pointed. So to read a whole file it is sufficient 
//        call readSD() consecutively until EOF is reached
{
  if (openSDFile)
  {
    int numBytes;
    numBytes = openSDFile.read(buffSD, SEGMENT_SIZE);
    if (numBytes >= 0)
    {
      *numReadBytes = (byte)numBytes;
      return NO_ERROR;
    }
    Serial.printf(F("Failed read from %s\n\r"), openSDFile.name());
    return NOT_READY;
  }
  else
  {
    Serial.printf(F("File %s is not open\n\r"), openSDFile.name());
    return NOT_OPENED;
  }
}

// ------------------------------------------------------------------------------

byte writeSD(byte* buffSD, byte* numWrittenBytes)
// Write one "segment" (SEGMENT_SIZE bytes) starting from the current sector (512 bytes) of the opened file on SD:
// *  "BuffSD" is the pointer to the segment buffer;
// *  "numWrittenBytes" is the pointer to the variables that store the number of written bytes;
//     if < SEGMENT_SIZE (including = 0) an EOF was reached.
// The returned value is the resulting status (0 = ok, otherwise see printErrSD())
//
// NOTE1: Each SD sector (512 bytes) is divided into 16 segments (SEGMENT_SIZE bytes each); to write a sector you need to
//        to call writeSD() 16 times consecutively
//
// NOTE2: Past current sector boundary, the next sector will be pointed. So to write a whole file it is sufficient 
//        call writeSD() consecutively until EOF is reached
//
// NOTE3: To finalize the current write operation a writeSD(NULL, &numWrittenBytes) must be called as last action
{
  if (openSDFile)
  {
    size_t numBytes;

    if (buffSD != NULL)
    {
      numBytes = openSDFile.write((const char *)buffSD, SEGMENT_SIZE);
      if (numBytes >= 0)
      {
        *numWrittenBytes = (byte) numBytes;
        return NO_ERROR;
      }
      else
      {
        Serial.printf(F("Failed to write %d bytes to file %s\n\r"), numBytes, openSDFile.name());
        return NOT_READY;
      }
    }
    else
    {
      openSDFile.flush();
      return NO_ERROR;
    }
  }
  return NOT_OPENED;
}

// ------------------------------------------------------------------------------

byte seekSD(word sectNum)
// Set the pointer of the current sector for the current opened file on SD:
// *  "sectNum" is the sector number to set. First sector is 0.
// The returned value is the resulting status (0 = ok, otherwise see printErrSD())
//
// NOTE: "secNum" is in the range [0..16383], and the sector addressing is continuos inside a "disk file";
//       16383 = (512 * 32) - 1, where 512 is the number of emulated tracks, 32 is the number of emulated sectors
//
{
  if (openSDFile)
  {
    uint32_t offset = ((uint32_t)sectNum) << 9;
    if (openSDFile.seek(offset))
    {
      return NO_ERROR;
    }
    else
    {
      return NOT_READY;
    }
  }

  return NOT_OPENED;
}

// ------------------------------------------------------------------------------

void printErrSD(byte opType, byte errCode, const char* fileName)
// Print the error occurred during a SD I/O operation:
//  * "OpType" is the operation that generated the error (0 = mount, 1= open, 2 = read,
//     3 = write, 4 = seek);
//  * "errCode" is the error code from the PetitFS library (0 = no error);
//  * "fileName" is the pointer to the file name or NULL (no file name)
//
// ........................................................................
//
// Errors legend (from PetitFS library) for the implemented operations:
//
// ------------------
// mountSD():
// ------------------
// NOT_READY
//     The storage device could not be initialized due to a hard error or no medium.
// DISK_ERR
//     An error occured in the disk read function.
// NO_FILESYSTEM
//     There is no valid FAT partition on the drive.
//
// ------------------
// openSD():
// ------------------
// NO_FILE
//     Could not find the file.
// DISK_ERR
//     The function failed due to a hard error in the disk function, a wrong FAT structure or an internal error.
// NOT_ENABLED
//     The volume has not been mounted.
//
// ------------------
// readSD() and writeSD():
// ------------------
// DISK_ERR
//     The function failed due to a hard error in the disk function, a wrong FAT structure or an internal error.
// NOT_OPENED
//     The file has not been opened.
// NOT_ENABLED
//     The volume has not been mounted.
// 
// ------------------
// seekSD():
// ------------------
// DISK_ERR
//     The function failed due to an error in the disk function, a wrong FAT structure or an internal error.
// NOT_OPENED
//     The file has not been opened.
//
// ........................................................................
{
  if (errCode)
  {
    Serial.print(F("\r\nIOS: SD error "));
    Serial.print(errCode);
    Serial.print(F(" ("));
    switch (errCode)
    // See PetitFS implementation for the codes
    {
      case NO_ERROR: Serial.print(F("NO_ERROR")); break;
      case DISK_ERR: Serial.print(F("DISK_ERR")); break;
      case NOT_READY: Serial.print(F("NOT_READY")); break;
      case NO_FILE: Serial.print(F("NO_FILE")); break;
      case NOT_OPENED: Serial.print(F("NOT_OPENED")); break;
      case NOT_ENABLED: Serial.print(F("NOT_ENABLED")); break;
      case NO_FILESYSTEM: Serial.print(F("NO_FILESYSTEM")); break;
      default: Serial.print(F("UNKNOWN")); 
    }
    Serial.print(F(" on "));
    switch (opType)
    {
      case MOUNT: Serial.print(F("MOUNT")); break;
      case OPEN: Serial.print(F("OPEN")); break;
      case READ: Serial.print(F("READ")); break;
      case WRITE: Serial.print(F("WRITE")); break;
      case SEEK: Serial.print(F("SEEK")); break;
      default: Serial.print(F("UNKNOWN"));
    }
    Serial.print(F(" operation"));
    if (fileName)
    // Not a NULL pointer, so print file name too
    {
      Serial.print(F(" - File: "));
      Serial.print(fileName);
    }
    Serial.println(F(")"));
  }
}
