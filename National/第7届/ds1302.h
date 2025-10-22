#ifndef __DS1302_H_
#define __DS1302_H_

void Write_Ds1302(unsigned  char temp);
void Write_Ds1302_Byte( unsigned char address,unsigned char dat );
unsigned char Read_Ds1302_Byte ( unsigned char address );
void Time_Read(unsigned char *time);
void Time_Write(unsigned char *time);
	
#endif
