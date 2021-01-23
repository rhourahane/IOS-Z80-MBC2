#if !defined(_OP_CODES_H_)
#define _OP_CODES_H_
enum OpCodes : byte
{
  // Write OpCodes start from 0x00
  USER_LED     = 0x00,
  SERIAL_TX    = 0x01,
  GPIOA_WRITE  = 0x03,
  GPIOB_WRITE  = 0x04,
  IODIRA_WRITE = 0x05,
  IODIRB_WRITE = 0x06,
  GPPUA_WRITE  = 0x07,
  GPPUB_WRITE  = 0x08,
  SELDISK      = 0x09,
  SELTRACK     = 0x0A,
  SELSECT      = 0x0B,
  WRITESECT    = 0x0C,
  SETBANK      = 0x0D,
  SETPATH      = 0x0E,
  SETSEGMENT   = 0x0F,

  // Read OpCodes start from 0x80
  USER_KEY     = 0x80,
  GPIOA_READ   = 0x81,
  GPIOB_READ   = 0x82,
  SYSFLAGS     = 0x83,
  DATETIME     = 0x84,
  ERRDISK      = 0x85,
  READSECT     = 0x86,
  SDMOUNT      = 0x87,
  READDIR      = 0x88,
  READFILE     = 0x89,
      
  NO_OP        = 0xFF
};
#endif
