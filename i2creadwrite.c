/******************************************************************************
i2creadwrite.c
Raspberry Pi I2C interface with APDS9960
Solan Bongase

A brief demonstration of the Raspberry Pi I2C interface, using the SparkFun APDS9960
This uses the read and write system calls to read and write to registers

Resources:

The init call returns a standard file descriptor.  More detailed configuration
of the interface can be performed using ioctl calls on that descriptor.
Parameters configurable with ioctl are documented here:
https://www.kernel.org/doc/Documentation/i2c/dev-interface

Hardware connections:

This file interfaces with the SparkFun APDS9960
https://www.adafruit.com/product/3595

The board was connected as follows:
(Raspberry Pi)(APDS9960)
GND  -> GND
3.3V -> Vcc
SCL  -> SCL
SDA  -> SDA

To build this file, I use the command:
>  gcc apds9960.c

Then to run it
> ./a.out

This will print out the proximity readings.

Development environment specifics:
Tested on Raspberry Pi V3 hardware, running Raspbian.
******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdbool.h>

#define APDS9960_SLAVE_ADDR 0x39
typedef unsigned char   u8;

int fd;

/*
 * I2CReadByte:
 *	Read an 8 bit value from device
 *********************************************************************************
 */

int I2CReadByte (int fd, u8 *pvalue)
{
  u8 buf[1];
  buf[0] = 0;
  /* Using I2C Read, equivalent of i2c_smbus_read_byte(file) */
  if (read(fd, buf, 1) != 1) {
    /* ERROR HANDLING: i2c transaction failed */
	char err[200];
    sprintf(err, "I2CReadByte");
    perror(err);
	return -1;
  } else {
    /* buf[0] contains the read byte */
	*pvalue = buf[0];
  }
  return 0;
}

/*
 * I2CWriteByte:
 *	Write an 8 or 16-bit value to the given register
 *********************************************************************************
 */

int I2CWriteByte (int fd, u8 value)
{
  u8 buf[1];
  buf[0] = value;
  if (write(fd, buf, 1) != 1) {
    /* ERROR HANDLING: i2c transaction failed */
	char err[200];
    sprintf(err, "I2CWriteByte");
    perror(err);
	return -1;
  }
  return 0;
}
/*
*   combinedI2CReadReg8
*	Read an 8 bit value from a regsiter on the device
*********************************************************************************
 */
int combinedI2CReadReg8 (int fd, u8 reg, u8 *pvalue) {
    int result;
   
    result = I2CWriteByte(fd,reg);
   
    if (result >= 0)
        result = I2CReadByte(fd, pvalue);
   
    return result;
}
/*
 * combinedI2CWriteReg8:
 *	Write an 8 or 16-bit value to the given register
 *********************************************************************************
 */
int combinedI2CWriteReg8 (int fd, u8 reg, u8 value) {
    int result;
   
    result = I2CWriteByte(fd,reg);
	
	if (result >= 0)
		result = I2CWriteByte(fd,value);
	
	return result;
}

int main()
{
   int result;

   // Initialize the interface by giving it an external device ID.
   // The APDS9960 defaults to address 0x39.
   //
   // It returns a standard file descriptor.
   //
   char* device = "/dev/i2c-1";

  if ((fd = open (device, O_RDWR)) < 0) {
    printf( "Unable to open I2C device: %s\n", strerror (errno)) ;
	return -1;
  }

  if (ioctl (fd, I2C_SLAVE, APDS9960_SLAVE_ADDR) < 0) {
    printf("Unable to select I2C device: %s\n", strerror (errno)) ;
	return -1;
  }
   
   u8 dev_id = 0;

   result = combinedI2CReadReg8(fd,0x92,&dev_id);
   if (result >= 0)
       printf("Init result: %d 0x%08x\n",fd,dev_id);
   else {
	   printf("Error reading device ID\n");
	   close(fd);
	   return -1;
   }

    // Setup
    if (result != -1)
       result = combinedI2CWriteReg8(fd, 0x89, 0 ); // Promximity Low Threshold
    if (result != -1)
       result = combinedI2CWriteReg8(fd, 0x8B, 175 ); // Promximity High Threshold
    if (result != -1)
       result = combinedI2CWriteReg8(fd, 0x8C, 0xC0 ); // Promximity Interrupt Persistance
    if (result != -1)
        result = combinedI2CWriteReg8(fd, 0x80, 0x25 ); // Promximity Enable - PIEN, PEN, PON

    u8 pvalid;
    u8 pdata;
    while(fd > 0 && result != -1) {
        // first read PVALID
        result = combinedI2CReadReg8(fd,0x93,&pvalid);
        pvalid &= 0x2;
        // then read PDATA
        if (result != -1 && pvalid) {
            result = combinedI2CReadReg8(fd,0x9C,&pdata);
            printf("%d\n",pdata);
        }
    }
    close(fd);
}
