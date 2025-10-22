#ifndef __IIC_H__
#define __IIC_H__

static void I2C_Delay(unsigned char n);
void I2CStart(void);
void I2CStop(void);
void I2CSendByte(unsigned char byt);
unsigned char I2CReceiveByte(void);
unsigned char I2CWaitAck(void);
void I2CSendAck(unsigned char ackbit);
unsigned char AD_read(unsigned char addr);
void EEPROM_Write(unsigned char *String,addr,num);
void EEPROM_Read(unsigned char *String,addr,num);

#endif