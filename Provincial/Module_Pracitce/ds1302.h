#ifndef __DS1302_H__
#define __DS1302_H__

void Write_Ds1302(unsigned  char temp);
void Write_Ds1302_Byte( unsigned char address,unsigned char dat );
unsigned char Read_Ds1302_Byte ( unsigned char address );
void Time_write(unsigned char *dat);
void Time_read(unsigned char *dat);

#endif