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

#include "HwDefines.h"
#include "OsBootInfo.h"
#include "RtcUtilities.h"
#include "ConfigOptions.h"
#include "BootMenu.h"
#include "Utils.h"
#include "WireUtils.h"
#include "DiskUtils.h"
#include "Opcodes.h"
#include "FatSystem.h"

#define HW_VERSION "A040618"
#define SW_VERSION "RMH-OS-SD"


// ------------------------------------------------------------------------------
//
//  Constants
//
// ------------------------------------------------------------------------------
const byte    debug        = 0;           // Debug off = 0, on = 1, on = 2 with interrupt trace

// ------------------------------------------------------------------------------
//
//  Global variables
//
// ------------------------------------------------------------------------------

// General purpose variables
byte          ioAddress;                  // Virtual I/O address. Only two possible addresses are valid (0x00 and 0x01)
byte          ioData;                     // Data byte used for the I/O operation
byte          ioOpcode       = NO_OP;     // I/O operation code or Opcode (0xFF means "No Operation")
word          ioByteCnt;                  // Exchanged bytes counter during an I/O operation
byte          moduleGPIO     = 0;         // Set to 1 if the module is found, 0 otherwise
byte *        BootImage;                  // Pointer to selected flash payload array (image) to boot
word          BootImageSize  = 0;         // Size of the selected flash payload array (image) to boot
word          BootStrAddr;                // Starting address of the selected program to boot (from flash or SD)
byte          Z80IntEnFlag   = 0;         // Z80 INT_ enable flag (0 = No INT_ used, 1 = INT_ used for I/O)
byte          LastRxIsEmpty;              // "Last Rx char was empty" flag. Is set when a serial Rx operation was done
                                          // when the Rx buffer was empty

ConfigOptions SystemOptions;
OsBootInfo    BootInfo;

byte          numReadBytes;               // Number of read bytes after a readSD() call

// Disk emulation on SD
char          diskName[11]    = Z80DISK;  // String used for virtual disk file name
word          trackSel;                   // Store the current track number [0..511]
byte          sectSel;                    // Store the current sector number [0..31]
byte          diskErr         = 19;       // SELDISK, SELSECT, SELTRACK, WRITESECT, READSECT or SDMOUNT resulting 
                                          // error code
byte          numWriBytes;                // Number of written bytes after a writeSD() call

FatSystem fatSystem;

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
  Wire.begin();
  if (ProbeAddress(GPIOEXP_ADDR))
  {
    moduleGPIO = 1;// Set to 1 if GPIO Module is found
  }
  
  // Print if the input serial buffer is 128 bytes wide (this is needed for xmodem protocol support)
  if (SERIAL_RX_BUFFER_SIZE >= 128)
  {
    Serial.println(F("IOS: Found extended serial Rx buffer"));
  }

  // Print the Z80 clock speed mode
  Serial.printf(F("IOS: Z80 clock set at %dMHz\n\r"), SystemOptions.ClockMode ? CLOCK_LOW : CLOCK_HIGH);

  // Print RTC and GPIO informations if found
  foundRTC = autoSetRTC();                        // Check if RTC is present and initialize it as needed
  if (moduleGPIO)
  {
    Serial.println(F("IOS: Found GPE Option"));
  }
  
  // Print CP/M Autoexec on cold boot status
  Serial.print(F("IOS: CP/M Autoexec is "));
  if (SystemOptions.AutoexecFlag)
  {
    Serial.println(F("ON"));
  }
  else
  {
    Serial.println(F("OFF"));
  }

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
  byte maxDiskSet = FindLastDiskSet();
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
    bootMode = BootMenu(bootMode, maxDiskSet, foundRTC, SystemOptions);
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
  const char *fileNameSD;

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
    byte errCodeSD;
    
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
        for (int iCount = 0; iCount < numReadBytes; iCount++)
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

void loop() 
{
  byte tempByte;
  
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
      // Opcode 0x0E  SETPATH         12
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
            WriteRegisters(GPIOEXP_ADDR, GPIOA_REG, 1, &ioData);
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
            WriteRegisters(GPIOEXP_ADDR, GPIOB_REG, 1, &ioData);
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
            WriteRegisters(GPIOEXP_ADDR, IODIRA_REG, 1, &ioData);
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
            WriteRegisters(GPIOEXP_ADDR, IODIRB_REG, 1, &ioData);
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
            WriteRegisters(GPIOEXP_ADDR, GPPUA_REG, 1, &ioData);
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
            WriteRegisters(GPIOEXP_ADDR, GPPUB_REG, 1, &ioData);
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
        case SETPATH:
          fatSystem.SetPath(ioData);
        break;
        }
        if ((ioOpcode != 0x0A) &&
            (ioOpcode != 0x0C) &&
            (ioOpcode != SETPATH))
        {
          ioOpcode = 0xFF;    // All done for the single byte opcodes. 
        }
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
              if (!diskErr)
              {
                ioData = bufferSD[tempByte];// If no errors, exchange current data byte with the CPU
              }
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

          case READDIR:
            // READ FAT DIR
            // READDIR - Read the next directory entry for the FAT file path
            //
            // 
            ioOpcode = fatSystem.ReadNextDir(ioData);
          break;
          }
          if ((ioOpcode != 0x84) &&
              (ioOpcode != 0x86) &&
              (ioOpcode != READDIR))
          {
            ioOpcode = NO_OP;  // All done for the single byte opcodes. 
          }
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

void waitKey()
// Wait a key to continue
{
  FlushRxBuffer();
  
  Serial.println(F("IOS: Check SD and press a key to repeat\r\n"));
  while(Serial.available() < 1);
}

// ------------------------------------------------------------------------------
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
