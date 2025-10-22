#include <STC15F2K60S2.H>//2h15min
#include <intrins.h>
#include <iic.h>

sbit US_TX=P1^0;
sbit US_RX=P1^1;

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
	0xff//16
};
code unsigned char Seg_Wale[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char seg_slow_down,key_slow_down;
unsigned char seg_buf[]={16,16,16,16,16,16,16,16};
unsigned char seg_index;
unsigned char key_val,key_old,key_up,key_down;
unsigned char led_buf[]={0,0,0,0,0,0,0,0};
unsigned char V_RB2=0;//50-0
bit V_RB2_CT_500,C_Transmit;
unsigned int Time_1000,Time_500,transmit_time,ad_wait;
unsigned char distance;
unsigned char freight=1;
unsigned char mod3_count;
bit seg_flash_flag;

bit freight_flag=0;
bit urgent_stop=0;
unsigned char work_mod=0;//0-空载过载 1-类型判断 2-货物传送
unsigned char RELAY=0,BEEP=0;
unsigned char transmit_time_set[2]={2,4};

void Delay20us()		//@20.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 97;
	while (--i);
}
void system_init()
{
	P2 = P2 & 0x1f | 0x80;//led
	P0 = 0xff;
	P2 = P2 & 0x1f | 0xa0;//other
	P0 = 0x00;
	P2 = P2 & 0x1f;
}
void Timer1Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;		//定时器时钟12T模式
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0x18;		//设置定时初值
	TH1 = 0xFC;		//设置定时初值
	TF1 = 0;		//清除TF1标志
	TR1 = 1;		//定时器1开始计时
	ET1 = 1;
	EA =1;
}
void seg(unsigned char wale,valu)
{
	P2 = P2 & 0x1f | 0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f | 0xe0;
	P0 = Seg_Table[16];
	
	P2 = P2 & 0x1f | 0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f | 0xe0;
	P0 = Seg_Table[valu];
	P2 = P2 & 0x1f;
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
		P2 = P2 & 0x1f | 0x80;//led
		P0 =~val;
		P2 = P2 & 0x1f;
		old=val;
	}
}
void relay_beep(unsigned char relay_enable,beep_enable)
{
	unsigned char val=0x00,old=0xff;
	if(beep_enable)
		val|=0x40;
	else
		val&=~0x40;
	
	if(relay_enable)
		val|=0x10;
	else
		val&=~0x10;
	
	if(old!=val)
	{
		P2 = P2 & 0x1f | 0xa0;//other
		P0 = val;
		P2 = P2 & 0x1f;
		old=val;
	}
}
unsigned char key_read()
{
	unsigned char temp=0;
	ROW1=ROW2=ROW3=ROW4=1;
	COL1=0;COL2=COL3=COL4=1;
	if(ROW4==0) temp=4;
	if(ROW3==0) temp=5;
	if(ROW2==0) temp=6;
	if(ROW1==0) temp=7;
	return temp;
}
void us_wale()
{
	unsigned char i;
	for(i=0;i<10;i++)
	{
		US_TX=1;Delay20us();
		US_TX=0;
	}
}
unsigned char us_read()
{
	TMOD&=0XF0;
	TH0=TL0=0;
	us_wale();
	TR0=1;
	while(US_RX==1&&TF0==0);
	TR0=0;
	if(TF0)
	{
		TF0=0;
		return 0;
	}
	else
		return ((TH0<<8)|TL0)*0.017;
}
void led_proc()
{
	
	if(V_RB2>=0&&V_RB2<10) 	//空载
	{
		freight_flag=0;
		led_buf[0]=1;
	}
	else led_buf[0]=0;
	
	if(V_RB2>=10&&V_RB2<40) 	//非空载
	{
		led_buf[1]=1;
		if(freight_flag==0&&work_mod==0||freight_flag==0&&work_mod==1) freight_flag=1;
	}
	else led_buf[1]=0;
		
	if(V_RB2>=40) 				//过载
	{
		freight_flag=0;
		V_RB2_CT_500=1;
	}
	else V_RB2_CT_500=0;
	
	
	if(freight_flag==1)
	{
		work_mod=1;
		freight_flag=0;
	}
}
void key_proc()
{
	if(key_slow_down) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	switch(key_down)
	{
		case 4:
			if(V_RB2>=10&&V_RB2<40) 	//非空载
			{
				work_mod=2;
				C_Transmit=1;
				transmit_time=0;
			}
		break;
		case 5:
			if(C_Transmit==1) 	//运输中
			{
				urgent_stop^=1;
			}
		break;
		case 6:
			if(V_RB2>=0&&V_RB2<10) 	mod3_count++;//空载
			if(mod3_count==1) work_mod=3;
			if(mod3_count==4) 
			{
				work_mod=0;
				mod3_count=0;
			}
		break;
		case 7:
			if(V_RB2>=0&&V_RB2<10) 	//空载
			{
				if(mod3_count==2)	transmit_time_set[0]++;
				if(mod3_count==3)	transmit_time_set[1]++;
				if(transmit_time_set[0]==11) transmit_time_set[0]=1;
				if(transmit_time_set[1]==11) transmit_time_set[1]=1;
				eeprom_write(transmit_time_set,1,2);
			}
		break;
	}
}
void seg_proc()
{
	unsigned char i;
	if(seg_slow_down) return;
	seg_slow_down=0;
	for(i=0;i<8;i++) seg_buf[i]=16;
	switch(work_mod)//0-空载过载 1-类型判断 2-货物传送 3-调整时间
	{
		case 0:
			
		break;
		case 1:
			seg_buf[0]=1;
			seg_buf[3]=distance/10%10;
			seg_buf[4]=distance%10;
			seg_buf[7]=freight;
		break;
		case 2:
			seg_buf[0]=2;
		 
			if(freight==1)
			{
			seg_buf[6]=((int)transmit_time_set[0]*1000-transmit_time)/10000%10;
			seg_buf[7]=((int)transmit_time_set[0]*1000-transmit_time)/1000%10;
			}
			if(freight==2)
			{
			seg_buf[6]=((int)transmit_time_set[1]*1000-transmit_time)/10000%10;
			seg_buf[7]=((int)transmit_time_set[1]*1000-transmit_time)/1000%10;
			}
		break;
		case 3:
			seg_buf[0]=3;
			seg_buf[3]=(int)transmit_time_set[0]/10%10;
			seg_buf[4]=(int)transmit_time_set[0]%10;
		
			if(mod3_count==2) 
			{
				seg_flash_flag?(seg_buf[3]=(int)transmit_time_set[0]/10%10):(seg_buf[3]=16);
				seg_flash_flag?(seg_buf[4]=(int)transmit_time_set[0]%10):(seg_buf[4]=16);
			}
			else
			{
				seg_buf[3]=(int)transmit_time_set[0]/10%10;
				seg_buf[4]=(int)transmit_time_set[0]%10;
			}
			
			if(mod3_count==3) 
			{
				seg_flash_flag?(seg_buf[6]=(int)transmit_time_set[1]/10%10):(seg_buf[6]=16);
				seg_flash_flag?(seg_buf[7]=(int)transmit_time_set[1]%10):(seg_buf[7]=16);
			}
			else
			{
				seg_buf[6]=(int)transmit_time_set[1]/10%10;
				seg_buf[7]=(int)transmit_time_set[1]%10;
			}
		break;
	}
}
void T1_R(void) interrupt 3
{
	if(++Time_1000==1000) Time_1000=0;
	if(++ad_wait==200) ad_wait=0;
	if(++seg_slow_down==50) seg_slow_down=0;
	if(++key_slow_down==100) key_slow_down=0;
	if(++seg_index==8) seg_index=0;
	seg(seg_index,seg_buf[seg_index]);
	led(seg_index,led_buf[seg_index]);
	relay_beep(RELAY,BEEP);
	key_proc();seg_proc();led_proc();
	if(V_RB2_CT_500)
	{
		BEEP=1;
		if(++Time_500==500)
		{
			Time_500=0;
			led_buf[2]^=1;
		}
	}
	else{led_buf[2]=0;BEEP=0;}
	
	if(mod3_count==2||mod3_count==3)
	{
		if(++Time_500==500)
		{
			Time_500=0;
			seg_flash_flag^=1;
		}
	}
	
	if(C_Transmit==1&&V_RB2>=10&&V_RB2<40)
	{
		if(urgent_stop==0)
		{
			led_buf[3]=0;
			RELAY=1;
			
			if(freight==1)
			if(++transmit_time==(int)transmit_time_set[0]*1000)
			{
				freight_flag=0;
				C_Transmit=0;
			}
			
			if(freight==2)
			if(++transmit_time==(int)transmit_time_set[1]*1000)
			{
				freight_flag=0;
				C_Transmit=0;
			}
		}
		else
		{
			RELAY=0;
			if(++Time_500==500)
			{
				Time_500=0;
				led_buf[3]^=1;
			}
		}
	}
	else{RELAY=0;C_Transmit=0;led_buf[3]=0;urgent_stop=0;
		 if(freight==1)transmit_time=(int)transmit_time_set[0]*1000;
  else if(freight==2)transmit_time=(int)transmit_time_set[1]*1000;}
}
void main()
{
	system_init();
	Timer1Init();
	//eeprom_write(transmit_time_set,1,2);
	eeprom_read(transmit_time_set,1,2);
	V_RB2=ad_read(0x43)*10/51;
	V_RB2=1;
	while(1)
	{
		if(ad_wait==0) V_RB2=ad_read(0x43)*10/51;
		if(work_mod==1&&Time_1000==0)
		{
			distance=us_read();
			if(distance<=30) freight=1;
			if(distance>30) freight=2;
		}
	}
}