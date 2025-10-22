#ifndef __DS1302_H__
#define __DS1302_H__

void Write_Ds1302(unsigned  char temp);
void Write_Ds1302_Byte( unsigned char address,unsigned char dat ); 
unsigned char Read_Ds1302_Byte ( unsigned char address );
void Rtc_Write(unsigned char *dat);
void Rtc_Read(unsigned char *dat);

#endif