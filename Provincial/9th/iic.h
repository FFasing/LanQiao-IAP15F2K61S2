#ifndef __IIC_H__
#define __IIC_H__

static void I2C_Delay(unsigned char n);
void I2CStart(void);
void I2CStop(void);
void I2CSendByte(unsigned char byt);
unsigned char I2CReceiveByte(void);
unsigned char I2CWaitAck(void);
void I2CSendAck(unsigned char ackbit);
void DA_Write(unsigned char dat);
void eeprom_write(unsigned char *String,unsigned char addr,unsigned char num);
void eeprom_read(unsigned char *String,unsigned char addr,unsigned char num);

#endif