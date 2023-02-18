// entrypoint includes
#include <stdio.h>
#include <string.h>

// bmp180 includes
#include <fcntl.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <math.h>

// addition smbus includes
#include <linux/types.h>
#include <errno.h>

/*** smbus ***/
/* Compatibility defines */
#ifndef I2C_SMBUS_I2C_BLOCK_BROKEN
#define I2C_SMBUS_I2C_BLOCK_BROKEN I2C_SMBUS_I2C_BLOCK_DATA
#endif
#ifndef I2C_FUNC_SMBUS_PEC
#define I2C_FUNC_SMBUS_PEC I2C_FUNC_SMBUS_HWPEC_CALC
#endif

__s32 i2c_smbus_access(int file, char read_write, __u8 command,
		       int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;
	__s32 err;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;

	err = ioctl(file, I2C_SMBUS, &args);
	if (err == -1)
		err = -errno;
	return err;
}


__s32 i2c_smbus_write_quick(int file, __u8 value)
{
	return i2c_smbus_access(file, value, 0, I2C_SMBUS_QUICK, NULL);
}

__s32 i2c_smbus_read_byte(int file)
{
	union i2c_smbus_data data;
	int err;

	err = i2c_smbus_access(file, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
	if (err < 0)
		return err;

	return 0x0FF & data.byte;
}

__s32 i2c_smbus_write_byte(int file, __u8 value)
{
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, value,
				I2C_SMBUS_BYTE, NULL);
}

__s32 i2c_smbus_read_byte_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	int err;

	err = i2c_smbus_access(file, I2C_SMBUS_READ, command,
			       I2C_SMBUS_BYTE_DATA, &data);
	if (err < 0)
		return err;

	return 0x0FF & data.byte;
}

__s32 i2c_smbus_write_byte_data(int file, __u8 command, __u8 value)
{
	union i2c_smbus_data data;
	data.byte = value;
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, command,
				I2C_SMBUS_BYTE_DATA, &data);
}

__s32 i2c_smbus_read_word_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	int err;

	err = i2c_smbus_access(file, I2C_SMBUS_READ, command,
			       I2C_SMBUS_WORD_DATA, &data);
	if (err < 0)
		return err;

	return 0x0FFFF & data.word;
}

__s32 i2c_smbus_write_word_data(int file, __u8 command, __u16 value)
{
	union i2c_smbus_data data;
	data.word = value;
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, command,
				I2C_SMBUS_WORD_DATA, &data);
}

__s32 i2c_smbus_process_call(int file, __u8 command, __u16 value)
{
	union i2c_smbus_data data;
	data.word = value;
	if (i2c_smbus_access(file, I2C_SMBUS_WRITE, command,
			     I2C_SMBUS_PROC_CALL, &data))
		return -1;
	else
		return 0x0FFFF & data.word;
}

/* Returns the number of read bytes */
__s32 i2c_smbus_read_block_data(int file, __u8 command, __u8 *values)
{
	union i2c_smbus_data data;
	int i, err;

	err = i2c_smbus_access(file, I2C_SMBUS_READ, command,
			       I2C_SMBUS_BLOCK_DATA, &data);
	if (err < 0)
		return err;

	for (i = 1; i <= data.block[0]; i++)
		values[i-1] = data.block[i];
	return data.block[0];
}

__s32 i2c_smbus_write_block_data(int file, __u8 command, __u8 length,
				 const __u8 *values)
{
	union i2c_smbus_data data;
	int i;
	if (length > I2C_SMBUS_BLOCK_MAX)
		length = I2C_SMBUS_BLOCK_MAX;
	for (i = 1; i <= length; i++)
		data.block[i] = values[i-1];
	data.block[0] = length;
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, command,
				I2C_SMBUS_BLOCK_DATA, &data);
}

/* Returns the number of read bytes */
/* Until kernel 2.6.22, the length is hardcoded to 32 bytes. If you
   ask for less than 32 bytes, your code will only work with kernels
   2.6.23 and later. */
__s32 i2c_smbus_read_i2c_block_data(int file, __u8 command, __u8 length,
				    __u8 *values)
{
	union i2c_smbus_data data;
	int i, err;

	if (length > I2C_SMBUS_BLOCK_MAX)
		length = I2C_SMBUS_BLOCK_MAX;
	data.block[0] = length;

	err = i2c_smbus_access(file, I2C_SMBUS_READ, command,
			       length == 32 ? I2C_SMBUS_I2C_BLOCK_BROKEN :
				I2C_SMBUS_I2C_BLOCK_DATA, &data);
	if (err < 0)
		return err;

	for (i = 1; i <= data.block[0]; i++)
		values[i-1] = data.block[i];
	return data.block[0];
}

__s32 i2c_smbus_write_i2c_block_data(int file, __u8 command, __u8 length,
				     const __u8 *values)
{
	union i2c_smbus_data data;
	int i;
	if (length > I2C_SMBUS_BLOCK_MAX)
		length = I2C_SMBUS_BLOCK_MAX;
	for (i = 1; i <= length; i++)
		data.block[i] = values[i-1];
	data.block[0] = length;
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, command,
				I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
}

/* Returns the number of read bytes */
__s32 i2c_smbus_block_process_call(int file, __u8 command, __u8 length,
				   __u8 *values)
{
	union i2c_smbus_data data;
	int i, err;

	if (length > I2C_SMBUS_BLOCK_MAX)
		length = I2C_SMBUS_BLOCK_MAX;
	for (i = 1; i <= length; i++)
		data.block[i] = values[i-1];
	data.block[0] = length;

	err = i2c_smbus_access(file, I2C_SMBUS_WRITE, command,
			       I2C_SMBUS_BLOCK_PROC_CALL, &data);
	if (err < 0)
		return err;

	for (i = 1; i <= data.block[0]; i++)
		values[i-1] = data.block[i];
	return data.block[0];
}

/*** BMP180 read out **/
#define BMP180_I2C_ADDRESS 0x77

// Set default calibration values from values in the datasheet example
// After that exact values will be read from BMP180/BMP085 sensor

struct calibrate {
	short int ac1;
	short int ac2;
	short int ac3;
	unsigned short int ac4;
	unsigned short int ac5;
	unsigned short int ac6;
	short int b1;
	short int b2;
	short int mb;
	short int mc;
	short int md;
} cal;


const unsigned char BMP085_OVERSAMPLING_SETTING = 3;

// Open a connection to the bmp085
// Returns a file id
int begin()
{
	int fd = 0;
	char *fileName = "/dev/i2c-1";
	
	// Open port for reading and writing
	if ((fd = open(fileName, O_RDWR)) < 0)
	{
		exit(1);
	}
	
	// Set the port options and set the address of the device
	if (ioctl(fd, I2C_SLAVE, BMP180_I2C_ADDRESS) < 0) 
	{					
		close(fd);
		exit(1);
	}

	return fd;
}

// Read two words from the BMP085 and supply it as a 16 bit integer
__s32 i2cReadInt(int fd, __u8 address)
{
	__s32 res = i2c_smbus_read_word_data(fd, address); // a word contains 16 bit in smbus protocol definition 
	if (0 > res) 
	{
		close(fd);
		exit(1);
	}

	// Convert result to 16 bits and swap bytes
	res = ((res<<8) & 0xFF00) | ((res>>8) & 0xFF); // UT = MSB << 8 + LSB due to masking with 0xFF00 = 1111111100000000  and 0xFF = 0000000011111111

	return res;
}

//Write a byte to the BMP085
void i2cWriteByteData(int fd, __u8 address, __u8 value)
{
	if (0 > i2c_smbus_write_byte_data(fd, address, value)) 
	{
		close(fd);
		exit(1);
	}
}

// Read a block of data BMP085
void i2cReadBlockData(int fd, __u8 address, __u8 length, __u8 *values)
{
	if (0 > i2c_smbus_read_i2c_block_data(fd, address,length,values)) 
	{
		close(fd);
		exit(1);
	}
}


void calibration()
{
	int fd = begin();
	cal.ac1 = i2cReadInt(fd,0xAA);
	cal.ac2 = i2cReadInt(fd,0xAC);
	cal.ac3 = i2cReadInt(fd,0xAE);
	cal.ac4 = i2cReadInt(fd,0xB0);
	cal.ac5 = i2cReadInt(fd,0xB2);
	cal.ac6 = i2cReadInt(fd,0xB4);
	cal.b1 = i2cReadInt(fd,0xB6);
	cal.b2 = i2cReadInt(fd,0xB8);
	cal.mb = i2cReadInt(fd,0xBA);
	cal.mc = i2cReadInt(fd,0xBC);
	cal.md = i2cReadInt(fd,0xBE);
	close(fd);
}

// Read the uncompensated temperature value
unsigned int readRawTemperature()
{
	int fd = begin();

	// Write 0x2E into Register 0xF4
	// This requests a temperature reading
	i2cWriteByteData(fd,0xF4,0x2E);
	
	// Wait at least 4.5ms - see datasheet https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf page 21 ("Max. conversion time [ms]")
	usleep(5000);

	// Read the two byte result from address 0xF6
	unsigned int ut = i2cReadInt(fd,0xF6); // read MSB (most significant bit first/big endian) from the i2c bus, see https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf page 18

	// Close the i2c file
	close (fd);
	
	return ut;
}

// Read the uncompensated pressure value
unsigned int readRawPressure()
{
	int fd = begin();

	// Write 0x34+(BMP085_OVERSAMPLING_SETTING<<6) into register 0xF4
	// Request a pressure reading w/ oversampling setting
	i2cWriteByteData(fd,0xF4,0x34 + (BMP085_OVERSAMPLING_SETTING<<6));

	// Wait for conversion, delay time dependent on oversampling setting
	usleep((2 + (3<<BMP085_OVERSAMPLING_SETTING)) * 1000);

	// Read the three byte result from 0xF6
	// 0xF6 = MSB, 0xF7 = LSB and 0xF8 = XLSB
	__u8 values[3];
	i2cReadBlockData(fd, 0xF6, 3, values);

	unsigned int up = (((unsigned int) values[0] << 16) | ((unsigned int) values[1] << 8) | (unsigned int) values[2]) >> (8-BMP085_OVERSAMPLING_SETTING);

	// Close the i2c file
	close (fd);
	
	return up;
}

/*** Convert to read measurement values ***/
int compensateTemperature()
{
	unsigned int ut = readRawTemperature();
	int x1 = (((int)ut - (int)cal.ac6)*(int)cal.ac5) >> 15;
 	int x2 = ((int)cal.mc << 11)/(x1 + cal.md);
	return x1 + x2; // = B5
}

// Calculate pressure given uncalibrated pressure
// Value returned will be in units of Pa
int getPressure()
{
	unsigned int up = readRawPressure();

	int b6 = compensateTemperature() - 4000;
	// Calculate B3
	int x1 = (cal.b2 * (b6 * b6)>>12)>>11;
	int x2 = (cal.ac2 * b6)>>11;
	int x3 = x1 + x2;
	int b3 = (((((int) cal.ac1)*4 + x3)<<BMP085_OVERSAMPLING_SETTING) + 2)>>2;
  
	// Calculate B4
	x1 = (cal.ac3 * b6)>>13;
	x2 = (cal.b1 * ((b6 * b6)>>12))>>16;
	x3 = ((x1 + x2) + 2)>>2;
	unsigned int b4 = (cal.ac4 * (unsigned int)(x3 + 32768))>>15;
  
	unsigned int b7 = ((unsigned int)(up - b3) * (50000>>BMP085_OVERSAMPLING_SETTING));
	int p = (b7 < 0x80000000) ? (b7<<1)/b4 : (b7/b4)<<1;
	x1 = (p>>8) * (p>>8);
	x1 = (x1 * 3038)>>16;
	x2 = (-7357 * p)>>16;
	p += (x1 + x2 + 3791)>>4;
  
	return p;
}

// Calculate temperature given uncalibrated temperature
double getTemperature()
{
	// calculation is based on https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf page 15
	// Retrieve temperature in units of 0.1 deg C
	int rawTemperature = ((compensateTemperature() + 8)>>4); // T = (B5 + 8)/2^4 (in 0.1 degree C), where right shift by 4 is the bit shifting equivalent to power of 4 since the binary system is on basis 2
	return ((double)rawTemperature)/10;
}

double getAltitude()
{
	double pressure = getPressure();
	// Sea level pressure: 101325.0
	return 44330.0 * (1.0 - pow(pressure / 101325.0, (1.0/5.255)));
}

/*** Entry point ***/
int main(int argc, char **argv)
{
        calibration();

        printf("Temperature\t%0.1f C\n", getTemperature());
        printf("Pressure\t%0.2f hPa\n", (double)getPressure()/100);
        printf("Altitude\t%0.2f m\n", getAltitude());

        return 0;
}

/*
Ressources:
https://docs.kernel.org/i2c/index.html
- Setting the read and write option is done by smbus
- The file descriptor does NOT need to be closed in order to send an restart command
- https://dri.freedesktop.org/docs/drm/i2c/dev-interface.html --> User space i2c communication
- https://dri.freedesktop.org/docs/drm/i2c/smbus-protocol.html --> smbus userspace
- https://docs.kernel.org/driver-api/i2c.html --> linux kernel driver api for i2c
- https://docs.kernel.org/i2c/i2c-protocol.html --> i2c protocol explained
- https://docs.kernel.org/i2c/smbus-protocol.html --> SMBus protocol explained
- https://www.youtube.com/watch?v=j9yx8LOslng --> i2c explained
- 0xEE is the device address 0x77 (01110111) << 1 and set the LSB to 1 for write und 0xEF (11101110) with LSB to 0 for read
- https://www.youtube.com/watch?v=_fgWQ3TIhyE --> i2c intro basics (good one!)
- https://github.com/leon-anavi/rpi-examples/blob/master/BMP180/c/BMP180.c --> Reference code for this C file
*/