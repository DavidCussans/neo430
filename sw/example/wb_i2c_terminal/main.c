// #################################################################################################
// #  < Wishbone bus explorer >                                                                    #
// # ********************************************************************************************* #
// # Manual access to the registers of modules, which are connected to Wishbone bus. This is also  #
// # a neat example to illustrate the construction of a console-like user interface.               #
// # ********************************************************************************************* #
// # This file is part of the NEO430 Processor project: https://github.com/stnolting/neo430        #
// # Copyright by Stephan Nolting: stnolting@gmail.com                                             #
// #                                                                                               #
// # This source file may be used and distributed without restriction provided that this copyright #
// # statement is not removed from the file and that any derivative work contains the original     #
// # copyright notice and the associated disclaimer.                                               #
// #                                                                                               #
// # This source file is free software; you can redistribute it and/or modify it under the terms   #
// # of the GNU Lesser General Public License as published by the Free Software Foundation,        #
// # either version 3 of the License, or (at your option) any later version.                       #
// #                                                                                               #
// # This source is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;      #
// # without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     #
// # See the GNU Lesser General Public License for more details.                                   #
// #                                                                                               #
// # You should have received a copy of the GNU Lesser General Public License along with this      #
// # source; if not, download it from https://www.gnu.org/licenses/lgpl-3.0.en.html                #
// # ********************************************************************************************* #
// #  Stephan Nolting, Hannover, Germany                                               06.10.2017  #
// #################################################################################################

// I2C routines modified from I2CuHal.py

// Libraries
#include <stdint.h>
#include <string.h>
#include "../../lib/neo430/neo430.h"
#include <stdbool.h>

// Global variables
uint8_t wb_config = 1;

// Prototypes
void setup_i2c(void);
int16_t read_i2c_address(uint8_t addr , uint8_t n , uint8_t data[]);
bool checkack(uint32_t delayVal);
int16_t write_i2c_address(uint8_t addr , uint8_t nToWrite , uint8_t data[], bool stop);
void dump_wb(void);
uint32_t hex_str_to_uint32(char *buffer);
void delay(uint32_t n );
bool enable_i2c_bridge();
int16_t read_E24AA025E48T();
uint16_t zero_buffer( uint8_t buffer[] , uint16_t elements);
int16_t write_Prom();
uint32_t read_Prom();
int16_t  read_i2c_prom( uint8_t startAddress , uint8_t wordsToRead , uint8_t buffer[] );
int16_t write_i2c_prom( uint8_t startAddress , uint8_t wordsToWrite, uint8_t buffer[] );
void uint8_to_decimal_str( uint8_t value , char *buffer) ;

#define DEBUG 1
#define DELAYVAL 512

// Configuration
#define MAX_CMD_LENGTH 16
#define BAUD_RATE 19200

#define MAX_N    16

#define ENABLECORE 0x1 << 7
#define STARTCMD 0x1 << 7
#define STOPCMD  0x1 << 6
#define READCMD  0x1 << 5
#define WRITECMD 0x1 << 4
#define ACK      0x1 << 3
#define INTACK   0x1

#define RECVDACK 0x1 << 7
#define BUSY     0x1 << 6
#define ARBLOST  0x1 << 5
#define INPROGRESS  0x1 << 1
#define INTERRUPT 0x1

// Multiply addresses by 4 to go from byte addresses (Wishbone) to Word addresses (IPBus)
#define ADDR_PRESCALE_LOW 0x0
#define ADDR_PRESCALE_HIGH 0x4
#define ADDR_CTRL 0x8
#define ADDR_DATA 0xC
#define ADDR_CMD_STAT 0x10

// I2C address of Crypto EEPROM on AX3
#define MYSLAVE 0x64

// I2C address of UiD EEPROM on TLU
#define EEPROMADDRESS  0x50

// PROM memory address start...
#define PROMMEMORYADDR 0x00

uint8_t buffer[MAX_N];
char command[MAX_CMD_LENGTH];


/* ------------------------------------------------------------
 * INFO Main function
 * ------------------------------------------------------------ */
int main(void) {

  //uint16_t length = 0;
  uint16_t length = 0;
  uint16_t selection = 0;
  uint16_t uid;


  // setup UART
  uart_set_baud(BAUD_RATE);
  USI_CT = (1<<USI_CT_EN);

  uart_br_print("\n--------------------------------------\n"
                  "--- I2C Wishbone Explorer Terminal --\n"
                  "--------------------------------------\n\n");

  // check if WB unit was synthesized, exit if no WB is available
  if (!(SYS_FEATURES & (1<<SYS_WB32_EN))) {
    uart_br_print("Error! No Wishbone adapter synthesized!");
    return 1;
  }


  // set for 32 bit transfer
  //wb_config = 4;

  setup_i2c();
       
  for (;;) {

    uart_br_print("\nEnter a command:> ");

    //length = uart_scan(command, MAX_CMD_LENGTH);
    length = uart_scan(command, MAX_CMD_LENGTH);
    uart_br_print("\n");

    if (!length) // nothing to be done
     continue;

        // decode input
    selection = 0;
    if (!strcmp(command, "help"))
      selection = 1;
    if (!strcmp(command, "enable"))
      selection = 2;
    if (!strcmp(command, "id"))
      selection = 3;
    if (!strcmp(command, "write"))
      selection = 4;
    if (!strcmp(command, "read"))
      selection = 5;
    if (!strcmp(command, "reset"))
      selection = 6;

        // execute command
    switch(selection) {

      case 1: // print help menu
        uart_br_print("Available commands:\n"
                      " help   - show this text\n"
                      " enable - enable I2C bridge on Enclustra\n"
                      " id     - Read from E24AA025E48T Unique ID\n"
                      " write  - write to E24AA025E48T PROM area \n"
                      " read   - read from E24AA025E48T PROM area \n"
                      " reset  - reset CPU\n"
                      );
        break;

      case 2: // Enable I2C Bridge
	     
	enable_i2c_bridge();
        break;

      case 3: // read from Unique ID address
	uid = read_E24AA025E48T();
	uart_br_print("\nUID from E24AA025E48T = ");
	uart_print_hex_dword(uid);
	uart_br_print("\n");
        break;

      case 4: // write to PROM
	write_Prom();
        break;

      case 5: // read from PROM
	uid = read_Prom();
	uart_br_print("\nData from PROM = \n");
	uart_print_hex_dword(uid);
	uart_br_print("\n");
	uart_br_print("\nIP Address = 192.168.");
	uint8_to_decimal_str( (uint8_t)(uid>>8)&0xFF  , command);
	uart_br_print( command  );
	uart_br_print( "."  );
	uint8_to_decimal_str( (uint8_t)(uid)&0xFF  , command);
	uart_br_print( command  );
	uart_br_print( "\n"  );

        break;

      case 6: // restart
        while ((USI_CT & (1<<USI_CT_UARTTXBSY)) != 0); // wait for current UART transmission
        soft_reset();
        break;

      default: // invalid command
        uart_br_print("Invalid command. Type 'help' to see all commands.\n");
        break;
    }
    


  }
     return 0;
}


bool checkack(uint32_t delayVal) {

#ifdef DEBUG
uart_br_print("\nChecking ACK\n");
#endif

  bool inprogress = true;
  bool ack = false;
  uint8_t cmd_stat = 0;
  while (inprogress) {
    delay(delayVal);
    cmd_stat = wishbone_read8(ADDR_CMD_STAT);
    inprogress = (cmd_stat & INPROGRESS) > 0;
    ack = (cmd_stat & RECVDACK) == 0;

#ifdef DEBUG
    uart_print_hex_byte( (uint8_t)ack );
#endif

  }
  return ack;
}

/* ------------------------------------------------------------
 * Delay by looping over "no-op"
 * ------------------------------------------------------------ */
void delay(uint32_t delayVal){
  for (uint32_t i=0;i<delayVal;i++){
    asm volatile ("MOV r3,r3");
  }
}


/* ------------------------------------------------------------
 * Zero buffer
 * ------------------------------------------------------------ */
uint16_t zero_buffer (uint8_t buffer[] , uint16_t elements) {

  for (uint16_t i=0;i<elements;i++){
    buffer[i] = 0;
  }

  return elements;
}

/* ------------------------------------------------------------
 * INFO Configure Wishbone adapter
 * ------------------------------------------------------------ */
void setup_i2c(void) {

  uint16_t prescale = 0x0400;

  uart_br_print("Setting up I2C core");

// Disable core
  wishbone_write8(ADDR_CTRL, 0);

// Setup prescale
  wishbone_write8(ADDR_PRESCALE_LOW , (prescale & 0x00ff) );
  wishbone_write8(ADDR_PRESCALE_HIGH, (prescale & 0xff00) >> 8);

// Enable core
  wishbone_write8(ADDR_CTRL, ENABLECORE);

  // Delay for at least 100us before proceeding
  delay(1000);

  uart_br_print("\nSetup done.\n");

}


/* ------------------------------------------------------------
 * INFO Read data from Wishbone address
 * ------------------------------------------------------------ */
int16_t read_i2c_address(uint8_t addr , uint8_t n , uint8_t data[]) {

  //static uint8_t data[MAX_N];

  uint8_t val;
  bool ack;

#ifdef DEBUG
  uart_br_print("\nReading From I2C.\n");
#endif

  addr &= 0x7f;
  addr = addr << 1;
  addr |= 0x1 ; // read bit
  wishbone_write8(ADDR_DATA , addr );
  wishbone_write8(ADDR_CMD_STAT, STARTCMD | WRITECMD );
  ack = checkack(DELAYVAL);
  if (! ack) {
      uart_br_print("\nread_i2c_address: No ACK. Sending STOP and terminating read.\n");
      wishbone_write8(ADDR_CMD_STAT, STOPCMD);
      return 0;
      }

  for (uint8_t i=0; i< n ; i++){

      if (i < (n-1)) {
          wishbone_write8(ADDR_CMD_STAT, READCMD);
        } else {
          wishbone_write8(ADDR_CMD_STAT, READCMD | ACK | STOPCMD); // <--- This tells the slave that it is the last word
        }
      ack = checkack(DELAYVAL);

#ifdef DEBUG
      uart_br_print("\nread_i2c_address: ACK = ");
      uart_print_hex_byte( (uint8_t) ack );
      uart_br_print("\n");
#endif
      
      val = wishbone_read8(ADDR_DATA);
      data[i] = val & 0xff;
    }

  return (int16_t) n;

}

int16_t write_i2c_address(uint8_t addr , uint8_t nToWrite , uint8_t data[], bool stop) {


  int16_t nwritten = -1;
  uint8_t val;
  bool ack;
  addr &= 0x7f;
  addr = addr << 1;

#ifdef DEBUG
  uart_br_print("\nWriting to I2C.\n");
#endif

  // Set transmit register (write operation, LSB=0)
  wishbone_write8(ADDR_DATA , addr );
  //  Set Command Register to 0x90 (write, start)
  wishbone_write8(ADDR_CMD_STAT, STARTCMD | WRITECMD );

  ack = checkack(DELAYVAL);

  if (! ack){
    uart_br_print("\nwrite_i2c_address: No ACK in response to device-ID. Sending STOP and terminating.\n");
    wishbone_write8(ADDR_CMD_STAT, STOPCMD);
    return nwritten;
  }

  nwritten += 1;

  for ( uint8_t i=0;i<nToWrite; i++){
      val = (data[i]& 0xff);
      //Write slave data
      wishbone_write8(ADDR_DATA , val );
      //Set Command Register to 0x10 (write)
      wishbone_write8(ADDR_CMD_STAT, WRITECMD);
      ack = checkack(DELAYVAL);
      if (!ack){
          wishbone_write8(ADDR_CMD_STAT, STOPCMD);
          return nwritten;
        }
      nwritten += 1;
    }

  if (stop) {
#ifdef DEBUG
    uart_br_print("\nwrite_i2c_address: Writing STOP\n");
#endif
    wishbone_write8(ADDR_CMD_STAT, STOPCMD);
  } else {
#ifdef DEBUG
    uart_br_print("\nwrite_i2c_address: Returning without writing STOP\n");
#endif
  }
    return nwritten;
}

/*
mystop=True
   print "  Write RegDir to set I/O[7] to output:"
   myslave= 0x21
   mycmd= [0x01, 0x7F]
   nwords= 1
   master_I2C.write(myslave, mycmd, mystop)

mystop=False
   mycmd= [0x01]
   master_I2C.write(myslave, mycmd, mystop)
   res= master_I2C.read( myslave, nwords)
   print "\tPost RegDir: ", res

*/
bool enable_i2c_bridge() {

  bool mystop;
  uint8_t I2CBRIDGE = 0x21;
  uint8_t wordsToRead = 1;
  uint8_t wordsForAddress = 1;
  uint8_t wordsToWrite = 2;

  uart_br_print("\nEnabling I2C bridge:\n");
  buffer[0] = 0x01;
  buffer[1] = 0x7F;
  mystop = true;
#ifdef DEBUG
   uart_br_print("\nWriting 0x01,0x7F to I2CBRIDGE. Stop = true:\n");
#endif
  write_i2c_address(I2CBRIDGE , wordsToWrite , buffer, mystop );

  mystop=false;
  buffer[0] = 0x01;
#ifdef DEBUG
   uart_br_print("\nWriting 0x01 to I2CBRIDGE. Stop = false:\n");
#endif
  write_i2c_address(I2CBRIDGE , wordsForAddress , buffer, mystop );

#ifdef DEBUG
  zero_buffer(buffer , sizeof(buffer));
#endif

#ifdef DEBUG
   uart_br_print("\nReading one word from I2CBRIDGE:\n");
#endif  
  read_i2c_address(I2CBRIDGE, wordsToRead , buffer);

  uart_br_print("Post RegDir: ");
  uart_print_hex_dword(buffer[0]);
  uart_br_print("\n");

  return true; // TODO: return a status, rather than True all the time...
 
}

/*
#EEPROM BEGIN
doEeprom= True
if doEeprom:
  zeEEPROM= E24AA025E48T(master_I2C, 0x57)
  res=zeEEPROM.readEEPROM(0xfa, 6)
  result="  EEPROM ID:\n\t"
  for iaddr in res:
      result+="%02x "%(iaddr)
  print result
#EEPROM END
 */


/* ---------------------------*
 *  Read bytes from PROM      *
 * ---------------------------*/
int16_t  read_i2c_prom( uint8_t startAddress , uint8_t  wordsToRead, uint8_t buffer[] ){

  bool mystop = false;

  buffer[0] = startAddress;
#ifdef DEBUG
  uart_br_print(" read_i2c_prom: Writing device ID: ");
#endif
  write_i2c_address( EEPROMADDRESS , 1 , buffer, mystop );

#ifdef DEBUG
  uart_br_print("read_i2c_prom: Reading memory of EEPROM: ");
  zero_buffer(buffer , wordsToRead);
#endif

  read_i2c_address( EEPROMADDRESS , wordsToRead , buffer);

#ifdef DEBUG
  uart_br_print("Data from EEPROM\n");
  for (uint8_t i=0; i< wordsToRead; i++){
    uart_br_print("\n");
    uart_print_hex_dword(buffer[i]);    
  }
#endif

  return wordsToRead;
}

/* ---------------------------*
 *  Read UID from E24AA025E   *
 * ---------------------------*/
int16_t read_E24AA025E48T(){

  uint8_t startAddress = 0xFA;
  uint8_t wordsToRead = 6;
  //  int16_t status;
  uint16_t uid ;

  //status =  read_i2c_prom( startAddress , wordsToRead, buffer );
  read_i2c_prom( startAddress , wordsToRead, buffer );

  uid = buffer[5] + (buffer[4]<<8);

  return uid; // Returns bottom 16-bits of UID

}

/* ---------------------------*
 *  Read 4 bytes from E24AA025E   *
 * ---------------------------*/
uint32_t read_Prom() {

  uint8_t wordsToRead = 4;
  //  int16_t status;
  uint32_t uid ;

  //status =  read_i2c_prom( startAddress , wordsToRead, buffer );
  read_i2c_prom( PROMMEMORYADDR , wordsToRead, buffer );

  uid = (uint32_t)buffer[3] + ((uint32_t)buffer[2]<<8) + ((uint32_t)buffer[1]<<16) + ((uint32_t)buffer[0]<<24);

  return uid; // Returns 32 word read from PROM

}


int16_t write_Prom(){

  uint8_t wordsToWrite = 4;
 
  int16_t status = 0;
  bool mystop = true;

  uart_br_print("Enter hexadecimal data to write to PROM: 0x");
  uart_scan(command, 9); // 8 hex chars for address plus '\0'
  uint32_t data = hex_str_to_uint32(command);

  // Pack data to write into buffer
  buffer[0] = PROMMEMORYADDR;
  
  for (uint8_t i=0; i< wordsToWrite; i++){
    buffer[i+1] = (data >> (i*8)) & 0xFF ;    
  }

  status = write_i2c_address(EEPROMADDRESS , (wordsToWrite+1), buffer, mystop);

  return status;

}

/*
int16_t write_i2c_prom( uint8_t startAddress , uint8_t wordsToWrite, uint8_t buffer[] ){

  int16_t status = 0;
  bool mystop = true;

  buffer[0] = startAddress;

  for (uint8_t i=0; i< wordsToWrite; i++){
    buffer[i+1] = wordsToWrite;    
  }

  status = write_i2c_address(EEPROMADDRESS , (wordsToWrite+1), buffer, mystop);

  return status;
}
*/

/*
def write(self, addr, data, stop=True):
    """Write data to the device with the given address."""
    # Start transfer with 7 bit address and write bit (0)
    nwritten = -1
    addr &= 0x7f
    addr = addr << 1
    startcmd = 0x1 << 7
    stopcmd = 0x1 << 6
    writecmd = 0x1 << 4
    #Set transmit register (write operation, LSB=0)
    self.data.write(addr)
    #Set Command Register to 0x90 (write, start)
    self.cmd_stat.write(I2CCore.startcmd | I2CCore.writecmd)
    self.target.dispatch()
    ack = self.delayorcheckack()
    if not ack:
        self.cmd_stat.write(I2CCore.stopcmd)
        self.target.dispatch()
        return nwritten
    nwritten += 1
    for val in data:
        val &= 0xff
        #Write slave memory address
        self.data.write(val)
        #Set Command Register to 0x10 (write)
        self.cmd_stat.write(I2CCore.writecmd)
        self.target.dispatch()
        ack = self.delayorcheckack()
        if not ack:
            self.cmd_stat.write(I2CCore.stopcmd)
            self.target.dispatch()
            return nwritten
        nwritten += 1
    if stop:
        self.cmd_stat.write(I2CCore.stopcmd)
        self.target.dispatch()
    return nwritten

*/


/* ------------------------------------------------------------
 * INFO Hex-char-string conversion function
 * PARAM String with hex-chars (zero-terminated)
 * not case-sensitive, non-hex chars are treated as '0'
 * RETURN Conversion result (32-bit)
 * ------------------------------------------------------------ */
uint32_t hex_str_to_uint32(char *buffer) {

  uint16_t length = strlen(buffer);
  uint32_t res = 0, d = 0;
  char c = 0;

  while (length--) {
    c = *buffer++;

    if ((c >= '0') && (c <= '9'))
      d = (uint32_t)(c - '0');
    else if ((c >= 'a') && (c <= 'f'))
      d = (uint32_t)((c - 'a') + 10);
    else if ((c >= 'A') && (c <= 'F'))
      d = (uint32_t)((c - 'A') + 10);
    else
      d = 0;

    res = res + (d << (length*4));
  }

  return res;
}

/* -----------------------------------
 * Convert uint8_t into a decimal string
 * Without using divide - we don't want
 * to implement divide unit and we 
 * don't care about speed.
 * ----------------------------------- */
void uint8_to_decimal_str( uint8_t value , char *buffer) {

  const uint8_t magnitude[3] = {1,10,100};

  uint16_t delta;
  uint16_t trialValue = 0;

  const char ASCII_zero_character = 48;

  buffer[0] = ASCII_zero_character; buffer[1] = ASCII_zero_character; buffer[2] = ASCII_zero_character; buffer[3] = 0;

  //printf("Start, converting %i\n",value);
  //for ( int i =0; i<4; i++){
  //  printf("%i , %i\n",i,buffer[i]);
  //}

  // loop through 100's , 10's and 1's
  for ( int16_t magnitudeIdx =2; magnitudeIdx > -1; magnitudeIdx-- ){

    delta = magnitude[magnitudeIdx];

    // printf("Delta = %i\n",delta);

    // for each magnitude
    for ( uint16_t digit = 0; digit < 10 ; digit ++ ){

      // printf("trialValue = %i\n",trialValue);

      if (( value - ( trialValue + delta )) >= 0) {

	  trialValue += delta;
	  buffer[2-magnitudeIdx] += 1;

	} else {
	  break; // go to the next order of magnitude.
	}
    }
  }

  //for ( int i =0; i<4; i++){
  //  printf("%i , %i\n",i,buffer[i]);
  //}
  return;

}

