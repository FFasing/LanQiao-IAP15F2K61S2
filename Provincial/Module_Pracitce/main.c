#include <STC15F2K60S2.H>
#include <intrins.h>
#include <ds1302.h>
#include <onewire.h>
#include <iic.h>

#define uchar unsigned char 
#define uint unsigned int

sbit ROW1=P3^0;
sbit ROW2=P3^1;
sbit ROW3=P3^2;
sbit ROW4=P3^3;
sbit COL1=P4^4;
sbit COL2=P4^2;
sbit COL3=P3^5;
sbit COL4=P3^4;

sbit us_tx=P1^0;
sbit us_rx=P1^1;

code unsigned char Eep_Table[]={0,1,2,3,4,5,6,7,8,9,12,15,11,16,13,17};
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
0xff,//10
0xc6,//C 11
0x88,//A 12
0x86,//E 13
0x8c,//P 14
0x83,//b 15
0xa1,//d 16 
0x8e,//F 17
0xaf,//r 18
0x98,//q 19
0xcf,//l 20
0xc1,//U 21
0x92,//S 22
0xc7//L 23
};
code unsigned char Seg_Wale[] =	{0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
uchar seg_bus[]={10,10,10,10,10,10,10,10};
uchar point_bus[]={0,0,0,0,0,0,0,0};
uchar led_bus[]={0,0,0,0,0,0,0,0};
uchar seg_slow_down,key_slow_down;
uchar seg_index;
uchar key_val,key_old,key_down,key_up;
uchar V_RB2,V_RD1;
uint Timer_100,Timer_1000;
bit relay_con=0;
bit beep_con=0;
uchar distances;
uchar word_index=0,led_index=0;
char word_buf[]={0,0,0,0};

uchar seg_show_mod=8;	
//0-Time(ds1302) 1-temperatur(ds18b20) 2-AD(fcp8591) 3-EEPROM(at24c02) 
//4-freq 		 5-relay			   6-beep 		 7-ultrasonic      
//8-led			 9-word
uchar Time[3]={0x23,0x59,0x50};
float temperatur;
uchar eep_save=0;
uint freq;


void system_init()
{
	P2 = P2 & 0x1f | 0x80;
	P0 = 0xff;
	P2 = P2 & 0x1f | 0xa0;
	P0 = 0x00;
}
void seg(uchar wale,valu,point)
{
	P2 = P2 & 0x1f | 0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f | 0xe0;
	P0 = Seg_Table[10];
	P2 = P2 & 0x1f | 0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f | 0xe0;
	P0 = Seg_Table[10];
	if(point) 
		P0 &= 0x7f;
	P0 &= Seg_Table[valu];
}
uchar key_read()
{
	uchar i=0;
	ROW1=ROW2=ROW3=ROW4=1;
	COL1=0;COL2=COL3=COL4=1;
	if(ROW4==0) i=4;
	if(ROW3==0) i=5;
	if(ROW2==0) i=6;
	if(ROW1==0) i=7;
	COL2=0;COL1=COL3=COL4=1;
	if(ROW4==0) i=8;
	if(ROW3==0) i=9;
	if(ROW2==0) i=10;
	if(ROW1==0) i=11;
	COL3=0;COL2=COL1=COL4=1;
	if(ROW4==0) i=12;
	if(ROW3==0) i=13;
	if(ROW2==0) i=14;
	if(ROW1==0) i=15;
	COL4=0;COL2=COL3=COL1=1;
	if(ROW4==0) i=16;
	if(ROW3==0) i=17;
	if(ROW2==0) i=18;
	if(ROW1==0) i=19;
	return i;
}
void led(uchar addr,enable)
{
	uchar val=0x00,old=0xff;
	if(enable)
		val|=0x01<<addr;
	else
		val&=~(0x01<<addr);
	if(old!=val)
	{
		P2 = P2 & 0x1f | 0x80;
		P0 =~val;
		old=val;
	}
}
void other(uchar relay,beep)
{
	uchar val=0x00,old=0xff;
	if(relay)
		val|=0x10;
	else
		val&=~0x10;
	if(beep)
		val|=0x40;//0100
	else
		val&=~0x40;
	if(old!=val)
	{
		P2 = P2 & 0x1f | 0xa0;
		P0 = val;
		old=val;
	}
}
void Timer0_Init(void)		//@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;
	EA = 1;
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
	
void Delay20us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 57;
	while (--i);
}
void ul_wave()
{
	unsigned char i;
	for(i=0;i<10;i++)
	{
		us_tx=1;
		Delay20us();
		us_tx=0;
		Delay20us();
	}
}
void ultrasonic(void)
{
	TMOD&=0xF0;
	TR0=0;
	TH0=TL0=0;
	ul_wave();
	TR0=1;
	while(us_rx==1&&TF0==0);
	TR0=0;
	if(TF0==1)
	{
		TF0=0;
		distances=0;
	}
	else
		distances=((TH0<<8)|TL0)*0.017;
}
void led_proc()
{
	
}
void key_proc()
{
	if(key_slow_down) return;
	key_slow_down=1;
	
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	
	switch(key_down)
	{
		case 4:
			seg_show_mod--;
			if((seg_show_mod==5)||(seg_show_mod==6)) {Timer0_Init();Timer1_Init();}
		break;
		case 5:
			seg_show_mod++;
			if((seg_show_mod==5)||(seg_show_mod==6)) {Timer0_Init();Timer1_Init();}
		break;
		case 8:
			if(seg_show_mod==3) 
			{
				eep_save++;
				if(eep_save==16) eep_save=0;
				EEPROM_Write(&eep_save,20,1);
			}
			if(seg_show_mod==5) 
			{
				relay_con^=1;
			}
			if(seg_show_mod==6) 
			{
				beep_con^=1;
			}
			if(seg_show_mod==7) 
			{
				ultrasonic();
			}
			if(seg_show_mod==8) 
			{
				led_index++;
				if(led_index==8) led_index=0;
			}
			if(seg_show_mod==9) 
			{
				word_index++;
				if(word_index==4) word_index=0;
				EEPROM_Write(word_buf,1,4);
			}
		break;
		case 9:
			if(seg_show_mod==8) 
			{
				led_bus[led_index]^=1;
			}
		break;
		case 13:
			if(seg_show_mod==9) 
			{
				word_buf[word_index]++;
				if(word_buf[word_index]==16) word_buf[word_index]=0;
				EEPROM_Write(word_buf,1,4);
			}
		break;
		case 12:
			if(seg_show_mod==9) 
			{
				word_buf[word_index]--;
				if(word_buf[word_index]==-1) word_buf[word_index]=15;
				EEPROM_Write(word_buf,1,4);
			}
		break;
	}
}
void seg_proc()
{
	uchar i;
	uint v_rb2,v_rd1;
	if(seg_slow_down) return;
	seg_slow_down=1;
	for(i=0;i<8;i++)
	{
		seg_bus[i]=10;
		point_bus[i]=0;
	}
	switch(seg_show_mod)
	{
		case 0:	
			Time_read(Time);
			seg_bus[0]=Time[0]/16;
			seg_bus[1]=Time[0]%16;
			seg_bus[3]=Time[1]/16;
			seg_bus[4]=Time[1]%16;
			seg_bus[6]=Time[2]/16;
			seg_bus[7]=Time[2]%16;
		break;
		case 1:
			seg_bus[0]=11;
			seg_bus[4]=(int)(temperatur/10.0)%10;
			seg_bus[5]=(int)(temperatur)%10;
			point_bus[5]=1;
			seg_bus[6]=(int)(temperatur*10.0)%10;
			seg_bus[7]=(int)(temperatur*100.0)%10;
		break;
		case 2:
			v_rb2=(int)V_RB2*10/51*10;
			v_rd1=(int)V_RD1*10/51*10;
			seg_bus[0]=12;
			seg_bus[1]=v_rb2/100%10;
			point_bus[1]=1;
			seg_bus[2]=v_rb2/10%10;
			seg_bus[3]=v_rb2%10;
			seg_bus[5]=v_rd1/100%10;
			point_bus[5]=1;
			seg_bus[6]=v_rd1/10%10;
			seg_bus[7]=v_rd1%10;
		break;
		case 3:
			seg_bus[0]=13;
			seg_bus[1]=13;
			seg_bus[2]=14;
			seg_bus[7]=Eep_Table[eep_save];
		break;
		case 4:
			seg_bus[0]=Eep_Table[15];//f
			seg_bus[1]=18;//r
			seg_bus[2]=Eep_Table[14];//e
			seg_bus[3]=freq/10000%10;
			seg_bus[4]=freq/1000%10;
			seg_bus[5]=freq/100%10;
			seg_bus[6]=freq/10%10;
			seg_bus[7]=freq/1%10;
		break;
		case 5:
			seg_bus[0]=18;//r
			seg_bus[1]=20;
			seg_bus[2]=Eep_Table[10];//a
			seg_bus[7]=relay_con;
		break;
		case 6:
			seg_bus[0]=Eep_Table[11];
			seg_bus[1]=Eep_Table[14];
			seg_bus[2]=Eep_Table[14];
			seg_bus[7]=beep_con;
		break;
		case 7:
			seg_bus[0]=21;
			seg_bus[1]=22;
			seg_bus[5]=(int)distances/100%10;
			seg_bus[6]=(int)distances/10%10;
			seg_bus[7]=(int)distances/1%10;
		break;
		case 8:
			seg_bus[0]=1;
			seg_bus[1]=Eep_Table[14];
			seg_bus[2]=Eep_Table[13];
			seg_bus[4]=23;
			seg_bus[5]=led_index+1;
			seg_bus[7]=led_bus[led_index];
		break;
		case 9:
			seg_bus[0]=word_index+1;
			seg_bus[2]=word_buf[word_index]/10;
			seg_bus[3]=word_buf[word_index]%10;
			seg_bus[4]=Eep_Table[word_buf[0]];
			seg_bus[5]=Eep_Table[word_buf[1]];
			seg_bus[6]=Eep_Table[word_buf[2]];
			seg_bus[7]=Eep_Table[word_buf[3]];
		break;
	}
}
void timer0_r() interrupt 3
{
	if(++seg_slow_down==50) seg_slow_down=0;
	if(++key_slow_down==100) key_slow_down=0;
	if(++seg_index==8) seg_index=0;
	seg(seg_index,seg_bus[seg_index], point_bus[seg_index]);
	led(seg_index,led_bus[seg_index]);
	led_proc();
	key_proc();
	seg_proc();
	if(seg_show_mod!=7)
	{
	if(++Timer_1000==1000)
	{
		Timer_1000=0;
		freq=(int)((TH0<<8)|TL0);
		TH0=TL0=0;
	}
	}
	other(relay_con,beep_con);
}
void main()
{
	EEPROM_Read(&eep_save,20,1);
	system_init();
	Timer1_Init();
	EEPROM_Read(word_buf,1,4);
	Timer0_Init();
	Time_write(Time);
	while(1)
	{
		V_RD1=AD_read(0x43);
		V_RB2=AD_read(0x41);
		temperatur=Temperatur_Read();
	}
}