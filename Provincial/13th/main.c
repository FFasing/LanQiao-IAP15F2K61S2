#include <STC15F2K60S2.H>
#include <onewire.h>
#include <ds1302.h>

sbit ROW1 = P3^0;
sbit ROW2 = P3^1;
sbit ROW3 = P3^2;
sbit ROW4 = P3^3;
sbit COL1 = P4^4;
sbit COL2 = P4^2;
sbit COL3 = P3^5;
sbit COL4 = P3^4;

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
0xc1,//11 U
0xbf,//12 -
0x88,//A
0x83,//b
0xc6,//C
0xa1,//d
0x86,//E
0x8e//F
};
code unsigned char Seg_Wale[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char seg_show_mod=0;		//0-温度 1-时间h/m 2-参数 3-m/s
unsigned char Time[3]={0x00,0x59,0x58};	//time
unsigned int T_compare=23;				//温度参数
bit work_mod=0;						//0-温度控制 1-时间控制
unsigned char point_bus[]={0,0,0,0,0,0,0,0};
unsigned char led_bus[]={0,0,0,0,0,0,0,0};
unsigned char seg_bus[]={10,10,10,10,10,10,10,10};
unsigned char seg_index,seg_slow_down,key_slow_down;
unsigned char key_val,key_old,key_up,key_down;
float Temperature;
float T_show;
unsigned int Timer_5000,Timer_50;
bit Time_5s_con,Time_1ms_con;
bit Relay_con=0;
unsigned char h_old,h_val;

void Relay(unsigned char enable)
{
	unsigned char temp=0x00,temp_old=0xff;
	if(enable)
		temp |= 0x10;
	else
		temp &=~ 0x10;
	if(temp!=temp_old)
	{
	P2 = P2 & 0x1f |0xa0;
	P0 = temp;
	P2 = P2 & 0x1f;
	temp_old=temp;
	}
}
void led(unsigned char addr,enable)
{
	unsigned char  temp=0x00,temp_old=0xff;
	if(enable)
		temp |= 0x01<<addr;
	else
		temp &=~(0x01<<addr);
	if(temp!=temp_old)
	{
	P2 = P2 & 0x1f |0x80;
	P0 =~temp;
	P2 = P2 & 0x1f;
	temp_old=temp;
	}
}
void led_proc()
{
	if(h_val!=h_old)
	{
		Time_5s_con=1;
		h_old=h_val;
	}
	else if(work_mod==0)
	{
		Relay_con=0;
		if(T_compare<T_show)
			Relay_con=1;
	}
	
	if(work_mod==0) led_bus[1]=1;
	else led_bus[1]=0;
	
	if(Relay_con==1) Time_1ms_con=1;
	else 
		{
			Time_1ms_con=0;
			led_bus[2]=0;
		}
}
void system_init()
{
	P2 = P2 & 0x1f |0x80;
	P0 = 0xff;
	P2 = P2 & 0x1f |0xa0;
	P0 = 0x00;
	P2 = P2 & 0x1f;
}
void seg(unsigned char wale,valu,point)
{
	P2 = P2 & 0x1f |0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f |0xe0;
	P0 = Seg_Table[10];
	
	P2 = P2 & 0x1f |0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f |0xe0;
	P0 = 0xff;
	if(point) 
		P0 &= 0x7f;
	P0 &= Seg_Table[valu];
	P2 = P2 & 0x1f;
}
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初值
	TH0 = 0xFC;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;
	EA = 1;
}
unsigned char key_read()
{
	unsigned char temp=0;
	ROW1=ROW2=ROW3=ROW4=1;
	COL3=0;COL1=COL2=COL4=1;
	if(ROW4==0) temp=12;
	if(ROW3==0) temp=13;
	ROW1=ROW2=ROW3=ROW4=1;
	COL4=0;COL1=COL2=COL3=1;
	if(ROW4==0) temp=16;
	if(ROW3==0) temp=17;
	return temp;
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
		case 12:
			seg_show_mod++;
			if(seg_show_mod==3) seg_show_mod=0;
		break;
		case 13:
			work_mod^=1;
		break;
		case 16:
			if(seg_show_mod==2)
			{
				T_compare++;
			}
		break;
		case 17:
			if(seg_show_mod==2)
			{
				T_compare--;
			}
			if(seg_show_mod==1)
			{
				seg_show_mod=3;
			}
		break;
	}
	if(seg_show_mod==3&&key_up==17) seg_show_mod=1;
}
void seg_proc()
{
	unsigned char i;
	if(seg_slow_down) return;
	seg_slow_down=1;
	
	T_show=Temperature;
	Rtc_Read(Time);
	h_val=Time[0];
	for(i=0;i<8;i++) 
		{
		point_bus[i]=0;
		seg_bus[i]=10;
		}
		switch(seg_show_mod)
	{
		case 0://温度
			
			seg_bus[0]=11;
			seg_bus[1]=1;
			seg_bus[5]=(unsigned char)T_show/10%10;
			seg_bus[6]=(unsigned char)T_show%10;
			point_bus[6]=1;
			seg_bus[7]=(unsigned char)(T_show*10)%10;
		break;
		case 1://时间
			seg_bus[0]=11;
			seg_bus[1]=2;
			seg_bus[3]=(int)Time[0]/16;
			seg_bus[4]=(int)Time[0]%16;
			seg_bus[5]=12;
			seg_bus[6]=(int)Time[1]/16;
			seg_bus[7]=(int)Time[1]%16;
		break;
		case 2://参数
			seg_bus[0]=11;
			seg_bus[1]=3;
			seg_bus[6]=(unsigned char)T_compare/10%10;
			seg_bus[7]=(unsigned char)T_compare%10;
		break;
		case 3://时间
			seg_bus[0]=11;
			seg_bus[1]=2;
			seg_bus[3]=(int)Time[1]/16;
			seg_bus[4]=(int)Time[1]%16;
			seg_bus[5]=12;
			seg_bus[6]=(int)Time[2]/16;
			seg_bus[7]=(int)Time[2]%16;
		break;
	}
	
}
void T0_R(void) interrupt 1
{
	if(++seg_slow_down==50) seg_slow_down=0;
	if(++key_slow_down==100) key_slow_down=0;
	if(++seg_index==8) seg_index=0;
	seg(seg_index,seg_bus[seg_index],point_bus[seg_index]);
	led(seg_index,led_bus[seg_index]);
	seg_proc();
	key_proc();
	if(Time_5s_con==1)
	{
		led_bus[0]=1;
		if(work_mod==1) Relay_con=1;
		if(++Timer_5000==5000)
		{
			Timer_5000=0;
			Time_5s_con=0;
			if(work_mod==1) Relay_con=0;
			led_bus[0]=0;
		}
	}
	if(Time_1ms_con==1)
	{
		if(++Timer_50==50)
		{
			Timer_50=0;
			led_bus[2]^=1;
		}
	}
	led_proc();
	Relay(Relay_con);
}
void main()
{
	system_init();
	Timer0Init();
	Rtc_Write(Time);
	h_val=Time[3];
	h_old=h_val;
	Relay_con=0;
	while(1)
	{
		Temperature=T_read();
	}
}
