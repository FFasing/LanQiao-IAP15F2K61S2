#ifndef __IIC_H_
#define __IIC_H_

static void I2C_Delay(unsigned char n);
void I2CStart(void);
void I2CStop(void);
void I2CSendByte(unsigned char byt);
unsigned char I2CReceiveByte(void);
unsigned char I2CWaitAck(void);
void I2CSendAck(unsigned char ackbit);
unsigned char ad_read(unsigned char addr);
void eeprom_write(unsigned char *string,unsigned char addr,unsigned char num);
void eeprom_read(unsigned char *string,unsigned char addr,unsigned char num);

#endif
