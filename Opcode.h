#if !defined(_OP_CODE_H_)
#define _OP_CODE_H_
enum Opcode : byte
{
  // Write OpCodes start from 0x00
  // Set the state of the User LED
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              x  x  x  x  x  x  x  0    USER Led off
  //              x  x  x  x  x  x  x  1    USER Led on
  USER_LED     = 0x00,

  // Transmit a byte over the serial connection
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    ASCII char to be sent to serial
  SERIAL_TX    = 0x01,

  // GPIOA Write (GPE Option):
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    GPIOA value (see MCP23017 datasheet)
  GPIOA_WRITE  = 0x03,

  // GPIOB Write (GPE Option): 
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    GPIOB value (see MCP23017 datasheet)
  GPIOB_WRITE  = 0x04,

  // IODIRA Write (GPE Option):
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    IODIRA value (see MCP23017 datasheet)
  IODIRA_WRITE = 0x05,

  // IODIRB Write (GPE Option):
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    IODIRB value (see MCP23017 datasheet)
  IODIRB_WRITE = 0x06,

  // GPPUA Write (GPE Option):
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    GPPUA value (see MCP23017 datasheet)
  GPPUA_WRITE  = 0x07,

  // GPPUB Write (GPE Option):
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    GPPUB value (see MCP23017 datasheet)
  GPPUB_WRITE  = 0x08,

  // Select the emulated disk number (binary). 100 disks are supported [0..99]:
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    DISK number (binary) [0..99]
  //
  // Opens the "disk file" correspondig to the selected disk number, doing some checks.
  // A "disk file" is a binary file that emulates a disk using a LBA-like logical sector number.
  // Every "disk file" must have a dimension of 8388608 bytes, corresponding to 16384 LBA-like logical sectors
  // (each sector is 512 bytes long), correspinding to 512 tracks of 32 sectors each (see SELTRACK and 
  // SELSECT opcodes).
  // Errors are stored into "errDisk" (see ERRDISK opcode).
  //
  // "Disk file" filename convention:
  //
  // Every "disk file" must follow the sintax "DSsNnn.DSK" where
  //
  //    "s" is the "disk set" and must be in the [0..9] range (always one numeric ASCII character)
  //    "nn" is the "disk number" and must be in the [00..99] range (always two numeric ASCII characters)
  //
  // NOTE 1: The maximum disks number may be lower due the limitations of the used OS (e.g. CP/M 2.2 supports
  //         a maximum of 16 disks)
  // NOTE 2: Because SELDISK opens the "disk file" used for disk emulation, before using WRITESECT or READSECT
  //         a SELDISK must be performed at first.
  SELDISK      = 0x09,

  // Select the emulated track number (word splitted in 2 bytes in sequence: DATA 0 and DATA 1):
  // I/O DATA 0:  D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    Track number (binary) LSB [0..255]
  //
  // I/O DATA 1:  D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    Track number (binary) MSB [0..1]
  //
  // Stores the selected track number into "trackSel" for "disk file" access.
  // A "disk file" is a binary file that emulates a disk using a LBA-like logical sector number.
  // The SELTRACK and SELSECT operations convert the legacy track/sector address into a LBA-like logical 
  //  sector number used to set the logical sector address inside the "disk file".
  // A control is performed on both current sector and track number for valid values. 
  // Errors are stored into "diskErr" (see ERRDISK opcode).
  //
  // NOTE 1: Allowed track numbers are in the range [0..511] (512 tracks)
  // NOTE 2: Before a WRITESECT or READSECT operation at least a SELSECT or a SELTRAK operation
  //         must be performed
  SELTRACK = 0x0A,
  
  // Select the emulated sector number (binary):
  // I/O DATA:  D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //            D7 D6 D5 D4 D3 D2 D1 D0    Sector number (binary) [0..31]
  //
  // Stores the selected sector number into "sectSel" for "disk file" access.
  // A "disk file" is a binary file that emulates a disk using a LBA-like logical sector number.
  // The SELTRACK and SELSECT operations convert the legacy track/sector address into a LBA-like logical 
  //  sector number used to set the logical sector address inside the "disk file".
  // A control is performed on both current sector and track number for valid values. 
  // Errors are stored into "diskError" (see ERRDISK opcode).
  //
  // NOTE 1: Allowed sector numbers are in the range [0..31] (32 sectors)
  // NOTE 2: Before a WRITESECT or READSECT operation at least a SELSECT or a SELTRAK operation
  //         must be performed
  SELSECT = 0x0B,

  // WRITESECT - write 512 data bytes sequentially into the current emulated disk/track/sector:
  // I/O DATA 0:   D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //               D7 D6 D5 D4 D3 D2 D1 D0    First Data byte
  //      |               |
  //      |               |
  //      |               |                 <510 Data Bytes>
  //      |               |
  // I/O DATA 511: D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //               D7 D6 D5 D4 D3 D2 D1 D0    512th Data byte (Last byte)
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
  WRITESECT = 0x0C,

  // Select the Os RAM Bank (binary):
  // I/O DATA:  D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //            D7 D6 D5 D4 D3 D2 D1 D0    Os Bank number (binary) [0..2]
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
  //  Os Bank | Logical Bank |  Z80 Address Bus    |   Physical Bank   |            Notes
  //  number  | BANK1 BANK0  |        A15          |  RAM_A16 RAM_A15  |
  // ------------------------------------------------------------------------------------------------
  //     X    |   X     X    |         1           |     0       1     |  Phy Bank 1 (common fixed)
  //     -    |   0     0    |         0           |     0       1     |  Phy Bank 1 (common fixed)
  //     0    |   0     1    |         0           |     0       0     |  Phy Bank 0 (Logical Bank 1)
  //     2    |   1     0    |         0           |     1       1     |  Phy Bank 3 (Logical Bank 2)
  //     1    |   1     1    |         0           |     1       0     |  Phy Bank 2 (Logical Bank 3)
  //
  //      Physical Bank      |    Logical Bank     |   Physical Bank   |   Physical RAM Addresses
  //          number         |       number        |  RAM_A16 RAM_A15  |
  // ------------------------------------------------------------------------------------------------
  //            0            |         1           |     0       0     |   From 0x00000 to 0x07FFF 
  //            1            |         0           |     0       1     |   From 0x08000 to 0x0FFFF
  //            2            |         3           |     1       0     |   From 0x01000 to 0x17FFF
  //            3            |         2           |     1       1     |   From 0x18000 to 0x1FFFF
  //
  // Note that the Logical Bank 0 can't be used as switchable Os Bank bacause it is the common 
  // fixed bank mapped in the upper half of the Z80 address space (from 0x8000 to 0xFFFF).
  //
  // NOTE: If the Os Bank number is greater than 2 no selection is done.
  SETBANK = 0x0D,

  // Set the path used for FAT file system operations
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0   D7 D6 D5 D4 D3 D2 D1 D0    1st char of path
  //     |                  |
  // I/O DATA N   D7 D6 D5 D4 D3 D2 D1 D0    '\0'
  // NOTE 1: The path string is terminated by a '\0' char and directories separated by '/'.
  // NOYE 2: After writing to a FAT file the path should be set to '\0' to close the file and
  //         ensure that all data is written.
  SETPATH = 0x0E,
  
  // Set segment number to be read/written from the FAT file
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0   D7 D6 D5 D4 D3 D2 D1 D0    LSB
  // I/O DATA 1   D7 D6 D5 D4 D3 D2 D1 D0    MSB
  // Note: Each seqment is 128 bytes long except the last one which can be any size.
  SETSEGMENT   = 0x0F,

  // Write a segment to a FAT file previously set via SETPATH
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0   D7 D6 D5 D4 D3 D2 D1 D0    Bytes to be written max 128
  // I/O DATA 1   D7 D6 D5 D4 D3 D2 D1 D0    Byte 1 of segement
  //     |                  |
  // I/O DATA N   D7 D6 D5 D4 D3 D2 D1 D0    Last by of segment
  // Note: Less than 128 bytes can be written if the file size is not a multiple of 128.
  WRITEFILE = 0x10,

  // Set the address and number of bytes for I2C reads and writes
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0   D7 D6 D5 D4 D3 D2 D1 D0    7 bit address
  // I/O DATA 1   D7 D6 D5 D4 D3 D2 D1 D0    number of bytea to transfer
  //
  // NOTE: The address and transfer count need to be set before reading or writing I2C
  I2CADDR = 0x11,

  // Write to the I2C bus the previously set number of bytes
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0   D7 D6 D5 D4 D3 D2 D1 D0
  //     |                  |
  // I/O DATA N   D7 D6 D5 D4 D3 D2 D1 D0
  I2CWRITE = 0x12,

  // Read OpCodes start from 0x80
  // Read user key      
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              0  0  0  0  0  0  0  0    USER Key not pressed
  //              0  0  0  0  0  0  0  1    USER Key pressed
  USER_KEY = 0x80,

  // GPIOA Read (GPE Option):
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    GPIOA value (see MCP23017 datasheet)
  //
  // NOTE: a value 0x00 is forced if the GPE Option is not present
  GPIOA_READ = 0x81,
  
  // GPIOB Read (GPE Option):
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    GPIOB value (see MCP23017 datasheet)
  //
  // NOTE: a value 0x00 is forced if the GPE Option is not present
  GPIOB_READ = 0x82,
  
  // SYSFLAGS (Various system flags for the OS):
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              X  X  X  X  X  X  X  0    AUTOEXEC not enabled
  //              X  X  X  X  X  X  X  1    AUTOEXEC enabled
  //              X  X  X  X  X  X  0  X    DS3231 RTC not found
  //              X  X  X  X  X  X  1  X    DS3231 RTC found
  //              X  X  X  X  X  0  X  X    Serial RX buffer empty
  //              X  X  X  X  X  1  X  X    Serial RX char available
  //              X  X  X  X  0  X  X  X    Previous RX char valid
  //              X  X  X  X  1  X  X  X    Previous RX char was a "buffer empty" flag
  //
  // NOTE: Currently only D0-D3 are used
  SYSFLAGS = 0x83,
  
  // DATETIME (Read date/time and temperature from the RTC. Binary values): 
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0   D7 D6 D5 D4 D3 D2 D1 D0    seconds [0..59]
  // I/O DATA 1   D7 D6 D5 D4 D3 D2 D1 D0    minutes [0..59]
  // I/O DATA 2   D7 D6 D5 D4 D3 D2 D1 D0    hours   [0..23]
  // I/O DATA 3   D7 D6 D5 D4 D3 D2 D1 D0    day     [1..31]
  // I/O DATA 4   D7 D6 D5 D4 D3 D2 D1 D0    month   [1..12]
  // I/O DATA 5   D7 D6 D5 D4 D3 D2 D1 D0    year    [0..99]
  // I/O DATA 6   D7 D6 D5 D4 D3 D2 D1 D0    tempC   [-128..127] (7th data byte)
  //
  // NOTE 1: If RTC is not found all read values wil be = 0
  // NOTE 2: Overread data (more then 7 bytes read) will be = 0
  // NOTE 3: The temperature (Celsius) is a byte with two complement binary format [-128..127]
  DATETIME = 0x84,
  
  // Rread the error code after a SELDISK, SELSECT, SELTRACK, WRITESECT, READSECT 
  // or SDMOUNT operation
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0    DISK error code (binary)
  //
  // Error codes table:
  //
  // error code | description
  // ---------------------------------------------------------------------------------------------------
  //     0      |  No error
  //     1      |  DISK_ERR: the function failed due to a hard error in the disk function, 
  //            |   a wrong FAT structure or an internal error
  //     2      |  NOT_READY: the storage device could not be initialized due to a hard error or 
  //            |   no medium
  //     3      |  NO_FILE: could not find the file
  //     4      |  NOT_OPENED: the file has not been opened
  //     5      |  NOT_ENABLED: the volume has not been mounted
  //     6      |  NO_FILESYSTEM: there is no valid FAT partition on the drive
  //    16      |  Illegal disk number
  //    17      |  Illegal track number
  //    18      |  Illegal sector number
  //    19      |  Reached an unexpected EOF
  //
  // NOTE 1: ERRDISK code is referred to the previous SELDISK, SELSECT, SELTRACK, WRITESECT or READSECT
  //         operation
  // NOTE 2: Error codes from 0 to 6 come from the PetitFS library implementation
  // NOTE 3: ERRDISK must not be used to read the resulting error code after a SDMOUNT operation 
  //         (see the SDMOUNT opcode)
  ERRDISK = 0x85,

  // Read 512 data bytes sequentially from the current emulated disk/track/sector:
  // I/O DATA:     D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0    D7 D6 D5 D4 D3 D2 D1 D0    First Data byte
  //
  //       |                |
  //       |                |
  //       |                |                 <510 Data Bytes>
  //       |                |
  //
  // I/O DATA 511  D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //               D7 D6 D5 D4 D3 D2 D1 D0    512th Data byte (Last byte)
  //
  // Reads the current sector (512 bytes) of the current track/sector, one data byte each call. 
  // All the 512 calls must be always performed sequentially to have a READSECT operation correctly done. 
  // If an error occurs during the READSECT operation, all subsequent read data will be = 0.
  // If an error occurs calling any DISK EMULATION opcode (SDMOUNT excluded) immediately before the READSECT 
  // opcode call, all the read data will be will be = 0 and the READSECT operation will not be performed.
  // Errors are stored into "diskError" (see ERRDISK opcode).
  //
  // NOTE 1: Before a READSECT operation at least a SELTRACK or a SELSECT must be always performed
  // NOTE 2: Remember to open the right "disk file" at first using the SELDISK opcode
  READSECT = 0x86,

  // Mount a volume on SD, returning an error code (binary):
  // I/O DATA 0: D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //             D7 D6 D5 D4 D3 D2 D1 D0    error code (binary)
  //
  // NOTE 1: This opcode is "normally" not used. Only needed if using a virtual disk from a custom program
  //         loaded with iLoad or with the Autoboot mode (e.g. ViDiT). Can be used to handle SD hot-swapping
  // NOTE 2: For error codes explanation see ERRDISK opcode
  // NOTE 3: Only for this disk opcode, the resulting error is read as a data byte without using the 
  //         ERRDISK opcode
  SDMOUNT = 0x87,

  // Read next directory entry from directory set via SETPATH
  //
  // I/O DATA:     D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0    D7 D6 D5 D4 D3 D2 D1 D0    1st byte of file name
  //     |                   |
  // I/O DATA 12   D7 D6 D5 D4 D3 D2 D1 D0    12th and last byte of file name
  // I/O DATA 13   D7 D6 D5 D4 D3 D2 D1 D0    MSB File size in bytes
  //     |                   |
  // I/O DATA 16   D7 D6 D5 D4 D3 D2 D1 D0    LSB File size in bytes
  // I/O DATA 17   D7 D6 D5 D4 D3 D2 D1 D0    File attributes
  //
  // NOTE 1: The file name is terminated with a '\0' character
  // NOTE 2: The file attributes D0 indicates a directory
  READDIR = 0x88,

  // Read up to 128 data bytes sequentially from a FAT file set via SETPATH
  // I/O DATA:     D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0    D7 D6 D5 D4 D3 D2 D1 D0    Number of bytes read
  // I/O DATA 1    D7 D6 D5 D4 D3 D2 D1 D0    1st Data byte
  //       |                |
  // I/O DATA 128  D7 D6 D5 D4 D3 D2 D1 D0    128th Data byte
  //
  // NOTE: The final segment of the file will 
  READFILE = 0x89,

  // Find if the file exists FAT file set via SETPATH
  // I/O DATA:     D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0    D7 D6 D5 D4 D3 D2 D1 D0    Status
  //
  // NOTE: 1 file exists otherwise 0
  FILEEXISTS = 0x8A,
  
  // Create a directory in the FAT file system set via SETPATH
  // I/O DATA:     D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0    D7 D6 D5 D4 D3 D2 D1 D0    Status
  MKDIR = 0x8B,

  // Delete the FAT file set via SETPATH
  // I/O DATA:     D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0    D7 D6 D5 D4 D3 D2 D1 D0    Status
  DELFILE = 0x8C,

  // Probe an address on the I2C bus
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  //              D7 D6 D5 D4 D3 D2 D1 D0   Status
  // Note: 0 if not pressent 1 if present.
  I2CPROBE = 0x8D,

  // Read from the I2C bus the previously set number of bytes
  // I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
  // ---------------------------------------------------------
  // I/O DATA 0   D7 D6 D5 D4 D3 D2 D1 D0
  //     |                  |
  // I/O DATA N   D7 D6 D5 D4 D3 D2 D1 D0
  I2CREAD = 0X8E,

  // No operation, used when no IO opteration is in flight.
  NO_OP = 0xFF
};
#endif
