/* ------------------------------------------------------------------------------

HW ref: A040618

IOS - I/O  for Z80-MBC2 (Multi Boot Computer - Z80 128kB RAM @ 4/8Mhz @ Fosc = 16MHz)


Notes:

1:  This SW is ONLY for the Atmega32A used as EEPROM and I/O subsystem (16MHz external oscillator).
    
2:  Tested on Atmega32A @ Arduino IDE 1.8.12.

3:  Embedded FW: S200718 iLoad (Intel-Hex loader)

4:  To run the stand-alone Basic and Forth interpreters the SD optional module must be installed with 
    the required binary files on a microSD (FAT16 or FAT32 formatted)

5:  Utilities:   S111216 TASM conversion utility


---------------------------------------------------------------------------------
*/
#include "Wire.h"           // Needed for I2C bus
#include <EEPROM.h>         // Needed for internal EEPROM R/W
#include <SPI.h>            // Needed for SPI used to access SD Card
#include <SD.h>             // Needed for SD file access

#define HW_VERSION "A040618"
#define SW_VERSION "RMH-OS-SD"

// ------------------------------------------------------------------------------
//
// Hardware definitions for A040618 (Z80-MBC2) - Base system
//
// ------------------------------------------------------------------------------

#define   D0            24    // PA0 pin 40   Z80 data bus
#define   D1            25    // PA1 pin 39
#define   D2            26    // PA2 pin 38
#define   D3            27    // PA3 pin 37
#define   D4            28    // PA4 pin 36
#define   D5            29    // PA5 pin 35
#define   D6            30    // PA6 pin 34
#define   D7            31    // PA7 pin 33

#define   AD0           18    // PC2 pin 24   Z80 A0
#define   WR_           19    // PC3 pin 25   Z80 WR
#define   RD_           20    // PC4 pin 26   Z80 RD
#define   MREQ_         21    // PC5 pin 27   Z80 MREQ
#define   RESET_        22    // PC6 pin 28   Z80 RESET
#define   MCU_RTS_      23    // PC7 pin 29   * RESERVED - NOT USED *
#define   MCU_CTS_      10    // PD2 pin 16   * RESERVED - NOT USED *
#define   BANK1         11    // PD3 pin 17   RAM Memory bank address (High)
#define   BANK0         12    // PD4 pin 18   RAM Memory bank address (Low)
#define   INT_           1    // PB1 pin 2    Z80 control bus
#define   RAM_CE2        2    // PB2 pin 3    RAM Chip Enable (CE2). Active HIGH. Used only during boot
#define   WAIT_          3    // PB3 pin 4    Z80 WAIT
#define   SS_            4    // PB4 pin 5    SD SPI
#define   MOSI           5    // PB5 pin 6    SD SPI
#define   MISO           6    // PB6 pin 7    SD SPI
#define   SCK            7    // PB7 pin 8    SD SPI
#define   BUSREQ_       14    // PD6 pin 20   Z80 BUSRQ
#define   CLK           15    // PD7 pin 21   Z80 CLK
#define   SCL_PC0       16    // PC0 pin 22   IOEXP connector (I2C)
#define   SDA_PC1       17    // PC1 pin 23   IOEXP connector (I2C)
#define   LED_IOS        0    // PB0 pin 1    Led LED_IOS is ON if HIGH
#define   WAIT_RES_      0    // PB0 pin 1    Reset the Wait FF
#define   USER          13    // PD5 pin 19   Led USER and key (led USER is ON if LOW)

// ------------------------------------------------------------------------------
//
// Hardware definitions for A040618 GPE Option (Optional GPIO Expander)
//
// ------------------------------------------------------------------------------

#define   GPIOEXP_ADDR  0x20  // I2C module address (see datasheet)
#define   IODIRA_REG    0x00  // MCP23017 internal register IODIRA  (see datasheet)
#define   IODIRB_REG    0x01  // MCP23017 internal register IODIRB  (see datasheet)
#define   GPPUA_REG     0x0C  // MCP23017 internal register GPPUA  (see datasheet)
#define   GPPUB_REG     0x0D  // MCP23017 internal register GPPUB  (see datasheet)
#define   GPIOA_REG     0x12  // MCP23017 internal register GPIOA  (see datasheet)
#define   GPIOB_REG     0x13  // MCP23017 internal register GPIOB  (see datasheet)

// ------------------------------------------------------------------------------
//
// Hardware definitions for A040618 RTC Module Option (see DS3231 datasheet)
//
// ------------------------------------------------------------------------------

#define   DS3231_RTC    0x68  // DS3231 I2C address
#define   DS3231_SECRG  0x00  // DS3231 Seconds Register
#define   DS3231_STATRG 0x0F  // DS3231 Status Register

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

struct ConfigOptions
{
  byte Valid;               // Set to 1 if values have been saved starts as 0xFF
  
  // Use bit fields to reduce space used by flags.
  byte BootMode : 3;        // Set the program to boot (from flash or SD)
  byte AutoexecFlag : 1;    // Set to 1 if AUTOEXEC must be executed at CP/M cold boot, 0 otherwise
  byte ClockMode : 1;       // Z80 clock HI/LO speed selector (0 = 8/10MHz, 1 = 4/5MHz)
  byte DiskSet;             // Disk set to boot from if BootMode OS boot
};

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

// ------------------------------------------------------------------------------
//
// Atmega clock speed check
//
// ------------------------------------------------------------------------------

#if F_CPU == 20000000
  #define CLOCK_LOW   5
  #define CLOCK_HIGH  10
#else
  #define CLOCK_LOW   4
  #define CLOCK_HIGH  8
#endif

// ------------------------------------------------------------------------------
//
//  Constants
//
// ------------------------------------------------------------------------------

const byte    LD_HL        =  0x36;       // Opcode of the Z80 instruction: LD(HL), n
const byte    INC_HL       =  0x23;       // Opcode of the Z80 instruction: INC HL
const byte    LD_HLnn      =  0x21;       // Opcode of the Z80 instruction: LD HL, nn
const byte    JP_nn        =  0xC3;       // Opcode of the Z80 instruction: JP nn
const byte    daysOfMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const byte    debug        = 0;           // Debug off = 0, on = 1, on = 2 with interrupt trace
const byte    maxDiskNum   = 99;          // Max number of virtual disks
const byte    maxDiskSetNum = 10;         // Restrict disk set numbers to 0-9
const int     configAddr = 0;

// Z80 programs images into flash and related constants
const word  boot_A_StrAddr = 0xfd10;      // Payload A image starting address (flash)
const byte  boot_A_[] PROGMEM = {         // Payload A image (S200718 iLoad)
  0x31, 0x10, 0xFD, 0x21, 0x52, 0xFD, 0xCD, 0xC6, 0xFE, 0xCD, 0x3E, 0xFF, 0xCD, 0xF4, 0xFD, 0x3E, 
  0xFF, 0xBC, 0x20, 0x10, 0xBD, 0x20, 0x0D, 0x21, 0xD9, 0xFD, 0xCD, 0xC6, 0xFE, 0x21, 0x88, 0xFD, 
  0xCD, 0xC6, 0xFE, 0x76, 0xE5, 0x21, 0xD9, 0xFD, 0xCD, 0xC6, 0xFE, 0x21, 0x75, 0xFD, 0xCD, 0xC6, 
  0xFE, 0xE1, 0xCD, 0x4B, 0xFF, 0xCD, 0x3E, 0xFF, 0xCD, 0x3E, 0xFF, 0xDB, 0x01, 0xFE, 0xFF, 0x20, 
  0xFA, 0xE9, 0x69, 0x4C, 0x6F, 0x61, 0x64, 0x20, 0x2D, 0x20, 0x49, 0x6E, 0x74, 0x65, 0x6C, 0x2D, 
  0x48, 0x65, 0x78, 0x20, 0x4C, 0x6F, 0x61, 0x64, 0x65, 0x72, 0x20, 0x2D, 0x20, 0x53, 0x32, 0x30, 
  0x30, 0x37, 0x31, 0x38, 0x00, 0x53, 0x74, 0x61, 0x72, 0x74, 0x69, 0x6E, 0x67, 0x20, 0x41, 0x64, 
  0x64, 0x72, 0x65, 0x73, 0x73, 0x3A, 0x20, 0x00, 0x4C, 0x6F, 0x61, 0x64, 0x20, 0x65, 0x72, 0x72, 
  0x6F, 0x72, 0x20, 0x2D, 0x20, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6D, 0x20, 0x68, 0x61, 0x6C, 0x74, 
  0x65, 0x64, 0x00, 0x57, 0x61, 0x69, 0x74, 0x69, 0x6E, 0x67, 0x20, 0x69, 0x6E, 0x70, 0x75, 0x74, 
  0x20, 0x73, 0x74, 0x72, 0x65, 0x61, 0x6D, 0x2E, 0x2E, 0x2E, 0x00, 0x53, 0x79, 0x6E, 0x74, 0x61, 
  0x78, 0x20, 0x65, 0x72, 0x72, 0x6F, 0x72, 0x21, 0x00, 0x43, 0x68, 0x65, 0x63, 0x6B, 0x73, 0x75, 
  0x6D, 0x20, 0x65, 0x72, 0x72, 0x6F, 0x72, 0x21, 0x00, 0x69, 0x4C, 0x6F, 0x61, 0x64, 0x3A, 0x20, 
  0x00, 0x41, 0x64, 0x64, 0x72, 0x65, 0x73, 0x73, 0x20, 0x76, 0x69, 0x6F, 0x6C, 0x61, 0x74, 0x69, 
  0x6F, 0x6E, 0x21, 0x00, 0xF5, 0xD5, 0xC5, 0x01, 0xFF, 0xFF, 0x21, 0xA3, 0xFD, 0xCD, 0xC6, 0xFE, 
  0xCD, 0x3E, 0xFF, 0xCD, 0x72, 0xFF, 0xFE, 0x0D, 0x28, 0xF9, 0xFE, 0x0A, 0x28, 0xF5, 0xFE, 0x20, 
  0x28, 0xF1, 0xCD, 0x1A, 0xFF, 0xCD, 0x69, 0xFF, 0xFE, 0x3A, 0xC2, 0xA3, 0xFE, 0xCD, 0xE1, 0xFE, 
  0x57, 0x1E, 0x00, 0xCD, 0xBE, 0xFE, 0xCD, 0xD6, 0xFE, 0x3E, 0xFF, 0xB8, 0x20, 0x05, 0xB9, 0x20, 
  0x02, 0x44, 0x4D, 0x7C, 0xCD, 0xBE, 0xFE, 0x7D, 0xCD, 0xBE, 0xFE, 0xCD, 0xE1, 0xFE, 0xCD, 0xBE, 
  0xFE, 0xFE, 0x01, 0x20, 0x1E, 0xCD, 0xE1, 0xFE, 0xCD, 0xBE, 0xFE, 0x7B, 0xA7, 0x28, 0x66, 0xCD, 
  0x3E, 0xFF, 0x21, 0xD9, 0xFD, 0xCD, 0xC6, 0xFE, 0x21, 0xC9, 0xFD, 0xCD, 0xC6, 0xFE, 0x01, 0xFF, 
  0xFF, 0x18, 0x52, 0x7A, 0xA7, 0x28, 0x2C, 0xCD, 0xE1, 0xFE, 0xCD, 0xBE, 0xFE, 0xE5, 0xC5, 0xA7, 
  0x01, 0xF0, 0xFC, 0xED, 0x42, 0xC1, 0xE1, 0xDA, 0x8E, 0xFE, 0xCD, 0x3E, 0xFF, 0x21, 0xD9, 0xFD, 
  0xCD, 0xC6, 0xFE, 0x21, 0xE1, 0xFD, 0xCD, 0xC6, 0xFE, 0x01, 0xFF, 0xFF, 0x18, 0x27, 0x77, 0x23, 
  0x15, 0x18, 0xD0, 0xCD, 0xE1, 0xFE, 0xCD, 0xBE, 0xFE, 0x7B, 0xA7, 0x20, 0xB2, 0xCD, 0x3E, 0xFF, 
  0xC3, 0x03, 0xFE, 0xCD, 0x3E, 0xFF, 0x21, 0xD9, 0xFD, 0xCD, 0xC6, 0xFE, 0x21, 0xBB, 0xFD, 0xCD, 
  0xC6, 0xFE, 0x01, 0xFF, 0xFF, 0xCD, 0x3E, 0xFF, 0x60, 0x69, 0xC1, 0xD1, 0xF1, 0xC9, 0xC5, 0x4F, 
  0x7B, 0x91, 0x5F, 0x79, 0xC1, 0xC9, 0xF5, 0xE5, 0x7E, 0xFE, 0x00, 0x28, 0x06, 0xCD, 0x69, 0xFF, 
  0x23, 0x18, 0xF5, 0xE1, 0xF1, 0xC9, 0xF5, 0xCD, 0xE1, 0xFE, 0x67, 0xCD, 0xE1, 0xFE, 0x6F, 0xF1, 
  0xC9, 0xC5, 0xCD, 0xF4, 0xFE, 0xCB, 0x07, 0xCB, 0x07, 0xCB, 0x07, 0xCB, 0x07, 0x47, 0xCD, 0xF4, 
  0xFE, 0xB0, 0xC1, 0xC9, 0xCD, 0x72, 0xFF, 0xCD, 0x1A, 0xFF, 0xCD, 0x06, 0xFF, 0x30, 0xF5, 0xCD, 
  0x23, 0xFF, 0xCD, 0x2E, 0xFF, 0xC9, 0xFE, 0x47, 0xD0, 0xFE, 0x30, 0x30, 0x02, 0x3F, 0xC9, 0xFE, 
  0x3A, 0xD8, 0xFE, 0x41, 0x30, 0x02, 0x3F, 0xC9, 0x37, 0xC9, 0xFE, 0x61, 0xD8, 0xFE, 0x7B, 0xD0, 
  0xE6, 0x5F, 0xC9, 0xFE, 0x3A, 0x38, 0x02, 0xD6, 0x07, 0xD6, 0x30, 0xE6, 0x0F, 0xC9, 0xF5, 0xE6, 
  0x0F, 0xC6, 0x30, 0xFE, 0x3A, 0x38, 0x02, 0xC6, 0x07, 0xCD, 0x69, 0xFF, 0xF1, 0xC9, 0xF5, 0x3E, 
  0x0D, 0xCD, 0x69, 0xFF, 0x3E, 0x0A, 0xCD, 0x69, 0xFF, 0xF1, 0xC9, 0xE5, 0xF5, 0x7C, 0xCD, 0x58, 
  0xFF, 0x7D, 0xCD, 0x58, 0xFF, 0xF1, 0xE1, 0xC9, 0xF5, 0xC5, 0x47, 0x0F, 0x0F, 0x0F, 0x0F, 0xCD, 
  0x2E, 0xFF, 0x78, 0xCD, 0x2E, 0xFF, 0xC1, 0xF1, 0xC9, 0xF5, 0x3E, 0x01, 0xD3, 0x01, 0xF1, 0xD3, 
  0x00, 0xC9, 0xDB, 0x01, 0xFE, 0xFF, 0xCA, 0x72, 0xFF, 0xC9
  };
const byte * const flashBootTable[1] PROGMEM = {boot_A_}; // Payload pointers table (flash)

const OsBootInfo DefaultOsInfo[] PROGMEM = {
  { "CP/M 2.2", CPMFN, CPMSTRADDR },
  { "QP/M 2.71", QPMFN, QPMSTRADDR },
  { "CP/M 3.0", CPM3FN, CPM3STRADDR },
  { "UCSD Pascal", UCSDFN, UCSDSTRADDR },
  { "Collapse Os", COSFN, COSSTRADDR }
};
const byte MaxDefaultOsInfo = sizeof(DefaultOsInfo)/sizeof(OsBootInfo);

// ------------------------------------------------------------------------------
//
//  Global variables
//
// ------------------------------------------------------------------------------

// General purpose variables
byte          ioAddress;                  // Virtual I/O address. Only two possible addresses are valid (0x00 and 0x01)
byte          ioData;                     // Data byte used for the I/O operation
byte          ioOpcode       = 0xFF;      // I/O operation code or Opcode (0xFF means "No Operation")
word          ioByteCnt;                  // Exchanged bytes counter during an I/O operation
byte          tempByte;                   // Temporary variable (buffer)
byte          moduleGPIO     = 0;         // Set to 1 if the module is found, 0 otherwise
byte *        BootImage;                  // Pointer to selected flash payload array (image) to boot
word          BootImageSize  = 0;         // Size of the selected flash payload array (image) to boot
word          BootStrAddr;                // Starting address of the selected program to boot (from flash or SD)
byte          Z80IntEnFlag   = 0;         // Z80 INT_ enable flag (0 = No INT_ used, 1 = INT_ used for I/O)
unsigned long timeStamp;                  // Timestamp for led blinking
char          inChar;                     // Input char from serial
byte          iCount;                     // Temporary variable (counter)
byte          LastRxIsEmpty;              // "Last Rx char was empty" flag. Is set when a serial Rx operation was done
                                          // when the Rx buffer was empty

ConfigOptions SystemOptions;
OsBootInfo    BootInfo;

// DS3231 RTC variables
byte          foundRTC;                   // Set to 1 if RTC is found, 0 otherwise
byte          seconds, minutes, hours, day, month, year;
byte          tempC;                      // Temperature (Celsius) encoded in two's complement integer format

// SD disk and CP/M support variables
#define SEGMENT_SIZE  128
#define BLOCK_SIZE    512
#define SECTOR_COUNT  32
#define TRACK_COUNT   512

byte          bufferSD[SEGMENT_SIZE];     // I/O buffer for SD disk operations (store a "segment" of a SD sector).
                                          // Each SD sector (512 bytes) is divided into N segments (SEGMENT_SIZE bytes each)
const char *  fileNameSD;                 // Pointer to the string with the currently used file name
byte          errCodeSD;                  // Temporary variable to store error codes from the PetitFS
byte          numReadBytes;               // Number of read bytes after a readSD() call

// Disk emulation on SD
char          diskName[11]    = Z80DISK;  // String used for virtual disk file name
char          OsName[11]      = DS_OSNAME;// String used for file holding the OS name
char          OsNameTx[11]    = TX_OSNAME;// String used for file holding the OS boot details in text Name, Boot File, Boot Address(Hex)
word          trackSel;                   // Store the current track number [0..511]
byte          sectSel;                    // Store the current sector number [0..31]
byte          diskErr         = 19;       // SELDISK, SELSECT, SELTRACK, WRITESECT, READSECT or SDMOUNT resulting 
                                          // error code
byte          numWriBytes;                // Number of written bytes after a writeSD() call
byte          maxDiskSet      = 5;        // Number of configured Disk Sets (default to number of built in disk sets)

// ------------------------------------------------------------------------------

void setup() 
{
  
// ------------------------------------------------------------------------------
//
//  Local variables
//
// ------------------------------------------------------------------------------

  byte          data;                       // External RAM data byte
  word          address;                    // External RAM current address;
  char          minBootChar   = '1';        // Minimum allowed ASCII value selection (boot selection)
  char          maxSelChar    = '8';        // Maximum allowed ASCII value selection (boot selection)
  byte          maxBootMode   = 4;          // Default maximum allowed value for bootMode [0..4]
  byte          bootSelection = 0;          // Flag to enter into the boot mode selection

// ------------------------------------------------------------------------------

  // ----------------------------------------
  // INITIALIZATION
  // ----------------------------------------
  // Print some system information
  Serial.begin(115200);
  Serial.println(F("\r\n\nZ80-MBC2 - " HW_VERSION "\r\nIOS - I/O Subsystem - " SW_VERSION "\r\n"));

  EEPROM.get(configAddr, SystemOptions);
  if (SystemOptions.Valid != 1)
  {
    EEPROM.put(configAddr, SystemOptions);
    PrintSystemOptions(SystemOptions, F("Saved initial "));
  }
  
  // Initialize RESET_ and WAIT_RES_
  pinMode(RESET_, OUTPUT);                        // Configure RESET_ and set it ACTIVE
  digitalWrite(RESET_, LOW);
  pinMode(WAIT_RES_, OUTPUT);                     // Configure WAIT_RES_ and set it ACTIVE to reset the WAIT FF (U1C/D)
  digitalWrite(WAIT_RES_, LOW);

  // Check USER Key for boot mode changes 
  pinMode(USER, INPUT_PULLUP);                    // Read USER Key to enter into the boot mode selection
  if (!digitalRead(USER)) bootSelection = 1;

  // Initialize USER,  INT_, RAM_CE2, and BUSREQ_
  pinMode(USER, OUTPUT);                          // USER led OFF
  digitalWrite(USER, HIGH);
  pinMode(INT_, INPUT_PULLUP);                    // Configure INT_ and set it NOT ACTIVE
  pinMode(INT_, OUTPUT);
  digitalWrite(INT_, HIGH);
  pinMode(RAM_CE2, OUTPUT);                       // Configure RAM_CE2 as output
  digitalWrite(RAM_CE2, HIGH);                    // Set RAM_CE2 active
  pinMode(WAIT_, INPUT);                          // Configure WAIT_ as input
  pinMode(BUSREQ_, INPUT_PULLUP);                 // Set BUSREQ_ HIGH
  pinMode(BUSREQ_, OUTPUT);
  digitalWrite(BUSREQ_, HIGH);

  // Initialize D0-D7, AD0, MREQ_, RD_ and WR_
  DDRA = 0x00;                                    // Configure Z80 data bus D0-D7 (PA0-PA7) as input with pull-up
  PORTA = 0xFF;
  pinMode(MREQ_, INPUT_PULLUP);                   // Configure MREQ_ as input with pull-up
  pinMode(RD_, INPUT_PULLUP);                     // Configure RD_ as input with pull-up
  pinMode(WR_, INPUT_PULLUP);                     // Configure WR_ as input with pull-up
  pinMode(AD0, INPUT_PULLUP);

  // Initialize the Logical RAM Bank (32KB) to map into the lower half of the Z80 addressing space
  pinMode(BANK0, OUTPUT);                         // Set RAM Logical Bank 1 (Os Bank 0)
  digitalWrite(BANK0, HIGH);
  pinMode(BANK1, OUTPUT);
  digitalWrite(BANK1, LOW);

  // Initialize CLK (single clock pulses mode) and reset the Z80 CPU
  pinMode(CLK, OUTPUT);                           // Set CLK as output
  
  singlePulsesResetZ80();                         // Reset the Z80 CPU using single clock pulses

  // Initialize MCU_RTS and MCU_CTS and reset uTerm (A071218-R250119) if present
  pinMode(MCU_CTS_, INPUT_PULLUP);                // Parked (not used)
  pinMode(MCU_RTS_, OUTPUT);
  digitalWrite(MCU_RTS_, LOW);                    // Reset uTerm (A071218-R250119)
  delay(100); 
  digitalWrite(MCU_RTS_, HIGH); 
  delay(500);

  // Initialize the EXP_PORT (I2C) and search for "known" optional modules
  Wire.begin();                                   // Wake up I2C bus
  Wire.beginTransmission(GPIOEXP_ADDR);
  if (Wire.endTransmission() == 0) moduleGPIO = 1;// Set to 1 if GPIO Module is found
  
  // Print if the input serial buffer is 128 bytes wide (this is needed for xmodem protocol support)
  if (SERIAL_RX_BUFFER_SIZE >= 128) Serial.println(F("IOS: Found extended serial Rx buffer"));

  // Print the Z80 clock speed mode
  Serial.printf(F("IOS: Z80 clock set at %dMHz\n\r"), SystemOptions.ClockMode ? CLOCK_LOW : CLOCK_HIGH);

  // Print RTC and GPIO informations if found
  foundRTC = autoSetRTC();                        // Check if RTC is present and initialize it as needed
  if (moduleGPIO) Serial.println(F("IOS: Found GPE Option"));

  // Print CP/M Autoexec on cold boot status
  Serial.print(F("IOS: CP/M Autoexec is "));
  if (SystemOptions.AutoexecFlag) Serial.println(F("ON"));
  else Serial.println(F("OFF"));

  // ----------------------------------------
  // BOOT SELECTION AND SYS PARAMETERS MENU
  // ----------------------------------------

  // Boot selection and system parameters menu if requested
  auto mountRes = mountSD();
  if (mountRes != NO_ERROR)
  {
    mountRes = mountSD();
    if (mountRes != NO_ERROR)
    {
      printErrSD(MOUNT, mountRes, NULL);
    }
  }
  
  // Find the maximum number of disk sets
  maxDiskSet = FindLastDiskSet();
  if (SystemOptions.DiskSet >= maxDiskSet)
  {
    // Can no longer boot for pervious disk set so select
    // 0 and go into the boot menu.
    SystemOptions.DiskSet = 0;
    bootSelection = 1;
  }
  byte bootMode = SystemOptions.BootMode;

  if ((bootSelection == 1 ) || (bootMode > maxBootMode))
  // Enter in the boot selection menu if USER key was pressed at startup 
  //   or an invalid bootMode code was read from internal EEPROM
  {
    do
    {
      FlushRxBuffer();
      Serial.println();
      Serial.println(F("IOS: Select boot mode or system parameters:"));
      Serial.println();
      if (bootMode <= maxBootMode)
      // Previous valid boot mode read, so enable '0' selection
      {
        minBootChar = '0';
        Serial.printf(F(" 0: No change (%d)\r\n"), bootMode + 1);
      }
      Serial.println(F(" 1: Basic"));   
      Serial.println(F(" 2: Forth"));
      Serial.print(F(" 3: Load OS from "));
      printOsName(SystemOptions.DiskSet);
      Serial.println(F("\r\n 4: Autoboot"));
      Serial.println(F(" 5: iLoad"));
      Serial.printf(F(" 6: Change Z80 clock speed (->%dMHz)\r\n"), SystemOptions.ClockMode ? CLOCK_LOW : CLOCK_HIGH);
      Serial.print(F(" 7: Toggle CP/M Autoexec (->"));
      if (SystemOptions.AutoexecFlag) Serial.print(F("ON"));
      else Serial.print(F("OFF"));
      Serial.println(F(")"));
      Serial.print(F(" 8: Change "));
      printOsName(SystemOptions.DiskSet);
      Serial.println();
  
      // If RTC module is present add a menu choice
      if (foundRTC)
      {
        Serial.println(F(" 9: Change RTC time/date"));
        maxSelChar = '9';
      }
  
      // Ask a choice
      Serial.println();
      timeStamp = millis();
      Serial.print(F("Enter your choice >"));
      do
      {
        blinkIOSled(&timeStamp);
        inChar = Serial.read();
      }               
      while ((inChar < minBootChar) || (inChar > maxSelChar));
      Serial.print(inChar);
      Serial.println(F("  Ok"));
  
      // Make the selected action for the system paramters choice
      switch (inChar)
      {
        case '6':                                   // Change the clock speed of the Z80 CPU
          SystemOptions.ClockMode = !SystemOptions.ClockMode;                   // Toggle Z80 clock speed mode (High/Low)
        break;
  
        case '7':                                   // Toggle CP/M AUTOEXEC execution on cold boot
          SystemOptions.AutoexecFlag = !SystemOptions.AutoexecFlag;             // Toggle AUTOEXEC executiont status
        break;
  
        case '8':                                   // Change current Disk Set
          SystemOptions.DiskSet = ChangeDiskSet(SystemOptions.DiskSet);
        break;
  
        case '9':                                   // Change RTC Date/Time
          ChangeRTC();                              // Change RTC Date/Time if requested
        break;
      }
    } while (inChar > '5');
        
    // Save selectd boot program if changed
    bootMode = inChar - '1';                      // Calculate bootMode from inChar
    if (bootMode <= maxBootMode)
    {
      SystemOptions.BootMode = bootMode; // Save to the internal EEPROM if required
    }
    else
    {
      bootMode = SystemOptions.BootMode;    // Reload boot mode if '0' or > '5' choice selected
    }
  
    EEPROM.put(configAddr, SystemOptions);
    PrintSystemOptions(SystemOptions, F("Saved updated "));
  }

  // Print current Disk Set and OS name (if OS boot is enabled)
  if (bootMode == 2)
  {
    Serial.print(F("IOS: Current "));
    printOsName(SystemOptions.DiskSet);
    Serial.println();
  }

  // ----------------------------------------
  // Z80 PROGRAM LOAD
  // ----------------------------------------

  // Get the starting address of the program to load and boot, and its size if stored in the flash
  switch (bootMode)
  {
    case 0:                                       // Load Basic from SD
      fileNameSD = BASICFN;
      BootStrAddr = BASSTRADDR;
      Z80IntEnFlag = 1;                           // Enable INT_ signal generation (Z80 M1 INT I/O)
    break;
    
    case 1:                                       // Load Forth from SD
      fileNameSD = FORTHFN;
      BootStrAddr = FORSTRADDR;
    break;

    case 2:                                       // Load an OS from current Disk Set on SD
      BootInfo = GetDiskSetBootInfo(SystemOptions.DiskSet);
      fileNameSD = BootInfo.BootFile;
      BootStrAddr = BootInfo.BootAddr;
    break;
    
    case 3:                                       // Load AUTOBOOT.BIN from SD (load an user executable binary file)
      fileNameSD = AUTOFN;
      BootStrAddr = AUTSTRADDR;
    break;
    
    case 4:                                       // Load iLoad from flash
      BootImage = (byte *) pgm_read_word (&flashBootTable[0]); 
      BootImageSize = sizeof(boot_A_);
      BootStrAddr = boot_A_StrAddr;
    break;
  }
  digitalWrite(WAIT_RES_, HIGH);                  // Set WAIT_RES_ HIGH (Led LED_0 ON)
  
  // Load a JP instruction if the boot program starting addr is > 0x0000
  if (BootStrAddr > 0x0000)                       // Check if the boot program starting addr > 0x0000
  // Inject a "JP <BootStrAddr>" instruction to jump at boot starting address
  {
    loadHL(0x0000);                               // HL = 0x0000 (used as pointer to RAM)
    loadByteToRAM(JP_nn);                         // Write the JP opcode @ 0x0000;
    loadByteToRAM(lowByte(BootStrAddr));          // Write LSB to jump @ 0x0001
    loadByteToRAM(highByte(BootStrAddr));         // Write MSB to jump @ 0x0002
    //
    // DEBUG ----------------------------------
    if (debug)
    {
      Serial.printf(F("DEBUG: Injected JP 0x%0X\n\r"), BootStrAddr);
    }
    // DEBUG END ------------------------------
    //
  }

  // Execute the load of the selected file on SD or image on flash
  loadHL(BootStrAddr);                            // Set Z80 HL = boot starting address (used as pointer to RAM);
  //
  // DEBUG ----------------------------------
  if (debug)
  {
    Serial.printf(F("DEBUG: Flash BootImageSize = %d\n\r"), BootImageSize);
    Serial.printf(F("DEBUG: BootStrAddr = 0x%0X\n\r"), BootStrAddr);
  }
  // DEBUG END ------------------------------
  //
  if (bootMode < maxBootMode)
  // Load from SD
  {
    // Mount a volume on SD
    if (mountSD())
    // Error mounting. Try again
    {
      errCodeSD = mountSD();
      if (errCodeSD)
      // Error again. Repeat until error disappears (or the user forces a reset)
      do
      {
        printErrSD(MOUNT, errCodeSD, NULL);
        waitKey();                                // Wait a key to repeat
        errCodeSD = mountSD();
      }
      while (errCodeSD);
    }

    // Open the selected file to load
    errCodeSD = openSD(fileNameSD);
    if (errCodeSD)
    // Error opening the required file. Repeat until error disappears (or the user forces a reset)
    do
    {
      printErrSD(OPEN, errCodeSD, fileNameSD);
      waitKey();                                  // Wait a key to repeat
      errCodeSD = openSD(fileNameSD);
      if (errCodeSD != 3)
      // Try to do a two mount operations followed by an open
      {
        mountSD();
        errCodeSD = openSD(fileNameSD);
      }
    }
    while (errCodeSD);

    // Read the selected file from SD and load it into RAM until an EOF is reached
    Serial.printf(F("IOS: Loading boot program (%s@0x%04x)..."), fileNameSD, BootStrAddr);
    do
    // If an error occurs repeat until error disappears (or the user forces a reset)
    {
      do
      // Read a "segment" of a SD sector and load it into RAM
      {
        errCodeSD = readSD(bufferSD, &numReadBytes);  // Read current "segment" (SEGMENT_SIZE bytes) of the current SD serctor
        for (iCount = 0; iCount < numReadBytes; iCount++)
        // Load the read "segment" into RAM
        {
          loadByteToRAM(bufferSD[iCount]);        // Write current data byte into RAM
        }
      }
      while ((numReadBytes == SEGMENT_SIZE) && (!errCodeSD));   // If numReadBytes < SEGMENT_SIZE -> EOF reached
      if (errCodeSD)
      {
        printErrSD(READ, errCodeSD, fileNameSD);
        waitKey();                                // Wait a key to repeat
        seekSD(0);                                // Reset the sector pointer
      }
    }
    while (errCodeSD);
  }
  else
  // Load from flash
  {
    Serial.print(F("IOS: Loading boot program..."));
    for (word i = 0; i < BootImageSize; i++)
    // Write boot program into external RAM
    {
      loadByteToRAM(pgm_read_byte(BootImage + i));  // Write current data byte into RAM
    }
  }
  Serial.println(F(" Done"));

  // ----------------------------------------
  // Z80 BOOT
  // ----------------------------------------
  
  digitalWrite(RESET_, LOW);                      // Activate the RESET_ signal

  // Initialize CLK @ 4/8MHz (@ Fosc = 16MHz). Z80 clock_freq = (Atmega_clock) / ((OCR2 + 1) * 2)
  ASSR &= ~(1 << AS2);                    // Set Timer2 clock from system clock
#if defined(TCCR2)  
  TCCR2 |= (1 << CS20);                   // Set Timer2 clock to "no prescaling"
  TCCR2 &= ~((1 << CS21) | (1 << CS22));
  TCCR2 |= (1 << WGM21);                  // Set Timer2 CTC mode
  TCCR2 &= ~(1 << WGM20);
  TCCR2 |= (1 <<  COM20);                 // Set "toggle OC2 on compare match"
  TCCR2 &= ~(1 << COM21);
  OCR2 = SystemOptions.ClockMode;                       // Set the compare value to toggle OC2 (0 = high or 1 = low)
#else
  TCCR2B |= (1 << CS20);                  // Set Timer2 clock to "no prescaling"
  TCCR2B &= ~((1 << CS21) | (1 << CS22));
  TCCR2A |= (1 << WGM21);                 // Set Timer2 CTC mode
  TCCR2A &= ~(1 << WGM20);
  TCCR2A |= (1 <<  COM2A0);               // Set "toggle OC2 on compare match"
  TCCR2A &= ~(1 << COM2A1);
  OCR2A = SystemOptions.ClockMode;                      // Set the compare value to toggle OC2 (0 = high or 1 = low)
#endif
  pinMode(CLK, OUTPUT);                           // Set OC2 as output and start to output the clock
  Serial.println(F("IOS: Z80 is running from now"));
  Serial.println();

  FlushRxBuffer();

  // Leave the Z80 CPU running
  delay(1);                                       // Just to be sure...
  digitalWrite(RESET_, HIGH);                     // Release Z80 from reset and let it run
}

// ------------------------------------------------------------------------------

void loop() 
{
  if (!digitalRead(WAIT_))
  // I/O operaton requested
  {
    if (!digitalRead(WR_))
    // I/O WRITE operation requested

    // ----------------------------------------
    // VIRTUAL I/O WRTE OPERATIONS ENGINE
    // ----------------------------------------
    
    {
      ioAddress = digitalRead(AD0);               // Read Z80 address bus line AD0 (PC2)
      ioData = PINA;                              // Read Z80 data bus D0-D7 (PA0-PA7)
      if (ioAddress)                              // Check the I/O address (only AD0 is checked!)
      // .........................................................................................................
      //
      // AD0 = 1 (I/O write address = 0x01). STORE OPCODE.
      //
      // Store (write) an "I/O operation code" (Opcode) and reset the exchanged bytes counter.
      //
      // NOTE 1: An Opcode can be a write or read Opcode, if the I/O operation is read or write.
      // NOTE 2: the STORE OPCODE operation must always precede an EXECUTE WRITE OPCODE or EXECUTE READ OPCODE 
      //         operation.
      // NOTE 3: For multi-byte read opcode (as DATETIME) read sequentially all the data bytes without to send
      //         a STORE OPCODE operation before each data byte after the first one.
      // .........................................................................................................
      //
      // Currently defined Opcodes for I/O write operations:
      //
      //   Opcode     Name            Exchanged bytes
      // -------------------------------------------------
      // Opcode 0x00  USER LED        1
      // Opcode 0x01  SERIAL TX       1
      // Opcode 0x03  GPIOA Write     1
      // Opcode 0x04  GPIOB Write     1
      // Opcode 0x05  IODIRA Write    1
      // Opcode 0x06  IODIRB Write    1
      // Opcode 0x07  GPPUA Write     1
      // Opcode 0x08  GPPUB Write     1
      // Opcode 0x09  SELDISK         1
      // Opcode 0x0A  SELTRACK        2
      // Opcode 0x0B  SELSECT         1  
      // Opcode 0x0C  WRITESECT       512
      // Opcode 0x0D  SETBANK         1
      // Opcode 0xFF  No operation    1
      //
      //
      // Currently defined Opcodes for I/O read operations:
      //
      //   Opcode     Name            Exchanged bytes
      // -------------------------------------------------
      // Opcode 0x80  USER KEY        1
      // Opcode 0x81  GPIOA Read      1
      // Opcode 0x82  GPIOB Read      1
      // Opcode 0x83  SYSFLAGS        1
      // Opcode 0x84  DATETIME        7
      // Opcode 0x85  ERRDISK         1
      // Opcode 0x86  READSECT        512
      // Opcode 0x87  SDMOUNT         1
      // Opcode 0xFF  No operation    1
      //
      // See the following lines for the Opcodes details.
      // 
      // .........................................................................................................     
      {
        ioOpcode = ioData;                        // Store the I/O operation code (Opcode)
        ioByteCnt = 0;                            // Reset the exchanged bytes counter
      }
      else
      // .........................................................................................................
      //
      // AD0 = 0 (I/O write address = 0x00). EXECUTE WRITE OPCODE.
      //
      // Execute the previously stored I/O write opcode with the current data.
      // The code of the I/O write operation (Opcode) must be previously stored with a STORE OPCODE operation.
      // .........................................................................................................
      //
      {
        switch (ioOpcode)
        // Execute the requested I/O WRITE Opcode. The 0xFF value is reserved as "No operation".
        {
          case  0x00:
          // USER LED:      
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                              x  x  x  x  x  x  x  0    USER Led off
          //                              x  x  x  x  x  x  x  1    USER Led on
          
          if (ioData & B00000001) digitalWrite(USER, LOW); 
          else digitalWrite(USER, HIGH);
        break;

        case  0x01:
          // SERIAL TX:     
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    ASCII char to be sent to serial
          
          Serial.write(ioData);
        break;

        case  0x03:
          // GPIOA Write (GPE Option):
          //
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    GPIOA value (see MCP23017 datasheet)
          
          if (moduleGPIO) 
          {
            Wire.beginTransmission(GPIOEXP_ADDR);
            Wire.write(GPIOA_REG);                // Select GPIOA
            Wire.write(ioData);                   // Write value
            Wire.endTransmission();
          }
        break;
        
        case  0x04:
          // GPIOB Write (GPE Option): 
          //   
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    GPIOB value (see MCP23017 datasheet)
          
          if (moduleGPIO) 
          {
            Wire.beginTransmission(GPIOEXP_ADDR);
            Wire.write(GPIOB_REG);                // Select GPIOB
            Wire.write(ioData);                   // Write value
            Wire.endTransmission();
          }
        break;
        
        case  0x05:
          // IODIRA Write (GPE Option):
          //
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    IODIRA value (see MCP23017 datasheet)
          
          if (moduleGPIO) 
          {
            Wire.beginTransmission(GPIOEXP_ADDR);
            Wire.write(IODIRA_REG);               // Select IODIRA
            Wire.write(ioData);                   // Write value
            Wire.endTransmission();
          }
        break;
        
        case  0x06:
          // IODIRB Write (GPE Option):
          //
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    IODIRB value (see MCP23017 datasheet)
          
          if (moduleGPIO) 
          {
            Wire.beginTransmission(GPIOEXP_ADDR);
            Wire.write(IODIRB_REG);               // Select IODIRB
            Wire.write(ioData);                   // Write value
            Wire.endTransmission();
          }
        break;
        
        case  0x07:
          // GPPUA Write (GPE Option):
          //
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    GPPUA value (see MCP23017 datasheet)
          
          if (moduleGPIO) 
          {
            Wire.beginTransmission(GPIOEXP_ADDR);
            Wire.write(GPPUA_REG);                // Select GPPUA
            Wire.write(ioData);                   // Write value
            Wire.endTransmission();
          }
        break;
        
        case  0x08:
          // GPPUB Write (GPIO Exp. Mod. ):
          //
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    GPPUB value (see MCP23017 datasheet)
          
          if (moduleGPIO) 
          {
            Wire.beginTransmission(GPIOEXP_ADDR);
            Wire.write(GPPUB_REG);                // Select GPPUB
            Wire.write(ioData);                   // Write value
            Wire.endTransmission();
          }
        break;
        
        case  0x09:
          // DISK EMULATION
          // SELDISK - select the emulated disk number (binary). 100 disks are supported [0..99]:
          //
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    DISK number (binary) [0..99]
          //
          //
          // Opens the "disk file" correspondig to the selected disk number, doing some checks.
          // A "disk file" is a binary file that emulates a disk using a LBA-like logical sector number.
          // Every "disk file" must have a dimension of 8388608 bytes, corresponding to 16384 LBA-like logical sectors
          //  (each sector is 512 bytes long), correspinding to 512 tracks of 32 sectors each (see SELTRACK and 
          //  SELSECT opcodes).
          // Errors are stored into "errDisk" (see ERRDISK opcode).
          //
          //
          // ...........................................................................................
          //
          // "Disk file" filename convention:
          //
          // Every "disk file" must follow the sintax "DSsNnn.DSK" where
          //
          //    "s" is the "disk set" and must be in the [0..9] range (always one numeric ASCII character)
          //    "nn" is the "disk number" and must be in the [00..99] range (always two numeric ASCII characters)
          //
          // ...........................................................................................
          //          
          //
          // NOTE 1: The maximum disks number may be lower due the limitations of the used OS (e.g. CP/M 2.2 supports
          //         a maximum of 16 disks)
          // NOTE 2: Because SELDISK opens the "disk file" used for disk emulation, before using WRITESECT or READSECT
          //         a SELDISK must be performed at first.

          if (ioData <= maxDiskNum)               // Valid disk number
          // Set the name of the file to open as virtual disk, and open it
          {
            diskName[2] = SystemOptions.DiskSet + 48;           // Set the current Disk Set
            diskName[4] = (ioData / 10) + 48;     // Set the disk number
            diskName[5] = ioData - ((ioData / 10) * 10) + 48;
            diskErr = openSD(diskName);           // Open the "disk file" corresponding to the given disk number
          }
          else diskErr = BAD_DISK_NO;             // Illegal disk number
        break;

        case  0x0A:
          // DISK EMULATION
          // SELTRACK - select the emulated track number (word splitted in 2 bytes in sequence: DATA 0 and DATA 1):
          //
          //                I/O DATA 0:  D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    Track number (binary) LSB [0..255]
          //
          //                I/O DATA 1:  D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    Track number (binary) MSB [0..1]
          //
          //
          // Stores the selected track number into "trackSel" for "disk file" access.
          // A "disk file" is a binary file that emulates a disk using a LBA-like logical sector number.
          // The SELTRACK and SELSECT operations convert the legacy track/sector address into a LBA-like logical 
          //  sector number used to set the logical sector address inside the "disk file".
          // A control is performed on both current sector and track number for valid values. 
          // Errors are stored into "diskErr" (see ERRDISK opcode).
          //
          //
          // NOTE 1: Allowed track numbers are in the range [0..511] (512 tracks)
          // NOTE 2: Before a WRITESECT or READSECT operation at least a SELSECT or a SELTRAK operation
          //         must be performed

          if (!ioByteCnt)
          // LSB
          {
            trackSel = ioData;
          }
          else
          // MSB
          {
            trackSel = (((word) ioData) << 8) | lowByte(trackSel);
            if ((trackSel < TRACK_COUNT) && (sectSel < SECTOR_COUNT))
            // Sector and track numbers valid
            {
              diskErr = NO_ERROR;             // No errors
            }
            else
            // Sector or track invalid number
            {
              if (sectSel < SECTOR_COUNT)
              {
                diskErr = BAD_TRACK_NO;       // Illegal track number
              }
              else diskErr = BAD_SECTOR_NO;   // Illegal sector number
            }
            ioOpcode = 0xFF;                  // All done. Set ioOpcode = "No operation"
          }
          ioByteCnt++;
        break;

        case  0x0B:
          // DISK EMULATION
          // SELSECT - select the emulated sector number (binary):
          //
          //                  I/O DATA:  D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    Sector number (binary) [0..31]
          //
          //
          // Stores the selected sector number into "sectSel" for "disk file" access.
          // A "disk file" is a binary file that emulates a disk using a LBA-like logical sector number.
          // The SELTRACK and SELSECT operations convert the legacy track/sector address into a LBA-like logical 
          //  sector number used to set the logical sector address inside the "disk file".
          // A control is performed on both current sector and track number for valid values. 
          // Errors are stored into "diskErr" (see ERRDISK opcode).
          //
          //
          // NOTE 1: Allowed sector numbers are in the range [0..31] (32 sectors)
          // NOTE 2: Before a WRITESECT or READSECT operation at least a SELSECT or a SELTRAK operation
          //         must be performed

          sectSel = ioData;
          if ((trackSel < TRACK_COUNT) && (sectSel < SECTOR_COUNT))
          // Sector and track numbers valid
          {
            diskErr = NO_ERROR;                 // No errors
          }
          else
          // Sector or track invalid number
          {
            if (sectSel < SECTOR_COUNT)
            {
              diskErr = BAD_TRACK_NO;           // Illegal track number
            }
            else diskErr = BAD_SECTOR_NO;       // Illegal sector number
          }
        break;

        case  0x0C:
          // DISK EMULATION
          // WRITESECT - write 512 data bytes sequentially into the current emulated disk/track/sector:
          //
          //                 I/O DATA 0: D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    First Data byte
          //
          //                      |               |
          //                      |               |
          //                      |               |                 <510 Data Bytes>
          //                      |               |
          //
          //               I/O DATA 511: D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    512th Data byte (Last byte)
          //
          //
          // Writes the current sector (512 bytes) of the current track/sector, one data byte each call. 
          // All the 512 calls must be always performed sequentially to have a WRITESECT operation correctly done. 
          // If an error occurs during the WRITESECT operation, all subsequent write data will be ignored and
          //  the write finalization will not be done.
          // If an error occurs calling any DISK EMULATION opcode (SDMOUNT excluded) immediately before the WRITESECT 
          //  opcode call, all the write data will be ignored and the WRITESECT operation will not be performed.
          // Errors are stored into "diskErr" (see ERRDISK opcode).
          //
          // NOTE 1: Before a WRITESECT operation at least a SELTRACK or a SELSECT must be always performed
          // NOTE 2: Remember to open the right "disk file" at first using the SELDISK opcode
          // NOTE 3: The write finalization on SD "disk file" is executed only on the 512th data byte exchange, so be 
          //         sure that exactly 512 data bytes are exchanged.

          if (!ioByteCnt)
          // First byte of 512, so set the right file pointer to the current emulated track/sector first
          {
            if ((trackSel < 512) && (sectSel < 32) && (!diskErr))
            // Sector and track numbers valid and no previous error; set the LBA-like logical sector
            {
            diskErr = seekSD((trackSel << 5) | sectSel);  // Set the starting point inside the "disk file"
                                                          //  generating a 14 bit "disk file" LBA-like 
                                                          //  logical sector address created as TTTTTTTTTSSSSS
            }
          }
          

          if (!diskErr)
          // No previous error (e.g. selecting disk, track or sector)
          {
            tempByte = ioByteCnt % SEGMENT_SIZE;            // [0..SEGMENT_SIZE]
            bufferSD[tempByte] = ioData;          // Store current exchanged data byte in the buffer array
            if (tempByte == (SEGMENT_SIZE - 1))
            // Buffer full. Write all the buffer content (SEGMENT_SIZE bytes) into the "disk file"
            {
              diskErr = writeSD(bufferSD, &numWriBytes);
              if (numWriBytes < SEGMENT_SIZE)
      			  {
      				  diskErr = UNEXPECTED_EOF; // Reached an unexpected EOF
      			  }
              if (ioByteCnt >= (BLOCK_SIZE - 1))
              // Finalize write operation and check result (if no previous error occurred)
              {
                if (!diskErr) diskErr = writeSD(NULL, &numWriBytes);
                ioOpcode = 0xFF;                  // All done. Set ioOpcode = "No operation"
              }
            }
          }
          ioByteCnt++;                            // Increment the counter of the exchanged data bytes
        break;

        case  0x0D:
          // BANKED RAM
          // SETBANK - select the Os RAM Bank (binary):
          //
          //                  I/O DATA:  D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    Os Bank number (binary) [0..2]
          //
          //
          // Set a 32kB RAM bank for the lower half of the Z80 address space (from 0x0000 to 0x7FFF).
          // The upper half (from 0x8000 to 0xFFFF) is the common fixed bank.
          // Allowed Os Bank numbers are from 0 to 2.
          //
          // Please note that there are three kinds of Bank numbers (see the A040618 schematic):
          //
          // * the "Os Bank" number is the bank number managed (known) by the Os;
          // * the "Logical Bank" number is the bank seen by the Atmega32a (through BANK1 and BANK0 address lines);
          // * the "Physical Bank" number is the real bank addressed inside the RAM chip (RAM_A16 and RAM_A15 RAM 
          //   address lines).
          //
          // The following tables shows the relations:
          //
          //
          //  Os Bank | Logical Bank |  Z80 Address Bus    |   Physical Bank   |            Notes
          //  number  | BANK1 BANK0  |        A15          |  RAM_A16 RAM_A15  |
          // ------------------------------------------------------------------------------------------------
          //     X    |   X     X    |         1           |     0       1     |  Phy Bank 1 (common fixed)
          //     -    |   0     0    |         0           |     0       1     |  Phy Bank 1 (common fixed)
          //     0    |   0     1    |         0           |     0       0     |  Phy Bank 0 (Logical Bank 1)
          //     2    |   1     0    |         0           |     1       1     |  Phy Bank 3 (Logical Bank 2)
          //     1    |   1     1    |         0           |     1       0     |  Phy Bank 2 (Logical Bank 3)
          //
          //
          //
          //      Physical Bank      |    Logical Bank     |   Physical Bank   |   Physical RAM Addresses
          //          number         |       number        |  RAM_A16 RAM_A15  |
          // ------------------------------------------------------------------------------------------------
          //            0            |         1           |     0       0     |   From 0x00000 to 0x07FFF 
          //            1            |         0           |     0       1     |   From 0x08000 to 0x0FFFF
          //            2            |         3           |     1       0     |   From 0x01000 to 0x17FFF
          //            3            |         2           |     1       1     |   From 0x18000 to 0x1FFFF
          //
          //
          // Note that the Logical Bank 0 can't be used as switchable Os Bank bacause it is the common 
          //  fixed bank mapped in the upper half of the Z80 address space (from 0x8000 to 0xFFFF).
          //
          //
          // NOTE: If the Os Bank number is greater than 2 no selection is done.

          switch (ioData)
          {
            case 0:                               // Os bank 0
              // Set physical bank 0 (logical bank 1)
              digitalWrite(BANK0, HIGH);
              digitalWrite(BANK1, LOW);
            break;

            case 1:                               // Os bank 1
              // Set physical bank 2 (logical bank 3)
              digitalWrite(BANK0, HIGH);
              digitalWrite(BANK1, HIGH);
            break;  

            case 2:                               // Os bank 2
              // Set physical bank 3 (logical bank 2)
              digitalWrite(BANK0, LOW);
              digitalWrite(BANK1, HIGH);
            break;  
          }
        break;
        
        }
        if ((ioOpcode != 0x0A) && (ioOpcode != 0x0C)) ioOpcode = 0xFF;    // All done for the single byte opcodes. 
                                                                          //  Set ioOpcode = "No operation"
      }
      
      // Control bus sequence to exit from a wait state (M I/O write cycle)
      digitalWrite(BUSREQ_, LOW);                 // Request for a DMA
      digitalWrite(WAIT_RES_, LOW);               // Reset WAIT FF exiting from WAIT state
      digitalWrite(WAIT_RES_, HIGH);              // Now Z80 is in DMA, so it's safe set WAIT_RES_ HIGH again
      digitalWrite(BUSREQ_, HIGH);                // Resume Z80 from DMA
    }
    else 
      if (!digitalRead(RD_))
      // I/O READ operation requested

      // ----------------------------------------
      // VIRTUAL I/O READ OPERATIONS ENGINE
      // ----------------------------------------
      
      {
        ioAddress = digitalRead(AD0);             // Read Z80 address bus line AD0 (PC2)
        ioData = 0;                               // Clear input data buffer
        if (ioAddress)                            // Check the I/O address (only AD0 is checked!)
        // .........................................................................................................
        //
        // AD0 = 1 (I/O read address = 0x01). SERIAL RX.
        //
        // Execute a Serial I/O Read operation.
        // .........................................................................................................
        //
        {
          //
          // SERIAL RX:     
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    ASCII char read from serial
          //
          // NOTE 1: If there is no input char, a value 0xFF is forced as input char.
          // NOTE 2: The INT_ signal is always reset (set to HIGH) after this I/O operation.
          // NOTE 3: This is the only I/O that do not require any previous STORE OPCODE operation (for fast polling).
          // NOTE 4: A "RX buffer empty" flag and a "Last Rx char was empty" flag are available in the SYSFLAG opcode 
          //         to allow 8 bit I/O.
          //
          ioData = 0xFF;
          if (Serial.available() > 0)
          {
            ioData = Serial.read();
            LastRxIsEmpty = 0;                // Reset the "Last Rx char was empty" flag
          }
          else LastRxIsEmpty = 1;             // Set the "Last Rx char was empty" flag
          digitalWrite(INT_, HIGH);
        }
        else
        // .........................................................................................................
        //
        // AD0 = 0 (I/O read address = 0x00). EXECUTE READ OPCODE.
        //
        // Execute the previously stored I/O read operation with the current data.
        // The code of the I/O operation (Opcode) must be previously stored with a STORE OPCODE operation.
        //
        // NOTE: For multi-byte read opcode (as DATETIME) read sequentially all the data bytes without to send
        //       a STORE OPCODE operation before each data byte after the first one.
        // .........................................................................................................
        //
        {
          switch (ioOpcode)
          // Execute the requested I/O READ Opcode. The 0xFF value is reserved as "No operation".
          {
            case  0x80:
            // USER KEY:      
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              0  0  0  0  0  0  0  0    USER Key not pressed
            //                              0  0  0  0  0  0  0  1    USER Key pressed
            
            tempByte = digitalRead(USER);         // Save USER led status
            pinMode(USER, INPUT_PULLUP);          // Read USER Key
            ioData = !digitalRead(USER);
            pinMode(USER, OUTPUT); 
            digitalWrite(USER, tempByte);         // Restore USER led status
          break;

          case  0x81:
            // GPIOA Read (GPE Option):
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    GPIOA value (see MCP23017 datasheet)
            //
            // NOTE: a value 0x00 is forced if the GPE Option is not present
            
            if (moduleGPIO) 
            {
              // Set MCP23017 pointer to GPIOA
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOA_REG);
              Wire.endTransmission();
              // Read GPIOA
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.requestFrom(GPIOEXP_ADDR, 1);
              ioData = Wire.read();
            }
          break;

          case  0x82:
            // GPIOB Read (GPE Option):
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    GPIOB value (see MCP23017 datasheet)
            //
            // NOTE: a value 0x00 is forced if the GPE Option is not present
            
            if (moduleGPIO) 
            {
              // Set MCP23017 pointer to GPIOB
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOB_REG);
              Wire.endTransmission();
              // Read GPIOB
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.requestFrom(GPIOEXP_ADDR, 1);
              ioData = Wire.read();
            }
          break;

          case  0x83:
            // SYSFLAGS (Various system flags for the OS):
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              X  X  X  X  X  X  X  0    AUTOEXEC not enabled
            //                              X  X  X  X  X  X  X  1    AUTOEXEC enabled
            //                              X  X  X  X  X  X  0  X    DS3231 RTC not found
            //                              X  X  X  X  X  X  1  X    DS3231 RTC found
            //                              X  X  X  X  X  0  X  X    Serial RX buffer empty
            //                              X  X  X  X  X  1  X  X    Serial RX char available
            //                              X  X  X  X  0  X  X  X    Previous RX char valid
            //                              X  X  X  X  1  X  X  X    Previous RX char was a "buffer empty" flag
            //
            // NOTE: Currently only D0-D3 are used

            ioData = SystemOptions.AutoexecFlag | (foundRTC << 1) | ((Serial.available() > 0) << 2) | ((LastRxIsEmpty > 0) << 3);
          break;

          case  0x84:
            // DATETIME (Read date/time and temperature from the RTC. Binary values): 
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                I/O DATA 0   D7 D6 D5 D4 D3 D2 D1 D0    seconds [0..59]     (1st data byte)
            //                I/O DATA 1   D7 D6 D5 D4 D3 D2 D1 D0    minutes [0..59]
            //                I/O DATA 2   D7 D6 D5 D4 D3 D2 D1 D0    hours   [0..23]
            //                I/O DATA 3   D7 D6 D5 D4 D3 D2 D1 D0    day     [1..31]
            //                I/O DATA 4   D7 D6 D5 D4 D3 D2 D1 D0    month   [1..12]
            //                I/O DATA 5   D7 D6 D5 D4 D3 D2 D1 D0    year    [0..99]
            //                I/O DATA 6   D7 D6 D5 D4 D3 D2 D1 D0    tempC   [-128..127] (7th data byte)
            //
            // NOTE 1: If RTC is not found all read values wil be = 0
            // NOTE 2: Overread data (more then 7 bytes read) will be = 0
            // NOTE 3: The temperature (Celsius) is a byte with two complement binary format [-128..127]

            if (foundRTC)
            {
               if (ioByteCnt == 0) readRTC(&seconds, &minutes, &hours, &day, &month, &year, &tempC); // Read from RTC
               if (ioByteCnt < 7)
               // Send date/time (binary values) to Z80 bus
               {
                  switch (ioByteCnt)
                  {
                    case 0: ioData = seconds; break;
                    case 1: ioData = minutes; break;
                    case 2: ioData = hours; break;
                    case 3: ioData = day; break;
                    case 4: ioData = month; break;
                    case 5: ioData = year; break;
                    case 6: ioData = tempC; break;
                  }
                  ioByteCnt++;
               }
               else ioOpcode = 0xFF;              // All done. Set ioOpcode = "No operation"
            }
            else ioOpcode = 0xFF;                 // Nothing to do. Set ioOpcode = "No operation"
          break;

          case  0x85:
            // DISK EMULATION
            // ERRDISK - read the error code after a SELDISK, SELSECT, SELTRACK, WRITESECT, READSECT 
            //           or SDMOUNT operation
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    DISK error code (binary)
            //
            //
            // Error codes table:
            //
            //    error code    | description
            // ---------------------------------------------------------------------------------------------------
            //        0         |  No error
            //        1         |  DISK_ERR: the function failed due to a hard error in the disk function, 
            //                  |   a wrong FAT structure or an internal error
            //        2         |  NOT_READY: the storage device could not be initialized due to a hard error or 
            //                  |   no medium
            //        3         |  NO_FILE: could not find the file
            //        4         |  NOT_OPENED: the file has not been opened
            //        5         |  NOT_ENABLED: the volume has not been mounted
            //        6         |  NO_FILESYSTEM: there is no valid FAT partition on the drive
            //       16         |  Illegal disk number
            //       17         |  Illegal track number
            //       18         |  Illegal sector number
            //       19         |  Reached an unexpected EOF
            //
            //
            //
            //
            // NOTE 1: ERRDISK code is referred to the previous SELDISK, SELSECT, SELTRACK, WRITESECT or READSECT
            //         operation
            // NOTE 2: Error codes from 0 to 6 come from the PetitFS library implementation
            // NOTE 3: ERRDISK must not be used to read the resulting error code after a SDMOUNT operation 
            //         (see the SDMOUNT opcode)
             
            ioData = diskErr;
          break;

          case  0x86:
            // DISK EMULATION
            // READSECT - read 512 data bytes sequentially from the current emulated disk/track/sector:
            //
            //                 I/O DATA:   D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                 I/O DATA 0  D7 D6 D5 D4 D3 D2 D1 D0    First Data byte
            //
            //                      |               |
            //                      |               |
            //                      |               |                 <510 Data Bytes>
            //                      |               |
            //
            //               I/O DATA 127  D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    512th Data byte (Last byte)
            //
            //
            // Reads the current sector (512 bytes) of the current track/sector, one data byte each call. 
            // All the 512 calls must be always performed sequentially to have a READSECT operation correctly done. 
            // If an error occurs during the READSECT operation, all subsequent read data will be = 0.
            // If an error occurs calling any DISK EMULATION opcode (SDMOUNT excluded) immediately before the READSECT 
            //  opcode call, all the read data will be will be = 0 and the READSECT operation will not be performed.
            // Errors are stored into "diskErr" (see ERRDISK opcode).
            //
            // NOTE 1: Before a READSECT operation at least a SELTRACK or a SELSECT must be always performed
            // NOTE 2: Remember to open the right "disk file" at first using the SELDISK opcode

            if (!ioByteCnt)
            // First byte of 512, so set the right file pointer to the current emulated track/sector first
            {
              if ((trackSel < 512) && (sectSel < 32) && (!diskErr))
              // Sector and track numbers valid and no previous error; set the LBA-like logical sector
              {
              diskErr = seekSD((trackSel << 5) | sectSel);  // Set the starting point inside the "disk file"
                                                            //  generating a 14 bit "disk file" LBA-like 
                                                            //  logical sector address created as TTTTTTTTTSSSSS
              }
            }

            
            if (!diskErr)
            // No previous error (e.g. selecting disk, track or sector)
            {
              tempByte = ioByteCnt % SEGMENT_SIZE;          // [0..SEGMENT_SIZE -1]
              if (!tempByte)
              // Read SEGMENT_SIZE bytes of the current sector on SD in the buffer (every 32 calls, starting with the first)
              {
                diskErr = readSD(bufferSD, &numReadBytes); 
                if (numReadBytes < SEGMENT_SIZE)
				{
				  diskErr = 19;    // Reached an unexpected EOF
				}
              }
              if (!diskErr) ioData = bufferSD[tempByte];// If no errors, exchange current data byte with the CPU
            }
            if (ioByteCnt >= (BLOCK_SIZE - 1)) 
            {
              ioOpcode = 0xFF;                    // All done. Set ioOpcode = "No operation"
            }
            ioByteCnt++;                          // Increment the counter of the exchanged data bytes
          break;

          case  0x87:
            // DISK EMULATION
            // SDMOUNT - mount a volume on SD, returning an error code (binary):
            //
            //                 I/O DATA 0: D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    error code (binary)
            //
            //
            //
            // NOTE 1: This opcode is "normally" not used. Only needed if using a virtual disk from a custom program
            //         loaded with iLoad or with the Autoboot mode (e.g. ViDiT). Can be used to handle SD hot-swapping
            // NOTE 2: For error codes explanation see ERRDISK opcode
            // NOTE 3: Only for this disk opcode, the resulting error is read as a data byte without using the 
            //         ERRDISK opcode
            ioData = mountSD();
          break;          
          }
          if ((ioOpcode != 0x84) && (ioOpcode != 0x86)) ioOpcode = 0xFF;  // All done for the single byte opcodes. 
                                                                          //  Set ioOpcode = "No operation"
        }
        DDRA = 0xFF;                              // Configure Z80 data bus D0-D7 (PA0-PA7) as output
        PORTA = ioData;                           // Current output on data bus

        // Control bus sequence to exit from a wait state (M I/O read cycle)
        digitalWrite(BUSREQ_, LOW);               // Request for a DMA
        digitalWrite(WAIT_RES_, LOW);             // Now is safe reset WAIT FF (exiting from WAIT state)
        delayMicroseconds(2);                     // Wait 2us just to be sure that Z80 read the data and go HiZ
        DDRA = 0x00;                              // Configure Z80 data bus D0-D7 (PA0-PA7) as input with pull-up
        PORTA = 0xFF;
        digitalWrite(WAIT_RES_, HIGH);            // Now Z80 is in DMA (HiZ), so it's safe set WAIT_RES_ HIGH again
        digitalWrite(BUSREQ_, HIGH);              // Resume Z80 from DMA
      }
      else
      // INTERRUPT operation setting IORQ_ LOW, so nothing to do

      // ----------------------------------------
      // VIRTUAL INTERRUPT
      // ----------------------------------------

      // Nothing to do
      {
        //
        // DEBUG ----------------------------------
        if (debug > 2) 
        {
          Serial.println();
          Serial.println(F("DEBUG: INT operation (nothing to do)"));
        }
        // DEBUG END ------------------------------
        //
        
        // Control bus sequence to exit from a wait state (M interrupt cycle)
        digitalWrite(BUSREQ_, LOW);               // Request for a DMA
        digitalWrite(WAIT_RES_, LOW);             // Reset WAIT FF exiting from WAIT state
        digitalWrite(WAIT_RES_, HIGH);            // Now Z80 is in DMA, so it's safe set WAIT_RES_ HIGH again
        digitalWrite(BUSREQ_, HIGH);              // Resume Z80 from DMA
      }
  }
}


// ------------------------------------------------------------------------------

// Generic routines

// ------------------------------------------------------------------------------

void serialEvent()
// Set INT_ to ACTIVE if there are received chars from serial to read and if the interrupt generation is enabled
{
  if ((Serial.available()) && Z80IntEnFlag) digitalWrite(INT_, LOW);
}

// ------------------------------------------------------------------------------

void blinkIOSled(unsigned long *timestamp)
// Blink led IOS using a timestamp
{
  if ((millis() - *timestamp) > 200)
  {
    digitalWrite(LED_IOS,!digitalRead(LED_IOS));
    *timestamp = millis();
  }
}


// ------------------------------------------------------------------------------

// RTC Module routines

// ------------------------------------------------------------------------------


byte decToBcd(byte val)
// Convert a binary byte to a two digits BCD byte
{
  return( (val/10*16) + (val%10) );
}

// ------------------------------------------------------------------------------

byte bcdToDec(byte val)
// Convert binary coded decimal to normal decimal numbers
{
  return( (val/16*10) + (val%16) );
}

// ------------------------------------------------------------------------------

void readRTC(byte *second, byte *minute, byte *hour, byte *day, byte *month, byte *year, byte *tempC)
// Read current date/time binary values and the temprerature (2 complement) from the DS3231 RTC
{
  byte    i;
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_SECRG);                       // Set the DS3231 Seconds Register
  Wire.endTransmission();
  // Read from RTC and convert to binary
  Wire.requestFrom(DS3231_RTC, 18);
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  Wire.read();                                    // Jump over the DoW
  *day = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
  for (i = 0; i < 10; i++) Wire.read();           // Jump over 10 registers
  *tempC = Wire.read();
}

// ------------------------------------------------------------------------------

void writeRTC(byte second, byte minute, byte hour, byte day, byte month, byte year)
// Write given date/time binary values to the DS3231 RTC
{
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_SECRG);                       // Set the DS3231 Seconds Register
  Wire.write(decToBcd(seconds));
  Wire.write(decToBcd(minutes));
  Wire.write(decToBcd(hours));
  Wire.write(1);                                  // Day of week not used (always set to 1 = Sunday)
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

// ------------------------------------------------------------------------------

byte autoSetRTC()
// Check if the DS3231 RTC is present and set the date/time at compile date/time if 
// the RTC "Oscillator Stop Flag" is set (= date/time failure).
// Return value: 0 if RTC not present, 1 if found.
{
  byte    OscStopFlag;

  Wire.beginTransmission(DS3231_RTC);
  if (Wire.endTransmission() != 0) return 0;      // RTC not found
  Serial.print(F("IOS: Found RTC DS3231 Module ("));
  printDateTime(1);
  Serial.println(F(")"));

  // Print the temperaturefrom the RTC sensor
  Serial.print(F("IOS: RTC DS3231 temperature sensor: "));
  Serial.print((int8_t)tempC);
  Serial.println(F("C"));
  
  // Read the "Oscillator Stop Flag"
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_STATRG);                      // Set the DS3231 Status Register
  Wire.endTransmission();
  Wire.requestFrom(DS3231_RTC, 1);
  OscStopFlag = Wire.read() & 0x80;               // Read the "Oscillator Stop Flag"

  if (OscStopFlag)
  // RTC oscillator stopped. RTC must be set at compile date/time
  {
    String  compTimeStr(F(" __TIME__"));    // Compile timestamp string
    String  compDateStr(F("__DATE__"));    // Compile datestamp string
    
    // Convert compile time strings to numeric values
    seconds = compTimeStr.substring(6,8).toInt();
    minutes = compTimeStr.substring(3,5).toInt();
    hours = compTimeStr.substring(0,2).toInt();
    day = compDateStr.substring(4,6).toInt();
    switch (compDateStr[0]) 
      {
        case 'J': month = compDateStr[1] == 'a' ? 1 : month = compDateStr[2] == 'n' ? 6 : 7; break;
        case 'F': month = 2; break;
        case 'A': month = compDateStr[2] == 'r' ? 4 : 8; break;
        case 'M': month = compDateStr[2] == 'r' ? 3 : 5; break;
        case 'S': month = 9; break;
        case 'O': month = 10; break;
        case 'N': month = 11; break;
        case 'D': month = 12; break;
      };
    year = compDateStr.substring(9,11).toInt();

    // Ask for RTC setting al compile date/time
    Serial.println(F("IOS: RTC clock failure!"));
    Serial.print(F("\nDo you want set RTC at IOS compile time ("));
    printDateTime(0);
    Serial.print(F(")? [Y/N] >"));
    timeStamp = millis();
    do
    {
      blinkIOSled(&timeStamp);
      inChar = Serial.read();
    }
    while ((inChar != 'y') && (inChar != 'Y') && (inChar != 'n') &&(inChar != 'N'));
    Serial.println(inChar);
 
    // Set the RTC at the compile date/time and print a message
    if ((inChar == 'y') || (inChar == 'Y'))
    {
      writeRTC(seconds, minutes, hours, day, month, year);
      Serial.print(F("IOS: RTC set at compile time - Now: "));
      printDateTime(1);
      Serial.println();
    }

    // Reset the "Oscillator Stop Flag" 
    Wire.beginTransmission(DS3231_RTC);
    Wire.write(DS3231_STATRG);                    // Set the DS3231 Status Register
    Wire.write(0x08);                             // Reset the "Oscillator Stop Flag" (32KHz output left enabled)
    Wire.endTransmission();
  }
  return 1;
}

// ------------------------------------------------------------------------------

void printDateTime(byte readSourceFlag)
// Print to serial the current date/time from the global variables.
//
// Flag readSourceFlag [0..1] usage:
//    If readSourceFlag = 0 the RTC read is not done
//    If readSourceFlag = 1 the RTC read is done (global variables are updated)
{
  if (readSourceFlag)
  {
    readRTC(&seconds, &minutes, &hours, &day,  &month,  &year, &tempC);
  }
  Serial.printf(F("%2d/%02d/%02d %02d:%02d:%02d"), day, month, year, hours, minutes, seconds);
}

// ------------------------------------------------------------------------------

byte isLeapYear(byte yearXX)
// Check if the year 2000+XX (where XX is the argument yearXX [00..99]) is a leap year.
// Returns 1 if it is leap, 0 otherwise.
// This function works in the [2000..2099] years range. It should be enough...
{
  if (((2000 + yearXX) % 4) == 0) return 1;
  else return 0;
}

// ------------------------------------------------------------------------------

void ChangeRTC()
// Change manually the RTC Date/Time from keyboard
{
  // Read RTC
  readRTC(&seconds, &minutes, &hours, &day,  &month,  &year, &tempC);

  // Change RTC date/time from keyboard
  byte partIndex = 0;
  char inChar = 0;
  Serial.println(F("\nIOS: RTC manual setting:"));
  Serial.println(F("\nPress T/U to increment +10/+1 or CR to accept"));
  do
  {
    do
    {
      switch (partIndex)
      {
        case 0:
          Serial.printf(F(" Year -> %02d"), year);
        break;
        
        case 1:
          Serial.printf(F(" Month -> %02d"), month);
        break;

        case 2:
          Serial.printf(F(" Day -> %02d  "), day);
        break;

        case 3:
          Serial.printf(F(" Hours -> %02d"), hours);
        break;

        case 4:
          Serial.printf(F(" Minutes -> %02d"), minutes);
        break;

        case 5:
          Serial.printf(F(" Seconds -> %02d"), seconds);
        break;
      }

      timeStamp = millis();
      do
      {
        blinkIOSled(&timeStamp);
        inChar = toupper(Serial.read());
      }
      while ((inChar != 'U') && (inChar != 'T') && (inChar != 13));
      
      if ((inChar == 'U'))
      {
        // Change units
        switch (partIndex)
        {
          case 0:
            year++;
            if (year > 99) year = 0;
          break;

          case 1:
            month++;
            if (month > 12) month = 1;
          break;

          case 2:
            day++;
            if (month == 2)
            {
              if (day > (daysOfMonth[month - 1] + isLeapYear(year))) day = 1;
            }
            else
            {
              if (day > (daysOfMonth[month - 1])) day = 1;
            }
          break;

          case 3:
            hours++;
            if (hours > 23) hours = 0;
          break;

          case 4:
            minutes++;
            if (minutes > 59) minutes = 0;
          break;

          case 5:
            seconds++;
            if (seconds > 59) seconds = 0;
          break;
        }
      }
      if ((inChar == 'T'))
      {
        // Change tens
        switch (partIndex)
        {
          case 0:
            year = year + 10;
            if (year > 99) year = year - (year / 10) * 10; 
          break;

          case 1:
            if (month > 10) month = month - 10;
            else if (month < 3) month = month + 10;
          break;

          case 2:
            day = day + 10;
            if (day > (daysOfMonth[month - 1] + isLeapYear(year))) day = day - (day / 10) * 10;
            if (day == 0) day = 1;
          break;

          case 3:
            hours = hours + 10;
            if (hours > 23) hours = hours - (hours / 10 ) * 10;
          break;

          case 4:
            minutes = minutes + 10;
            if (minutes > 59) minutes = minutes - (minutes / 10 ) * 10;
          break;

          case 5:
            seconds = seconds + 10;
            if (seconds > 59) seconds = seconds - (seconds / 10 ) * 10;
          break;
        }
      }
      Serial.write(13);
    }
    while (inChar != 13);
    partIndex++;
  }
  while (partIndex < 6);  

  // Write new date/time into the RTC
  writeRTC(seconds, minutes, hours, day, month, year);
  Serial.println(F(" ...done      "));
  Serial.print(F("IOS: RTC date/time updated ("));
  printDateTime(1);
  Serial.println(F(")"));
}


// ------------------------------------------------------------------------------

// Z80 bootstrap routines

// ------------------------------------------------------------------------------


void pulseClock(byte numPulse)
// Generate <numPulse> clock pulses on the Z80 clock pin.
// The steady clock level is LOW, e.g. one clock pulse is a 0-1-0 transition
{
  byte    i;
  for (i = 0; i < numPulse; i++)
  // Generate one clock pulse
  {
    // Send one impulse (0-1-0) on the CLK output
    digitalWrite(CLK, HIGH);
    digitalWrite(CLK, LOW);
  }
}

// ------------------------------------------------------------------------------

void loadByteToRAM(byte value)
// Load a given byte to RAM using a sequence of two Z80 instructions forced on the data bus.
// The RAM_CE2 signal is used to force the RAM in HiZ, so the Atmega can write the needed instruction/data
//  on the data bus. Controlling the clock signal and knowing exactly how many clocks pulse are required it 
//  is possible control the whole loading process.
// In the following "T" are the T-cycles of the Z80 (See the Z80 datashet).
// The two instruction are "LD (HL), n" and "INC (HL)".
{
  
  // Execute the LD(HL),n instruction (T = 4+3+3). See the Z80 datasheet and manual.
  // After the execution of this instruction the <value> byte is loaded in the memory address pointed by HL.
  pulseClock(1);                      // Execute the T1 cycle of M1 (Opcode Fetch machine cycle)
  digitalWrite(RAM_CE2, LOW);         // Force the RAM in HiZ (CE2 = LOW)
  DDRA = 0xFF;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = LD_HL;                      // Write "LD (HL), n" opcode on data bus
  pulseClock(2);                      // Execute T2 and T3 cycles of M1
  DDRA = 0x00;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as input... 
  PORTA = 0xFF;                       // ...with pull-up
  pulseClock(2);                      // Complete the execution of M1 and execute the T1 cycle of the following 
                                      // Memory Read machine cycle
  DDRA = 0xFF;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = value;                      // Write the byte to load in RAM on data bus
  pulseClock(2);                      // Execute the T2 and T3 cycles of the Memory Read machine cycle
  DDRA = 0x00;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as input... 
  PORTA = 0xFF;                       // ...with pull-up
  digitalWrite(RAM_CE2, HIGH);        // Enable the RAM again (CE2 = HIGH)
  pulseClock(3);                      // Execute all the following Memory Write machine cycle

  // Execute the INC(HL) instruction (T = 6). See the Z80 datasheet and manual.
  // After the execution of this instruction HL points to the next memory address.
  pulseClock(1);                      // Execute the T1 cycle of M1 (Opcode Fetch machine cycle)
  digitalWrite(RAM_CE2, LOW);         // Force the RAM in HiZ (CE2 = LOW)
  DDRA = 0xFF;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = INC_HL;                     // Write "INC(HL)" opcode on data bus
  pulseClock(2);                      // Execute T2 and T3 cycles of M1
  DDRA = 0x00;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as input... 
  PORTA = 0xFF;                       // ...with pull-up
  digitalWrite(RAM_CE2, HIGH);        // Enable the RAM again (CE2 = HIGH)
  pulseClock(3);                      // Execute all the remaining T cycles
}

// ------------------------------------------------------------------------------

void loadHL(word value)
// Load "value" word into the HL registers inside the Z80 CPU, using the "LD HL,nn" instruction.
// In the following "T" are the T-cycles of the Z80 (See the Z80 datashet).
{
  // Execute the LD dd,nn instruction (T = 4+3+3), with dd = HL and nn = value. See the Z80 datasheet and manual.
  // After the execution of this instruction the word "value" (16bit) is loaded into HL.
  pulseClock(1);                      // Execute the T1 cycle of M1 (Opcode Fetch machine cycle)
  digitalWrite(RAM_CE2, LOW);         // Force the RAM in HiZ (CE2 = LOW)
  DDRA = 0xFF;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = LD_HLnn;                    // Write "LD HL, n" opcode on data bus
  pulseClock(2);                      // Execute T2 and T3 cycles of M1
  DDRA = 0x00;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as input... 
  PORTA = 0xFF;                       // ...with pull-up
  pulseClock(2);                      // Complete the execution of M1 and execute the T1 cycle of the following 
                                      // Memory Read machine cycle
  DDRA = 0xFF;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = lowByte(value);             // Write first byte of "value" to load in HL
  pulseClock(3);                      // Execute the T2 and T3 cycles of the first Memory Read machine cycle
                                      // and T1, of the second Memory Read machine cycle
  PORTA = highByte(value);            // Write second byte of "value" to load in HL
  pulseClock(2);                      // Execute the T2 and T3 cycles of the second Memory Read machine cycle                                    
  DDRA = 0x00;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as input... 
  PORTA = 0xFF;                       // ...with pull-up
  digitalWrite(RAM_CE2, HIGH);        // Enable the RAM again (CE2 = HIGH)
}

// ------------------------------------------------------------------------------

void singlePulsesResetZ80()
// Reset the Z80 CPU using single pulses clock
{
  digitalWrite(RESET_, LOW);          // Set RESET_ active
  pulseClock(6);                      // Generate twice the needed clock pulses to reset the Z80
  digitalWrite(RESET_, HIGH);         // Set RESET_ not active
  pulseClock(2);                      // Needed two more clock pulses after RESET_ goes HIGH
}


// ------------------------------------------------------------------------------

// SD Disk routines (FAT16 and FAT32 filesystems supported) using the PetitFS library.
// For more info about PetitFS see here: http://elm-chan.org/fsw/ff/00index_p.html

// ------------------------------------------------------------------------------

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
    byte  errcode;
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

void FlushRxBuffer()
{
  while (Serial.available() > 0)
  {
    Serial.read();
  }
}

void waitKey()
// Wait a key to continue
{
  FlushRxBuffer();
  
  Serial.println(F("IOS: Check SD and press a key to repeat\r\n"));
  while(Serial.available() < 1);
}

// ------------------------------------------------------------------------------

void printOsName(byte currentDiskSet)
// Print the current Disk Set number and the OS name, if it is defined.
// The OS name is inside the file defined in DS_OSNAME
{
  OsBootInfo bootInfo = GetDiskSetBootInfo(currentDiskSet);
  Serial.printf(F("Disk Set %1X (%s)"), currentDiskSet, bootInfo.OsName);
}

byte ChangeDiskSet(byte curDiskSet)
{
  byte newSet = maxDiskSet + 1;
  Serial.println();
  Serial.print(F("Current selection: "));
  printOsName(curDiskSet);
  Serial.println();
  Serial.println();
  do
  {
    for (int setNum = 0; setNum != maxDiskSet; ++setNum)
    {
      OsBootInfo tmpInfo = GetDiskSetBootInfo(setNum);
      Serial.printf(F(" %d: Disk Set %d (%s)\n\r"), setNum, setNum, tmpInfo.OsName);
    }
    Serial.print(F("Enter your choice or ESC to return>"));
    FlushRxBuffer();
    while(Serial.available() < 1)
    {
      blinkIOSled(&timeStamp);  // Wait a key
    }
    char inChar = Serial.read();
    if (isDigit(inChar))
    {
      newSet = inChar - '0';
    }
    else if (inChar == 27)
    {
      newSet = curDiskSet;
    }
  } while (newSet >= maxDiskSet);
  Serial.println(F("   Ok"));
  return newSet;
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

void PrintSystemOptions(const ConfigOptions &options, const __FlashStringHelper *msg)
{
  Serial.print(msg);
  Serial.println(F("configOptions are:"));
  Serial.printf(F("Valid = %d\n\r"), options.Valid);
  Serial.printf(F("BootMode = %d\n\r"), options.BootMode);
  Serial.printf(F("ClockMode = %d\n\r"), options.ClockMode);
  Serial.printf(F("DiskSet = %d\n\r"), options.DiskSet);
  Serial.printf(F("AutoexecFlag = %d\n\r"), options.AutoexecFlag);
}
