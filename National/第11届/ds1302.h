#ifndef __DS1302_H_
#define __DS1302_H_
void Write_Ds1302(unsigned  char temp);
void Write_Ds1302_Byte( unsigned char address,unsigned char dat );
unsigned char Read_Ds1302_Byte ( unsigned char address );
void time_write(unsigned char *time);
void time_read(unsigned char *time);
#endif