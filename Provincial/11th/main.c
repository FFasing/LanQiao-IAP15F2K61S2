#include <STC15F2K60S2.H>
#include <onewire.h>
#include <iic.h>

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
0xc6,//C 11
0x8c,//P 12	
0x88,//A 13
0x83,//b

0xa1,//d
0x86,//E
0x8e//F
};
code unsigned char Seg_Wale[] ={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char seg_bus[]={10,10,10,10,10,10,10,10};
unsigned char point_bus[]={0,0,0,0,0,0,0,0};
unsigned char led_bus[]={0,0,0,0,0,0,0,0};
unsigned char seg_slow_down,key_slow_down;
unsigned char Index;
unsigned char key_val,key_old,key_down,key_up;
float Temperature,V_show_out;
unsigned char V_out;
float T_show;
unsigned char Timerature_Key_Set;

unsigned char seg_show_mod=0;	//0-温度 1-参数 2-DAC
unsigned char Timerature_Set=25;
bit word_mod=0;	//0-模式1 1-模式2

void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初值
	TH0 = 0xFC;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;
	EA  = 1;
}
void system_init()
{
	P2=P2&0x1f| 0x80;
	P0=0xff;
	P2=P2&0x1f| 0xa0;
	P0=0x00;
	P2=P2&0x1f;
}
void seg(unsigned char wale,valu,point)
{
	P2=P2&0x1f| 0xc0;
	P0=Seg_Wale[wale];
	P2=P2&0x1f| 0xe0;
	P0=Seg_Table[10];
	P2=P2&0x1f| 0xc0;
	P0=Seg_Wale[wale];
	P2=P2&0x1f| 0xe0;
	P0=Seg_Table[10];
	if(point) P0&=0x7f;
	P0&=Seg_Table[valu];
	P2=P2&0x1f;
}
unsigned char key_read()
{
	unsigned char temp=0;
	ROW1=ROW2=ROW3=ROW4=1;
	COL1=0;COL2=COL3=COL4=1;
	if(ROW4==0) temp=4;
	if(ROW3==0) temp=5;
	ROW1=ROW2=ROW3=ROW4=1;
	COL2=0;COL1=COL3=COL4=1;
	if(ROW4==0) temp=8;
	if(ROW3==0) temp=9;
	return temp;
}
void led(unsigned char addr,enable)
{
	unsigned char val=0x00,old=0xff;
	if(enable)
		val|=0x01<<addr;
	else
		val&=~(0x01<<addr);
	if(old!=val)
	{
		P2=P2&0x1f| 0x80;
		P0=~val;
		P2=P2&0x1f;
		old=val;
	}
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
			seg_show_mod++;
			if(seg_show_mod==3) seg_show_mod=0;
			if(seg_show_mod==1)	Timerature_Key_Set=Timerature_Set;
			if(seg_show_mod!=1) Timerature_Set=Timerature_Key_Set;
		break;
		case 5:
			word_mod^=1;
		break;
		case 8:
			if(seg_show_mod==1) Timerature_Key_Set--;
		break;
		case 9:
			if(seg_show_mod==1) Timerature_Key_Set++;
		break;
	}
}
void seg_proc()
{
	unsigned char i;
	if(seg_slow_down) return;
	seg_slow_down=1;
	for(i=0;i<8;i++) {seg_bus[i]=10;point_bus[i]=0;}
	T_show=Temperature;
	switch(seg_show_mod)
	{
		case 0://温度 Temperature
			
			seg_bus[0]=11;
			seg_bus[4]=(int)T_show/10;
			seg_bus[5]=(int)T_show%10;
			point_bus[5]=1;
			seg_bus[6]=(int)(T_show*10)%10;
			seg_bus[7]=(int)(T_show*100)%10;
		break;
		case 1://参数 Timerature_Set
			seg_bus[0]=12;
		
			seg_bus[6]=(int)Timerature_Key_Set/10;
			seg_bus[7]=(int)Timerature_Key_Set%10;
		break;
		case 2://DAC V_show_out 
			seg_bus[0]=13;
		
			seg_bus[4]=(int)V_show_out/10;
			seg_bus[5]=(int)V_show_out%10;
			point_bus[5]=1;
			seg_bus[6]=(int)(V_show_out*10)%10;
			seg_bus[7]=(int)(V_show_out*100)%10;
		break;
	}
}
void led_proc()
{
	unsigned char i;
	for(i=0;i<8;i++) led_bus[i]=0;
	if(word_mod==0) led_bus[0]=1;
	if(seg_show_mod==0) led_bus[1]=1;
	if(seg_show_mod==1) led_bus[2]=1;
	if(seg_show_mod==2) led_bus[3]=1;
}
void T0_R(void) interrupt 1
{
	if(++seg_slow_down==50) seg_slow_down=0;
	if(++key_slow_down==100) key_slow_down=0;
	if(++Index==8) Index=0;
	seg(Index,seg_bus[Index],point_bus[Index]);
	led(Index,led_bus[Index]);
	seg_proc();
	key_proc();
	led_proc();
}
void main()
{
	Timer0Init();
	system_init();
	while(1)
	{
		Temperature=T_read();
		if(word_mod==1)
		{
			V_show_out=(Temperature*3.0)/20.0-2;
			if(V_show_out<=1.0) {V_out=255/5;V_show_out=1.0;}
			if(V_show_out>=4.0) {V_out=255/5*4;V_show_out=4.0;}
		}
		if(word_mod==0)
		{
			if(Timerature_Set>Temperature)
			{
				V_out=0;
				V_show_out=0.0;
			}
			else
			{
				V_out=255;
				V_show_out=5.0;
			}
		}
		DA_Write(V_out);
	}
}