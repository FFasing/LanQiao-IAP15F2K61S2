#include <STC15F2K60S2.H>
#include <iic.h>
#include <intrins.h>

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
	0xff,//  16
	0x89,//H 17
	0x8c//P 18
};
code unsigned char Seg_Wale[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char all_slow,index,seg_slow,key_slow,led_slow,dat_slow;
unsigned char led_buf[]={0,0,0,0,0,0,0,0},
point_buf[]={0,0,0,0,0,0,0,0},
seg_buf[]={16,16,16,16,16,16,16,16};
unsigned char key_v,key_o,key_d,key_u;
unsigned int Timer_1000,Timer_1s,Timer_100ms;
bit T1s_con;

bit freq_con=0;
bit relay_con;
unsigned char 	relay_num;

unsigned int 	freq;
unsigned char 	wetness;
unsigned int 	destance=45;
unsigned int 	freq_set=9000,
					wetness_set=40,
					destance_set=60;

unsigned char 	seg_mod;	//0-freq 1-wetness 2-destance 3-set
unsigned char 	m0_set=0;		//0-HZ 1-KHZ
unsigned char	m2_set=0;		//0-CM 1-M
unsigned char 	m3_set=0;		//0-freq_set 1-wetness_set 2-destance_set

unsigned int PWM_count,PWM_D;
bit motor_con;
void Timer2_Init(void)		//1000微秒@12.000MHz
{
	AUXR &= 0xFB;			//定时器时钟12T模式
	T2L = 0x00;				//设置定时初始值
	T2H = 0x00;				//设置定时初始值
	AUXR |= 0x10;			//定时器2开始计时
}
void Timer1_Init(void)		//100微秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0x00;			//设置定时器模式
	TMOD |= 0X05;		
	TL1 = 0x9C;				//设置定时初始值
	TH1 = 0xFF;				//设置定时初始值
	TF1 = 0;				//清除TF0标志
	TR1 = 1;				//定时器0开始计时
	ET1 = 1;
	EA = 1;
}
void seg(unsigned char wale,valu,point)
{
	P2=P2&0X1F|0XC0;
	P0=Seg_Wale[wale];
	P2=P2&0X1F|0XE0;
	P0=Seg_Table[16];
	
	P2=P2&0X1F|0XC0;
	P0=Seg_Wale[wale];
	P2=P2&0X1F|0XE0;
	P0=Seg_Table[valu];
	if(point) P0&=0x7f;
}
void key_read()
{
	unsigned char i=0;
	ROW1=ROW2=ROW3=ROW4=1;
	COL1=0;COL2=COL3=COL4=1;
	if(ROW4==0) i=4;
	if(ROW3==0) i=5;
	if(ROW2==0) i=6;
	if(ROW1==0) i=7;
	key_v=i;
	key_d=key_v&(key_v^key_o);
	key_u=~key_v&(key_v^key_o);
	key_o=key_v;
}
void led(unsigned char addr,enable)
{
	unsigned char v=0x00,o=0xff;
	if(enable)
		v|=0x01<<addr;
	else 
		v&=~(0x01<<addr);
	if(o!=v)
	{
		P2=P2&0X1F|0X80;
		P0=~v;
		o=v;
	}
}
void others(unsigned char relay_en,motor_en)
{
	unsigned char v=0x00,o=0xff;
	if(relay_en)
		v|=0x10;
	else 
		v&=~0x10;
	if(motor_en)
		v|=0x20;
	else 
		v&=~0x20;
	if(o!=v)
	{
		P2=P2&0X1F|0XA0;
		P0=v;
		o=v;
	}
}
void freq_read()
{
		if(++Timer_1000==1000)
		{
			Timer_1000=0;
			TR0=0;
			freq=(TH0<<8)|TL0;
			TH0=TL0=0;
			TR0=1;
		}
}

void Delay20us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 57;
	while (--i);
}
void us_wave()
{
	unsigned char i;
	for(i=0;i<10;i++)
	{
		US_TX=1;Delay20us();US_TX=0;
	}
}

void destance_read()
{
	unsigned char temp,TF2=0;
	AUXR&=0xef;//1110 1111
	T2H=T2L=0;
	us_wave();
	AUXR|=0x10;//0001 0000
	while(US_RX==1&&TF2==0)
	{
		if(T2H==0xff&&T2L==0xff) 
		{
			T2H=T2L=0;
			TF2=1;
		}
	}
	AUXR&=0xef;//1110 1111
	temp=((T2H<<8)|T2L)*0.017;
	T2H=T2L=0;  
	destance = (char)temp;
}
void dat_proc()
{
	float v_r,v_o,set;
	unsigned char V_out;
	if(dat_slow) return;
	dat_slow=1;
	
	v_r=voltage_read(0x43);
	wetness=(char)(v_r/51.0*20.0);
	
	set=(float)wetness_set;
	v_o=204/(80-set)*(v_r/51.0*20.0)*1.0+255.0-16320/(80-set)*1.0;
	V_out=(char)v_o;
	if(wetness<=wetness_set) 	V_out=0;
	if(wetness>=80)					V_out=255;
	voltage_out(V_out);
	
	destance_read();
	if(destance>destance_set)
	{
		if(relay_con==0) relay_num++;
		relay_con=1;
	}
		else relay_con=0;
}
void led_proc()
{
	unsigned char i;
	if(led_slow) return;
	led_slow=1;
	for(i=3;i<8;i++){led_buf[i]=0;}
	if(freq_set<freq) led_buf[3]=1;
	if(wetness_set<wetness) led_buf[4]=1;
	if(destance_set<destance) led_buf[5]=1;
}
void key_proc()
{
	if(key_slow) return;
	key_slow=1;
	key_read();
	switch(key_d)
	{
		case 4:seg_mod++;if(seg_mod==4) seg_mod=0;break;
		case 5:
			if(seg_mod==3) {m3_set++;if(m3_set==3) m3_set=0;}
		break;
		case 6:	//+ freq_set,wetness_set,destance_set;
			if(seg_mod==3)
			{
				if(m3_set==0) freq_set+=500;
				if(m3_set==1) wetness_set+=10;
				if(m3_set==2) destance_set+=10;
			}
			if(seg_mod==2) m2_set^=1;
		break;
		case 7:	//- freq_set,wetness_set,destance_set;
			if(seg_mod==3)
			{
				if(m3_set==0) freq_set-=500;
				if(m3_set==1) wetness_set-=10;
				if(m3_set==2) destance_set-=10;
			}
			if(seg_mod==0) m0_set^=1;
			if(seg_mod==1) T1s_con=1;
		break;
	}
	if(key_u==7&&seg_mod==1) 
	{
		T1s_con=0;Timer_1s=0;
	}
}
void seg_proc()
{
	unsigned char i;
	if(seg_slow) return;
	seg_slow=1;
	for(i=0;i<8;i++) {point_buf[i]=0;seg_buf[i]=16;}
	seg_buf[2]=PWM_D;
	switch(seg_mod)
	{
		case 0:
			if(m0_set==0)			//hz
			{
				seg_buf[0]=15;
				if(freq>=10000)	seg_buf[3]=freq/10000%10;
				if(freq>=1000)	seg_buf[4]=freq/1000%10;
				if(freq>=100)	seg_buf[5]=freq/100%10;
				if(freq>=10)		seg_buf[6]=freq/10%10;
									seg_buf[7]=freq%10;
			}
			if(m0_set==1)			//khz
			{
				seg_buf[0]=15;
				if(freq>=1000000)	seg_buf[3]=freq/1000000%10;
				if(freq>=100000)	seg_buf[4]=freq/100000%10;
				if(freq>=10000)		seg_buf[5]=freq/10000%10;
				if(freq>=1000)		seg_buf[6]=freq/1000%10;
				else 					seg_buf[6]=0;
				point_buf[6] = 1;
										seg_buf[7]=freq/100%10;
			}
		break;
		case 1:						//%RH
				seg_buf[0]=17;
				seg_buf[5]=wetness/100%10;
				seg_buf[6]=wetness/10%10;
				seg_buf[7]=wetness%10;
		break;
		case 2:
			if(m2_set==0)			//cm
			{
				seg_buf[0]=10;
				if(destance>=100)	seg_buf[5]=destance/100%10;
				if(destance>=10)	seg_buf[6]=destance/10%10;
				if(destance>=0)		seg_buf[7]=destance%10;
			}
			if(m2_set==1)			//m
			{
				seg_buf[0]=10;
				if(destance>=100)	seg_buf[5]=destance/100%10;
				else 					seg_buf[5]=0;
				point_buf[5] = 1;
				if(destance>=10)	seg_buf[6]=destance/10%10;
				if(destance>=0)		seg_buf[7]=destance%10;
			}
		break;
		case 3: 
			seg_buf[0]=18;
			if(m3_set==0)			//freq_set
			{
				seg_buf[1]=1;
				if(freq_set>=10000)	seg_buf[5]=freq_set/10000%10;
				if(freq_set>=1000)		seg_buf[6]=freq_set/1000%10;
				else 						seg_buf[6]=0;
				point_buf[6] = 1;
											seg_buf[7]=freq_set/100%10;
			}
			if(m3_set==1)			//wetness_set
			{
				seg_buf[1]=2;
				seg_buf[6]=wetness_set/10;
				seg_buf[7]=wetness_set%10;
			}
			if(m3_set==2)			//destance_set
			{
				seg_buf[1]=3;
				if(destance_set>=100)	seg_buf[6]=destance_set/100%10;
				else 						seg_buf[6]=0;
				point_buf[6] = 1;	
											seg_buf[7]=destance_set/10%10;
			}
		break;
	}
}
void time1_isr() interrupt 3
{
	if(++all_slow==10) 
	{
		all_slow=0;
		if(++index==8) index=0;
		if(++seg_slow==50) seg_slow=0;
		if(++key_slow==100) key_slow=0;
		if(++led_slow==150) led_slow=0;
		if(++dat_slow==300) dat_slow=0;
		if(T1s_con)
		{
			if(++Timer_1s==1000)
			{
				Timer_1s=0;
				T1s_con=0;
				relay_num=0;
			}
		}
		freq_read();
		led_proc();key_proc();seg_proc();
		led(index,led_buf[index]);
		seg(index,seg_buf[index],point_buf[index]);
		if(++Timer_100ms==100) //0-freq 1-wetness 2-destance 3-set
		{
			Timer_100ms=0;
			if(seg_mod==0) led_buf[0]^=1;
			else led_buf[0]=0;
			if(seg_mod==1) led_buf[1]^=1;
			else led_buf[1]=0;
			if(seg_mod==2) led_buf[2]^=1;
			else led_buf[2]=0;
		}
	}
	if(freq_set<freq) PWM_D=8;
	else PWM_D=2;
	
	if(++PWM_count==10) PWM_count=0;
	
	if(PWM_count<PWM_D) motor_con=1;
	else motor_con=0;
	
	others(relay_con,motor_con);
}
void init()
{
	Timer1_Init();
	Timer2_Init();
	P2=P2&0X1F|0X80;
	P0=0XFF;
	P2=P2&0X1F|0XA0;
	P0=0X00;
}
void main()
{
	init();
	while(1)
	{
		dat_proc();
	}
}