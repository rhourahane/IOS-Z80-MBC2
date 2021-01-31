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
#include "FatSubsys.h"
#include "GpioSubsys.h"
#include "UserSubsys.h"
#include "WireSubsys.h"
#include "DriveSubsys.h"
#include "BankSubsys.h"
#include "RtcSubsys.h"

#define HW_VERSION "A040618"
#define SW_VERSION "RMH-OS-MORE"


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

FatSystem fatSystem;
GpioSubsys gpioSubsys;
UserSubsys userSubsys;
WireSubsys wireSubsys;
DriveSubsys driveSubsys;
BankSubsys bankSubsys;
RtcSubsys rtcSubsys;

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
  rtcSubsys.foundRtc(foundRTC);
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
  driveSubsys.DiskSet(SystemOptions.DiskSet);
  
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
      byte numReadBytes;
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
      // Opcode 0x0E  SETPATH         Variable
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
        case  USER_LED:
          ioOpcode = userSubsys.Write((Opcodes)ioOpcode, ioData);
         break;

        case  SERIAL_TX:
          // SERIAL TX:     
          //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
          //                            ---------------------------------------------------------
          //                             D7 D6 D5 D4 D3 D2 D1 D0    ASCII char to be sent to serial
          
          Serial.write(ioData);
          ioOpcode = NO_OP;
        break;
        
        case GPIOA_WRITE:
        case GPIOB_WRITE:
        case IODIRA_WRITE:
        case IODIRB_WRITE:
        case GPPUA_WRITE:
        case GPPUB_WRITE:
          if (moduleGPIO)
          {
            ioOpcode = gpioSubsys.Write((Opcodes)ioOpcode, ioData);
          }
        break;

        case  SELDISK:
        case  SELTRACK:
        case  SELSECT:
        case  WRITESECT:
          ioOpcode = driveSubsys.Write((Opcodes)ioOpcode, ioData);
        break;

        case  SETBANK:
          ioOpcode = bankSubsys.Write((Opcodes)ioOpcode, ioData);
        break;
        
        case SETPATH:
          ioOpcode = fatSystem.SetPath(ioData);
        break;

        case SETSEGMENT:
          // SET SEGMENT
          // SETSEGMENT - Set the segment to be read.written from a FAT file each segment is 128 except for the last that
          //              might be less.
          ioOpcode = fatSystem.SetSegment(ioData);
        break;

        case WRITEFILE:
          ioOpcode = fatSystem.WriteFile(ioData);
        break;

        case I2CADDR:
        case I2CWRITE:
          ioOpcode = wireSubsys.Write((Opcodes)ioOpcode, ioData);
        break;
        }
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
            case USER_KEY:
            ioOpcode = userSubsys.Read((Opcodes)ioOpcode, ioData);
          break;

          case GPIOA_READ:
          case GPIOB_READ:
            if (moduleGPIO)
            {
              ioOpcode = gpioSubsys.Read((Opcodes)ioOpcode, ioData);
            }
          break;

          case  SYSFLAGS:
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
            ioOpcode = NO_OP;
          break;

          case  DATETIME:
            ioOpcode = rtcSubsys.Read((Opcodes)ioOpcode, ioData);
          break;

          case  ERRDISK:
          case  READSECT:
          case  SDMOUNT:
            ioOpcode = driveSubsys.Read((Opcodes)ioOpcode, ioData);
          break;

          case READDIR:
          case READFILE:
          case FILEEXISTS:
          case MKDIR:
            ioOpcode = fatSystem.ReadNextDir(ioData);
          break;

          case I2CPROBE:
          case I2CREAD:
            ioOpcode = wireSubsys.Read((Opcodes)ioOpcode, ioData);
          break;
          }
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
