#if !defined(_OS_BOOT_INFO_H_)
#define _OS_BOOT_INFO_H_
#include <Arduino.h>

// ------------------------------------------------------------------------------
//
// File names and starting addresses
//
// ------------------------------------------------------------------------------

#define   BASICFN       "BASIC47.BIN"     // "ROM" Basic
#define   FORTHFN       "FORTH13.BIN"     // "ROM" Forth
#define   CPMFN         "CPM22.BIN"       // CP/M 2.2 loader
#define   QPMFN         "QPMLDR.BIN"      // QP/M 2.71 loader
#define   CPM3FN        "CPMLDR.COM"      // CP/M 3 CPMLDR.COM loader
#define   UCSDFN        "UCSDLDR.BIN"     // UCSD Pascal loader
#define   COSFN         "COS.BIN"         // Collapse Os loader
#define   AUTOFN        "AUTOBOOT.BIN"    // Auotobbot.bin file
#define   Z80DISK       "DSxNyy.DSK"      // Generic Z80 disk name (from DS0N00.DSK to DS9N99.DSK)
#define   DS_OSNAME     "DSxNAM.DAT"      // File with the OS name for Disk Set "x" (from DS0NAM.DAT to DSFNAM.DAT)
#define   TX_OSNAME     "DSxNAM.TXT"      // File with the OS name for Disk Set "x" (from DS0NAM.TXT to DSFNAM.TXT)
#define   BASSTRADDR    0x0000            // Starting address for the stand-alone Basic interptreter
#define   FORSTRADDR    0x0100            // Starting address for the stand-alone Forth interptreter
#define   CPM22CBASE    0xD200            // CBASE value for CP/M 2.2
#define   CPMSTRADDR    (CPM22CBASE - 32) // Starting address for CP/M 2.2
#define   QPMSTRADDR    0x80              // Starting address for the QP/M 2.71 loader
#define   CPM3STRADDR   0x100             // Starting address for the CP/M 3 loader
#define   UCSDSTRADDR   0x0000            // Starting address for the UCSD Pascal loader
#define   COSSTRADDR    0x0000            // Starting address for the Collapse Os loader
#define   AUTSTRADDR    0x0000            // Starting address for the AUTOBOOT.BIN file

struct OsBootInfo
{
  char OsName[20];
  char BootFile[13];
  word BootAddr;
};

// ------------------------------------------------------------------------------
//
//  Constants
//
// ------------------------------------------------------------------------------
const byte    LD_HL        =  0x36;       // Opcode of the Z80 instruction: LD(HL), n
const byte    INC_HL       =  0x23;       // Opcode of the Z80 instruction: INC HL
const byte    LD_HLnn      =  0x21;       // Opcode of the Z80 instruction: LD HL, nn
const byte    JP_nn        =  0xC3;       // Opcode of the Z80 instruction: JP nn

const OsBootInfo DefaultOsInfo[] PROGMEM = {
  { "CP/M 2.2", CPMFN, CPMSTRADDR },
  { "QP/M 2.71", QPMFN, QPMSTRADDR },
  { "CP/M 3.0", CPM3FN, CPM3STRADDR },
  { "UCSD Pascal", UCSDFN, UCSDSTRADDR },
  { "Collapse Os", COSFN, COSSTRADDR }
};
const byte MaxDefaultOsInfo = sizeof(DefaultOsInfo)/sizeof(OsBootInfo);

const byte maxDiskNum     = 99;          // Max number of virtual disks
const byte maxDiskSetNum  = 10;         // Restrict disk set numbers to 0-9


void printOsName(byte currentDiskSet);
byte FindLastDiskSet();
OsBootInfo GetDiskSetBootInfo(byte setNum);

#endif
