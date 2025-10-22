#include <STC15F2K60S2.H>//4h
#include <iic.h>
#include <ds1302.h>

sbit ROW1=P3^0;
sbit ROW2=P3^1;
sbit ROW3=P3^2;
sbit ROW4=P3^3;
sbit COL1=P4^4;
sbit COL2=P4^2;
sbit COL3=P3^5;
sbit COL4=P3^4;

code unsigned char Seg_Table[] =
{
	0xc0,//0
	0xf9,//1
	0xa4,//2
	0xb0,//3
	0x99,//4
	0x92,//5
	0x82,//6
	0xf8,//7
	0x80,//8
	0x90,//9
	0x88,//A
	0x83,//b
	0xc6,//C
	0xa1,//d
	0x86,//E
	0x8e,//F
	0xff,//16
	0xbf//- 17
};
code unsigned char Seg_Wale[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char seg_slow_down,key_slow_down;
unsigned char seg_index,seg_buf[]={16,16,16,16,16,16,16,16};
unsigned char led_buf[]={0,0,0,0,0,0,0,0};
unsigned char key_val,key_old,key_down,key_up;
unsigned int Timer_1000,Timer_200;
unsigned int freq;
unsigned int V_RB2;
bit mod1_flash;
char time_set;
unsigned char event_mod;

unsigned char VL,VH;
unsigned char time[]={0x23,0x59,0x55};
unsigned char time_save[]={0,0,0};
unsigned char word_mod;	//0-rull 1-time 2-V_RB2 3-VLVH_SET 4-freq 5-search
unsigned char mod1_set,mod2_set,mod3_set,mod4_set;
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
}
void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;
	EA = 1;
}
void system_init(void)
{
	P2 = P2 & 0X1f |0x80;//led
	P0 = 0xff;
	P2 = P2 & 0X1f |0xa0;//other
	P0 = 0x00;
	P2 = P2 & 0X1f ;
}
void seg(unsigned char wale,valu)
{
	P2 = P2 & 0X1f |0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0X1f |0xe0;
	P0 = Seg_Table[16];
	P2 = P2 & 0X1f |0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0X1f |0xe0;
	P0 = Seg_Table[valu];
	P2 = P2 & 0X1f ;
}
unsigned char key_read(void)
{
	unsigned char val=0;
	ROW1=ROW2=ROW3=ROW4=1;
	COL1=0;COL2=COL3=COL4=1;
	if(ROW4==0) val=4;
	if(ROW3==0) val=5;
	if(ROW2==0) val=6;
	if(ROW1==0) val=7;
	COL2=0;COL1=COL3=COL4=1;
	if(ROW4==0) val=8;
	if(ROW3==0) val=9;
	if(ROW2==0) val=10;
	if(ROW1==0) val=11;
	return val;
}
void dat_read(void)
{
	if(Timer_200) return;
	Timer_200=1;
	V_RB2=(float)((ad_read(0x43)*10.0/3.0)/17.0*100.0);
	Time_Read(time);
		if((V_RB2/100)<=(int)VL) 
		{
			event_mod=0;
			eeprom_write(time,1,3);
		}
		if((V_RB2/100)>=(int)VH) 
		{
			event_mod=1;
			eeprom_write(time,1,3);
		}
		
}
void key_proc(void)
{
	unsigned char temp;
	if(key_slow_down) return;
	key_slow_down=1;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	switch(key_down)
	{
		case 9:
			word_mod=5;
			mod4_set=0;
		break;
		
		case 7:
		if(word_mod==1) mod1_set++;
		if(mod1_set==1) {Time_Read(time);time_set=time[0];}
		if(mod1_set==2) {Time_Read(time);time_set=time[1];}
		if(mod1_set==3) {Time_Read(time);time_set=time[2];}
		if(mod1_set==4) mod1_set=0;
		if(word_mod!=1)
		{
			word_mod=1;mod1_set=0;
		}
		break;
		
		case 6:
			word_mod=2;
			mod2_set=0;
		break;
		
		case 4:
			if(word_mod==2) 
			{
				word_mod=3;
				mod2_set=0;
			}
			if(word_mod==3) 
			{
				mod2_set++;
				if(mod2_set==3) 
				{
					word_mod=2;
					mod2_set=0;
				}
			}
			if(word_mod==4)
			{
				mod3_set^=1;
			}
			if(word_mod==5)
			{
				mod4_set^=1;
				if(mod4_set==1)
				{
					eeprom_read(time_save,1,3);
				}
			}
			break;
			
		case 5:
			word_mod=4;
			mod3_set=0;
		break;
		
		case 10:
			if(word_mod==3)
			{
				switch(mod2_set)
				{
					case 2:
						VL-=5;
						if(VL<=0) VL=0;
						eeprom_write(&VL,15,1);
					break;
					case 1:
						VH-=5;
						if(VH<=0) VH=0;
						eeprom_write(&VH,16,1);
					break;
				}
			}
			if(word_mod==1)
			{
				if(mod1_set==1) 
				{
					if(time_set==0) 	time_set=35;
					else
					{
					time_set--;
					if(time_set%16==15) 	time_set-=6;
					}
					time[0]=time_set;
				}
				if(mod1_set==2) //89
				{
					if(time_set==0) 	time_set=89;
					else
					{
					time_set--;
					if(time_set%16==15) 	time_set-=6;
					}
					time[1]=time_set;
				}
				if(mod1_set==3) 
				{
					if(time_set==0) 	time_set=89;
					else
					{
					time_set--;
					if(time_set%16==15) 	time_set-=6;
					}
					time[2]=time_set;
				}
				Time_Write(time);
			}
		break;
			
		case 11:
			if(word_mod==3)
			{
				switch(mod2_set)
				{
					case 2:
						VL+=5;
						if(VL>=95) VL=95;
						eeprom_write(&VL,15,1);
					break;
					case 1:
						VH+=5;
						if(VH>=95) VH=95;
						eeprom_write(&VH,16,1);
					break;
				}
			}
			if(word_mod==1)
			{
				if(mod1_set==1) 
				{
					time_set++;
					if(time_set%16==10) 	time_set+=6;
					if(time_set/16==2&&time_set%16==4)	time_set=0;
					time[0]=time_set;
				}
				if(mod1_set==2) 
				{
					time_set++;
					if(time_set%16==10) 	time_set+=6;
					if(time_set/16==6)	time_set=0;
					time[1]=time_set;
				}
				if(mod1_set==3) 
				{
					time_set++;
					if(time_set%16==10) 	time_set+=6;
					if(time_set/16==6)	time_set=0;
					time[2]=time_set;
				}
				Time_Write(time);
			}
		break;
	}
}
void seg_proc(void)
{
	unsigned char i;
	if(seg_slow_down) return;
	seg_slow_down=1;
	for(i=0;i<8;i++) seg_buf[i]=16;

	switch(word_mod)
	{
		case 1:
			if(word_mod==1)
			{
				if(mod1_set==1)
				{
					mod1_flash?(seg_buf[0]=time_set/16):0;
					mod1_flash?(seg_buf[1]=time_set%16):0;
				}
				else
				{
					seg_buf[0]=time[0]/16;
					seg_buf[1]=time[0]%16;
				}
				if(mod1_set==2)
				{
					mod1_flash?(seg_buf[3]=time_set/16):0;
					mod1_flash?(seg_buf[4]=time_set%16):0;
				}
				else
				{
					seg_buf[3]=time[1]/16;
					seg_buf[4]=time[1]%16;
				}
				if(mod1_set==3)
				{
					mod1_flash?(seg_buf[6]=time_set/16):0;
					mod1_flash?(seg_buf[7]=time_set%16):0;
				}
				else
				{
					seg_buf[6]=time[2]/16;
					seg_buf[7]=time[2]%16;
				}
				seg_buf[5]=seg_buf[2]=17;
			}
		break;
		case 2:
			if(word_mod==2)
			{
				seg_buf[0]=17;
				seg_buf[1]=1;
				seg_buf[2]=17;
				seg_buf[4]=V_RB2/1000%10;
				seg_buf[5]=V_RB2/100%10;
				seg_buf[6]=V_RB2/10%10;
				seg_buf[7]=V_RB2%10;
			}
		break;
		case 3:
			if(word_mod==3)
			{
				if(mod2_set==1)
				{
					mod1_flash?(seg_buf[0]=VH/10%10):0;
					mod1_flash?(seg_buf[1]=VH%10):0;
					mod1_flash?(seg_buf[2]=0):0;
					mod1_flash?(seg_buf[3]=0):0;
				}
				else
				{
					seg_buf[0]=VH/10%10;
					seg_buf[1]=VH%10;
					seg_buf[2]=0;
					seg_buf[3]=0;
				}
				if(mod2_set==2)
				{
					mod1_flash?(seg_buf[4]=VL/10%10):0;
					mod1_flash?(seg_buf[5]=VL%10):0;
					mod1_flash?(seg_buf[6]=0):0;
					mod1_flash?(seg_buf[7]=0):0;
				}
				else
				{
					seg_buf[4]=VL/10%10;
					seg_buf[5]=VL%10;
					seg_buf[6]=0;
					seg_buf[7]=0;
				}
			}
		break;
		case 4:
			if(word_mod==4)
			{
				switch(mod3_set)
				{
					case 0:
						seg_buf[0]=17;
						seg_buf[1]=2;
						seg_buf[2]=17;
						seg_buf[3]=freq/10000%10;
						seg_buf[4]=freq/1000%10;
						seg_buf[5]=freq/100%10;
						seg_buf[6]=freq/10%10;
						seg_buf[7]=freq%10;
					break;
					case 1:
						seg_buf[0]=17;
						seg_buf[1]=2;
						seg_buf[2]=17;
						seg_buf[3]=0;
						seg_buf[4]=1;
						seg_buf[5]=0;
						seg_buf[6]=0;
						seg_buf[7]=0;
					break;
				}
			}
		break;
		case 5:
		if(word_mod==5)
		{
			switch(mod4_set)
			{
				case 0:
					seg_buf[6]=event_mod/10%10;
					seg_buf[7]=event_mod%10;
				break;
				case 1:
					seg_buf[0]=time_save[0]/16;
					seg_buf[1]=time_save[0]%16;
					seg_buf[3]=time_save[1]/16;
					seg_buf[4]=time_save[1]%16;
					seg_buf[6]=time_save[2]/16;
					seg_buf[7]=time_save[2]%16;
				break;
			}
		}
	break;
	}
	
}
void Timer1_rountine(void) interrupt 3
{
	if(++Timer_200==200) Timer_200=0;
	if(++seg_slow_down==50) seg_slow_down=0;
	if(++key_slow_down==100) key_slow_down=0;
	if(++seg_index==8) seg_index=0;
	seg(seg_index,seg_buf[seg_index]);
	seg_proc();key_proc();
	if(++Timer_1000==1000)
	{
		Timer_1000=0;
		freq=(TH0<<8)|TL0;
		TH0=TL0=0;
		mod1_flash^=1;
	}
}
void main()
{
//	eeprom_read(time,1,3);
	eeprom_read(&VH,16,1);
	system_init();
	Timer0_Init();
	Timer1_Init();
//	eeprom_write(&VL,15,1);
	eeprom_read(&VL,15,1);
	Time_Write(time);
//	eeprom_write(&VH,20,1);
	while(1)
	{
		dat_read();
	}
}