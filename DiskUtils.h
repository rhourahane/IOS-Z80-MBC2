#if !defined(_DISK_UTILS_H_)
#define _DISK_UTILS_H_
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// SD disk and CP/M support variables
#define SEGMENT_SIZE  128
#define BLOCK_SIZE    512
#define SECTOR_COUNT  32
#define TRACK_COUNT   512

enum DiskErrCode : byte
{
  NO_ERROR = 0,
  DISK_ERR,
  NOT_READY,
  NO_FILE,
  NOT_OPENED,
  NOT_ENABLED,
  NO_FILESYSTEM,
  BAD_DISK_NO = 16,
  BAD_TRACK_NO,
  BAD_SECTOR_NO,
  UNEXPECTED_EOF
};

enum DiskOpCode : byte
{
  MOUNT,
  OPEN,
  READ,
  WRITE,
  SEEK,
};

// I/O buffer for SD disk operations (store a "segment" of a SD sector).
// Each SD sector (512 bytes) is divided into N segments (SEGMENT_SIZE bytes each)
extern byte bufferSD[SEGMENT_SIZE];
extern File openSDFile;

byte mountSD();
byte openSD(const char* fileName);
byte openSD(const char* fileName, bool create);
byte readSD(void* buffSD, byte* numReadBytes);
byte writeSD(byte* buffSD, byte* numWrittenBytes);
byte seekSD(word sectNum);
void printErrSD(byte opType, byte errCode, const char* fileName);

#endif
