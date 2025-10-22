#include <STC15F2K60S2.H>//start:16:50
#include <iic.h>

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
	0x8e,//F 11
	0xc1,//U 12
	0x88,//A
	0x83,//b
	0xc6,//C
	0xa1,//d
	0x86,//E
};
code unsigned char Seg_Wale[] ={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char seg_bus[]={10,10,10,10,10,10,10,10};
unsigned char point_bus[]={0,0,0,0,0,0,0,0,};
unsigned char led_bus[]={0,0,0,0,0,0,0,0,};
unsigned char found_index;
unsigned char seg_slow_down,key_slow_down,Vread_slow_down;
unsigned char key_val,key_old,key_down,key_up;
unsigned int Timer_1000;
unsigned int freq;
unsigned char V_RB2;
bit word_mod0_con=1;

unsigned char seg_show_mod=0; //0-频率 1-电压
bit word_mod;	//0-VRB2 1-2V
bit led_con=1;
bit seg_con=1;

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
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;
	EA = 1;
}
void system_init()
{
	P2=P2&0x1f|0x80; P0=0xff;
	P2=P2&0x1f|0xa0; P0=0x00;
	P2=P2&0x1f;
}

void seg(unsigned char wale,valu,point)
{
	P2=P2&0x1f|0xc0; P0=Seg_Wale[wale];
	P2=P2&0x1f|0xe0; P0=Seg_Table[10];
	
	P2=P2&0x1f|0xc0; P0=Seg_Wale[wale];
	P2=P2&0x1f|0xe0; P0=Seg_Table[10];
	if(point) 
		P0&=0x7f;
	P0&=Seg_Table[valu];
	P2=P2&0x1f;
}

void led(unsigned char addr,enable)
{
	unsigned char val=0x00,old=0xff;
	if(enable)
		val |= 0x01<<addr;
	else
		val &=~ (0x01<<addr);
	if(old!=val)
	{
		P2=P2&0x1f|0x80; 
		P0=~val;
		P2=P2&0x1f;
		old=val;
	}
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
void led_proc()
{
	unsigned char i;
	float v_n;
	for(i=0;i<8;i++) led_bus[i]=0;
	if(led_con==0) return;
	v_n=(float)(V_RB2/51.0);
	if(seg_show_mod==0) led_bus[1]=1;
	if(seg_show_mod==1) led_bus[0]=1;
	if((1.5<=v_n&&v_n<2.5)||(v_n>=3.5)) led_bus[2]=1;
	if((1000<=freq&&freq<5000)||(freq>=10000)) led_bus[3]=1;
	if(word_mod==0)   led_bus[4]=1;
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
			if(seg_show_mod==2) seg_show_mod=0;
		break;
		case 5:
			word_mod^=1;
			if(word_mod==0) 
				word_mod0_con=1;
			if(word_mod==1) 
			{
				word_mod0_con=0;
				V_RB2=255/5*2;
			}
		break;
		case 6:
			led_con^=1;
		break;
		case 7:
			seg_con^=1;
		break;
	}
}
void seg_proc()
{
	unsigned char i;
	float V_RB2_Show;
	if(seg_slow_down) return;
	seg_slow_down=1;
	for(i=0;i<8;i++)
	{
		point_bus[i]=0;
		seg_bus[i]=10;
	}
	if(seg_con==0) return;
	switch(seg_show_mod)
	{
		case 0:
			seg_bus[0]=11;
			if(freq>=0)
			{seg_bus[7]=freq%10;
				if(freq>=10)
				{seg_bus[6]=freq/10%10;
					if(freq>=100)
					{seg_bus[5]=freq/100%10;
						if(freq>=1000)
						{seg_bus[4]=freq/1000%10;
							if(freq>=10000)
							{seg_bus[3]=freq/10000%10;
								if(freq>=100000)
								seg_bus[2]=freq/100000%10;
							}
						}
					}
				}
			}
		break;
		case 1://V_RB2
			V_RB2_Show=(float)(V_RB2/51.0);
			seg_bus[0]=12;
			seg_bus[5]=(int)V_RB2_Show%10;
			point_bus[5]=1;
			seg_bus[6]=(int)(V_RB2_Show*10)%10;
			seg_bus[7]=(int)(V_RB2_Show*100)%10;
		break;
	}
	
}
void T1_R(void) interrupt 3
{
	if(++seg_slow_down==50) seg_slow_down=0;
	if(++key_slow_down==100) key_slow_down=0;
	if(++found_index==8) found_index=0;
	seg(found_index,seg_bus[found_index],point_bus[found_index]);
	led(found_index,led_bus[found_index]);
	key_proc();
	seg_proc();
	led_proc();
	if(++Timer_1000==1000)
	{
		Timer_1000=0;
		freq=(TH0<<8)|TL0;
		TH0=TL0=0;
	}
	if(word_mod0_con==1)
	{
	if(++Vread_slow_down==40)
		{
			Vread_slow_down=0;
			V_RB2=AD_Read(0x43);
		}
	}
}
void main()
{
	Timer1_Init();
	Timer0_Init();
	system_init();
	while(1)
	{
		
	}
}