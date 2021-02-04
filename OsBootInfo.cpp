#include <SPI.h>            // Needed for SPI used to access SD Card
#include <SD.h>             // Needed for SD file access

#include "OsBootInfo.h"
#include "DiskUtils.h"

char OsName[11]   = DS_OSNAME;// String used for file holding the OS name
char OsNameTx[11] = TX_OSNAME;// String used for file holding the OS boot details in text Name, Boot File, Boot Address(Hex)

const char *MkOsDiskSet(byte setNum);
const char *MkTxtDiskSet(byte setNum);

void printOsName(byte currentDiskSet)
// Print the current Disk Set number and the OS name, if it is defined.
// The OS name is inside the file defined in DS_OSNAME
{
  OsBootInfo bootInfo = GetDiskSetBootInfo(currentDiskSet);
  Serial.printf(F("Disk Set %1X (%s)"), currentDiskSet, bootInfo.OsName);
}


byte FindLastDiskSet()
{
  for (int setNum = 0; setNum != maxDiskSetNum; ++setNum)
  {
    OsBootInfo bootInfo = GetDiskSetBootInfo(setNum);
    if (bootInfo.OsName[0] != '\0')
    {
        Serial.printf(F("IOS: Found Disk Set %1X (%s)\n\r"), setNum, bootInfo.OsName);
    }
    else
    {
      return setNum;
    }
  }

  return maxDiskSetNum;
}

OsBootInfo GetDiskSetBootInfo(byte setNum)
{
  if (setNum < MaxDefaultOsInfo)
  {
    OsBootInfo tmpInfo;
    memcpy_P(&tmpInfo, &(DefaultOsInfo[setNum]), sizeof(OsBootInfo));
    return tmpInfo;
  }

  OsBootInfo tmpInfo = {0, 0, 0};
  const char *binName = MkOsDiskSet(setNum);
  byte numReadBytes;

  byte bufferSD[SEGMENT_SIZE];
  byte result = openSD(binName);
  if (result == 0)
  {
    readSD(bufferSD, &numReadBytes);
    return *((OsBootInfo *)bufferSD);
  }
  else
  {
    const char *txtName = MkTxtDiskSet(setNum);
    result = openSD(txtName);
    if (result == 0)
    {
      readSD(bufferSD, &numReadBytes);
      bufferSD[numReadBytes] = '\0';
      const char* token = strtok((char *)bufferSD, "\n\r");
      if (token != NULL)
      {
        strncpy(tmpInfo.OsName, token, sizeof(tmpInfo.OsName));
        tmpInfo.OsName[sizeof(tmpInfo.OsName) - 1] = '\0';
      }
      token = strtok(NULL, "\n\r");
      if (token != NULL)
      {
        strncpy(tmpInfo.BootFile, token, sizeof(tmpInfo.BootFile));
        tmpInfo.BootFile[sizeof(tmpInfo.BootFile) - 1] = '\0';
      }
      token = strtok(NULL, "\n\r");
      if (token != NULL)
      {
        tmpInfo.BootAddr = strtol(token, NULL, 0);
      }

      // Save the new OS configuration to a binary file for faster
      // loading next time.
      const char *binName = MkOsDiskSet(setNum);
      openSDFile = SD.open(binName, FILE_WRITE);
      if (openSDFile)
      {
        openSDFile.write((const char *)(&tmpInfo), sizeof(tmpInfo));
        openSDFile.close();
      }
    }
  }
  return tmpInfo;
}

const char *MkOsDiskSet(byte setNum)
{
    OsName[2] = setNum + '0';
    return OsName;
}

const char *MkTxtDiskSet(byte setNum)
{
    OsNameTx[2] = setNum + '0';
    return OsNameTx;
}
