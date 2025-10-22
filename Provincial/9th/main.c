#include <STC15F2K60S2.H>
#include "intrins.h"
#include "iic.h"
sbit US_TX=P1^0;
sbit US_RX=P1^1;
code unsigned char Seg_Wale[] ={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
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
0x8e,//F 12
0x88,//A
0x83,//b
0xa1,//d
0x8,//E
};
unsigned char seg_bus[]={10,10,10,10,10,10,10,10};
unsigned char point_bus[]={0,0,0,0,0,0,0,0};
unsigned char led_bus[]={0,0,0,0,0,0,0,0};
unsigned char found_index;
unsigned char seg_slow_down,key_slow_down;
unsigned char key_val,key_old,key_down,key_up;
unsigned char Distance_Found;
unsigned char Distance_Save[4]={0,0,0,0};
unsigned int blink_num,Timer_1000;
bit found_over_flag=0;

unsigned char V_out;
unsigned char Found_Num=0;
unsigned char Mod2_num,Mod3_set;
unsigned char seg_show_mod=0; //0-测距 1-回显 2-参数
unsigned char Distance_Set=20;

void Delay20us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 57;
	while (--i);
}
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TL0 = 0x18;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;
	EA = 1;
}
void system_init()
{
	P2 = P2 & 0x1f | 0x80;
	P0 = 0xff;
	P2 = P2 & 0x1f | 0xa0;
	P0 = 0x00;
	P2 = P2 & 0x1f;
}
void seg(unsigned char wale,valu,point)
{
	P2 = P2 & 0x1f | 0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f | 0xe0;
	P0 = Seg_Table[10];
	P2 = P2 & 0x1f | 0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f | 0xe0;
	P0 = Seg_Table[10];
	if(point) P0&= 0x7f;
	P0&= Seg_Table[valu];
	P2 = P2 & 0x1f;
}
unsigned char key_read()
{
	unsigned char temp=0;
	if(P33==0) temp=4;
	if(P32==0) temp=5;
	if(P31==0) temp=6;
	if(P30==0) temp=7;
	return temp;
}
void led(unsigned char addr,enable)
{
	unsigned char temp=0x00,temp_old=0xff;
	if(enable)
		temp|=0x01<<addr;
	else
		temp&=~(0x01<<addr);
	if(temp_old!=temp)
	{
		P2 = P2 & 0x1f | 0x80;
		P0 =~temp;
		P2 = P2 & 0x1f;
		temp_old=temp;
	}
}
void Ultrasonic_Wave()
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		US_TX=1;
		Delay20us();
		US_TX=0;
		Delay20us();
	}
}
unsigned char Us_Found()
{
	TMOD &= 0x0f;
	TH1=TL1=0;
	Ultrasonic_Wave();
	TR1=1;
	while((US_RX==1)&&(TF1==0));
	TR1=0;
	if(TF1==1)
	{
		TF1=0;
		return 0;
	}
	else
	{
		return ((TH1<<8)|TL1)*0.017;
	}
}
void Us_Text_One()
{
	Distance_Found=0;
	Distance_Found=Us_Found();
	Distance_Save[Found_Num]=Distance_Found;
	Found_Num++;
	if(Found_Num==4) Found_Num=0;
	eeprom_write(Distance_Save,1,4);
	found_over_flag=1;
	blink_num=0;
}
void DA_conversion()
{
	if(Distance_Found<=Distance_Set)
	{
		DA_Write(0);
	}
	else
	{
		DA_Write((int)(Distance_Found-Distance_Set));
	}
}
void led_proc()
{
	unsigned char i;
	for(i=1;i<8;i++) led_bus[i]=0;
	if(seg_show_mod==2) led_bus[6]=1;
	if(seg_show_mod==1) led_bus[7]=1;
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
			if(seg_show_mod==0) Us_Text_One();
		break;
		case 5:
			if(seg_show_mod!=1) seg_show_mod=1;
			else if(seg_show_mod==1) seg_show_mod=0;
		break;
		case 6:
			if(seg_show_mod!=2) 
			{
				Mod3_set=Distance_Set;
				seg_show_mod=2;
			}
			else if(seg_show_mod==2) 
			{
				Distance_Set=Mod3_set;
				eeprom_write(&Distance_Set,20,1);
				seg_show_mod=0;
			}
		break;
		case 7:
			if(seg_show_mod==1) Mod2_num++;
			if(seg_show_mod==2) Mod3_set+=10;
			if(Mod2_num==4) Mod2_num=0;
			if(Mod3_set==40) Mod3_set=0;
		break;
	}
	
}
void seg_proc()
{
	unsigned char i;
	if(seg_slow_down) return;
	seg_slow_down=1;
	for(i=0;i<8;i++)
	{
		seg_bus[i]=10;
		point_bus[i]=0;
	}
	switch(seg_show_mod)
	{
		case 0://测距
			seg_bus[0]=11;
			
			seg_bus[2]=(int)Distance_Found/100%10;
			seg_bus[3]=(int)Distance_Found/10%10;
			seg_bus[4]=(int)Distance_Found%10;
			if(Found_Num>1)
			{
			seg_bus[5]=(int)Distance_Save[Found_Num-2]/100%10;
			seg_bus[6]=(int)Distance_Save[Found_Num-2]/10%10;
			seg_bus[7]=(int)Distance_Save[Found_Num-2]%10;
			}
			else
			{
			seg_bus[5]=(int)Distance_Save[(3*Found_Num+4)/2]/100%10;
			seg_bus[6]=(int)Distance_Save[(3*Found_Num+4)/2]/10%10;
			seg_bus[7]=(int)Distance_Save[(3*Found_Num+4)/2]%10;
			}
		break;
		case 1://回显 Distance_Save[Mod2_num] 
			seg_bus[0]=2;
		
			seg_bus[5]=(int)Distance_Save[Mod2_num]/100%10;
			seg_bus[6]=(int)Distance_Save[Mod2_num]/10%10;
			seg_bus[7]=(int)Distance_Save[Mod2_num]%10;
		break;
		case 2://参数 Mod3_set
			seg_bus[0]=12;
			seg_bus[6]=(int)Mod3_set/10%10;
			seg_bus[7]=(int)Mod3_set%10;
		break;
	}
}
void T0_R(void) interrupt 1
{
	if(++seg_slow_down==50) seg_slow_down=0;
	if(++key_slow_down==90) key_slow_down=0;
	if(++found_index==8) found_index=0;
	led(found_index,led_bus[found_index]);
	seg(found_index,seg_bus[found_index],point_bus[found_index]);
	seg_proc();
	key_proc();
	led_proc();
	if(found_over_flag==1)
	{
		if(blink_num!=6)
		{
			if(++Timer_1000==1000)
			{
				Timer_1000=0;
				led_bus[0]^=1;
				blink_num++;
			}
		}
		else
		{
			led_bus[0]=0;
			found_over_flag=0;
			blink_num=0;
		}
	}
}
void main()
{
	Timer0_Init();
	system_init();
	eeprom_read(Distance_Save,1,4);
	eeprom_read(&Distance_Set,20,1);
//	eeprom_write(Distance_Save,1,4);
//	eeprom_write(&Distance_Set,20,1);
	while(1)
	{
		DA_conversion();
	}
}