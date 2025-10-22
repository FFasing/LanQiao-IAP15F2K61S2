#include <STC15F2K60S2.H>
#include <iic.h>
#include <onewire.h>
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
	0xc1,//U 17
	0x89,//H 18
	0x8c//P 19
};
code unsigned char Seg_Wale[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 };
unsigned char index,seg_bus[]={16,16,16,16,16,16,16,16},point_bus[]={0,0,0,0,0,0,0,0};
unsigned char dat_slow,seg_slow_down,key_slow_down,led_bus[]={0,0,0,0,0,0,0,0};
unsigned char key_val,key_old,key_down,key_up;
unsigned int freq;
unsigned int v_rb2,v_set;
unsigned int Timer_1000,Timer_800,Timer_200;
float temperature;
unsigned char flag_long=0;

unsigned char word_mod=0;		//0-数据显示 1-数据回显 2-阈值
unsigned char mod1_select=0;	//0-电压 1-频率 2-温度
unsigned char mod2_select=0;	//0-电压 1-频率 2-温度
unsigned char eeprom_save[6];	//0-电压 1\2-频率 3\4-温度
void Delay20ms(void)	//@12.000MHz
{
	unsigned char data i, j;

	i = 234;
	j = 115;
	do
	{
		while (--j);
	} while (--i);
}
void Timer1_Init(void)		//1000微秒@12.000MHz
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
void Timer0_Init(void)		//1000微秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
}
void system_init(void)
{
	eeprom_read(eeprom_save,1,6);
	Delay20ms();
	Timer1_Init();
	Timer0_Init();
	P2=P2&0x1f|0x80;
	P0=0xff;
	P2=P2&0x1f|0xa0;
	P0=0x00;
	P2=P2&0x1f;
}
void seg(unsigned char wale,valu,point)
{
	P2=P2&0x1f|0xc0;
	P0=Seg_Wale[wale];
	P2=P2&0x1f|0xe0;
	P0=Seg_Table[16];
	P2=P2&0x1f|0xc0;
	P0=Seg_Wale[wale];
	P2=P2&0x1f|0xe0;
	P0=Seg_Table[valu];
	if(point) P0&=0x7f;
	P2=P2&0x1f;
}
unsigned char key(void)
{
	unsigned char i=0;
	ROW1=ROW2=ROW3=ROW4=1;
	COL1=0;COL2=COL3=COL4=1;
	if(ROW4==0) i=4;
	if(ROW3==0) i=5;
	if(ROW2==0) i=6;
	if(ROW1==0) i=7;
	return i;
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
		P2=P2&0x1f|0x80;
		P0=~val;
		P2=P2&0x1f;
		old=val;
	}
}
void dat_proc(void)
{
	unsigned char V_RB2;
	if(dat_slow) return;
	dat_slow=1;
	
	V_RB2=ad_read(0x43);
	v_rb2=V_RB2;
	v_rb2=v_rb2*50/255;
	
	temperature=Temperature_Read();
	temperature*=100;
}
void led_proc(void)
{
	unsigned char i;
	for(i=0;i<3;i++) led_bus[i]=0;
	if(word_mod==0)
	{
		switch(mod1_select)
		{
			case 2:
				led_bus[0]=1;
			break;
			case 1:
				led_bus[1]=1;
			break;
			case 0:
				led_bus[2]=1;
			break;
		}
	}
}
void key_proc(void)
{
	unsigned char eeprom;
	if(key_slow_down) return;
	key_slow_down=1;
	key_val=key();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	switch(key_down)
	{
		case 4:
			if(word_mod==0) mod1_select++;
			if(word_mod==1) mod2_select++;
			if(mod1_select>=3) mod1_select=0;
			if(mod2_select>=3) mod2_select=0;
		break;
		case 5:
			if(word_mod==0)
			{
				switch(mod1_select)
				{
					case 0:
						eeprom=v_rb2;
						eeprom_save[0]=eeprom;
					break;
					case 1:
						eeprom=(freq/10000%10);
						eeprom_save[5]=eeprom;
						eeprom=0;
						eeprom+=(freq/1000%10)*10;
						eeprom+=(freq/100%10);
						eeprom_save[1]=eeprom;
						eeprom=0;
						eeprom+=(freq/10%10)*10;
						eeprom+=(freq%10);
						eeprom_save[2]=eeprom;
					break;
					case 2:
						eeprom=0;
						eeprom+=((int)temperature/1000%10)*10;
						eeprom+=((int)temperature/100%10);
						eeprom_save[3]=eeprom;
						eeprom=0;
						eeprom+=((int)temperature/10%10)*10;
						eeprom+=((int)temperature%10);
						eeprom_save[4]=eeprom;
					break;
				}
				eeprom_write(eeprom_save,1,6);
				Delay20ms();
			}
		break;
		case 6:
			if(word_mod==2) 
			{
				flag_long=1;
				v_set++;
			}
			else word_mod=1;
		break;
		case 7:
			if(word_mod!=2)
			word_mod=2;
			else 
			word_mod=0;
		break;
	}
	if(key_up==6&&word_mod==2) 
	{
		Timer_800=0;
		flag_long=0;
	}
}
void seg_proc(void)
{
	unsigned char eeprom;
	unsigned char i;
	if(seg_slow_down) return;
	seg_slow_down=1;
	for(i=0;i<8;i++)
	{
		seg_bus[i]=16;
		point_bus[i]=0;
	}
	if(word_mod==0)switch(mod1_select)
	{
		case 0:
			seg_bus[0]=17;
			seg_bus[6]=v_rb2/10%10;
			point_bus[6]=1;
			seg_bus[7]=v_rb2%10;
		break;
		case 1:
			seg_bus[0]=15;
			seg_bus[3]=freq/10000%10;
			seg_bus[4]=freq/1000%10;
			seg_bus[5]=freq/100%10;
			seg_bus[6]=freq/10%10;
			seg_bus[7]=freq%10;
		break;
		case 2:
			seg_bus[0]=12;
			seg_bus[4]=(int)temperature/1000%10;
			seg_bus[5]=(int)temperature/100%10;
			point_bus[5]=1;
			seg_bus[6]=(int)temperature/10%10;
			seg_bus[7]=(int)temperature%10;
		break;
	}
	if(word_mod==1)
	{
		seg_bus[0]=18;
		switch(mod2_select)
		{
		case 0:
			seg_bus[1]=17;
			point_bus[6]=1;
			eeprom=eeprom_save[0];
			seg_bus[6]=eeprom/10%10;
			seg_bus[7]=eeprom%10;
		break;
		case 1:
			seg_bus[1]=15;
			eeprom=eeprom_save[5];
			seg_bus[3]=eeprom%10;
			eeprom=eeprom_save[1];
			seg_bus[4]=eeprom/10%10;
			seg_bus[5]=eeprom%10;
			eeprom=eeprom_save[2];
			seg_bus[6]=eeprom/10%10;
			seg_bus[7]=eeprom%10;
		break;
		case 2:
			seg_bus[1]=12;
			point_bus[5]=1;
			eeprom=eeprom_save[3];
			seg_bus[4]=eeprom/10%10;
			seg_bus[5]=eeprom%10;
			eeprom=eeprom_save[4];
			seg_bus[6]=eeprom/10%10;
			seg_bus[7]=eeprom%10;
		break;
		}
	}
	if(word_mod==2)
	{
		seg_bus[0]=19;
		seg_bus[6]=v_set/10%10;
		point_bus[6]=1;
		seg_bus[7]=v_set%10;
	}
}
void Timer1_server(void) interrupt 3
{
	if(++dat_slow==500) dat_slow=0;
	if(++seg_slow_down==50) seg_slow_down=0;
	if(++key_slow_down==100) key_slow_down=0;
	if(++index==8) index=0;
	seg(index,seg_bus[index],point_bus[index]);
	led(index,led_bus[index]);
	key_proc();seg_proc();led_proc();
	if(++Timer_1000==1000)
	{
		Timer_1000=0;
		TR0=0;
		freq=(TH0<<8)|TL0;
		TL0=TH0=0;
		TR0=1;
	}
	if(flag_long)
	{
		if(++Timer_800==800)
		{
			if(flag_long==2) v_set++;
			Timer_800=0;
			flag_long=2;
		}
	}
	if(v_set>50) v_set=1;
	if(v_set<v_rb2)
	{
		if(++Timer_200==200)
		{
			Timer_200=0;
			led_bus[7]^=1;
		}
	}
	else led_bus[7]=0;
}
void main()
{
	system_init();
	while(1)
	{
		dat_proc();
	}
}