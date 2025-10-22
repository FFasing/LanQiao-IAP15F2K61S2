#include <STC15F2K60S2.H>
#include <iic.h>
#include <intrins.h>
#include <onewire.h>

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
	0xbf,//- 17
	0x8c,//P 18
	0x8e//F 19
};
code unsigned char Seg_Wale[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char seg_slow,key_slow,led_slow,dat_slow;
seg_buf[]={16,16,16,16,16,16,16,16},
index,led_buf[]={0,0,0,0,0,0,0,0},point_buf[]={0,0,0,0,0,0,0,0};
unsigned char key_old,key_val,key_down,key_up;
unsigned int Timer_100,Timer_2000,Timer_6000;
bit T2S_CON,T6S_CON;

float 			temperature;
float 			destance;
int 				compare;
float 			V_wave;
unsigned char 	L_DAC;
unsigned char 	temperature_set,
					destance_set;
unsigned char 	mod0_set;//0-cm 1-m
unsigned char 	mod1_set;//0-destance_set 1- temperature_set
unsigned char	mod2_set;//0-compare 1-V_wave 2-L_DAC

bit RELAY;
unsigned char mod;//0-测距 1-参数 2-工厂

bit ADC_CON=0;
bit KEY_CON=1;
bit RECORE_FLAG=0;
float destance_save;
void data_init()
{
	mod=0;
	mod0_set=0;
	destance_set=40;
	temperature_set=30;
	compare=0;
	V_wave=340;
	L_DAC=10;
}
void Timer0_Init(void)		//1000微秒@12.000MHz
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
void init()
{
	Timer0_Init();
	P2 = P2 & 0X1F | 0X80;
	P0 = 0XFF;
	P2 = P2 & 0X1F | 0XA0;
	P0 = 0X00;
}
void seg(unsigned char wale,valu,point)
{
	P2 = P2 & 0X1F | 0XC0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0X1F | 0XE0;
	P0 = Seg_Table[16];
	
	P2 = P2 & 0X1F | 0XC0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0X1F | 0XE0;
	P0 = Seg_Table[valu];
	if(point) P0&=0X7F;
} 
void led(unsigned char addr,enable)
{
	unsigned char v=0x00,o=0xff;
	if(enable)
		v|=0x01<<addr;
	else
		v&=~(0x01<<addr);
	if(o!=v){
	P2 = P2 & 0X1F | 0X80;
	P0 = ~v;
	o=v;
	}
}
void relay(unsigned char enable)
{
	unsigned char v=0x00,o=0xff;
	if(enable)
		v|=0x10;
	else
		v&=~0x10;
	if(o!=v){
	P2 = P2 & 0X1F | 0XA0;
	P0 = v;
	o=v;
	}
}
void key()
{
	unsigned char temp=0;
	ROW1=ROW2=ROW3=ROW4=1;
	COL1=0;COL2=COL3=COL4=1;
	if(ROW4==0) temp=4;
	if(ROW3==0) temp=5;
	COL2=0;COL1=COL3=COL4=1;
	if(ROW4==0) temp=8;
	if(ROW3==0) temp=9;
	if(ROW4==0&&ROW3==0) temp=1;
	key_val=temp;
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
}
void Delay20us(void)	//@20.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 97;
	while (--i);
}
void destance_read()
{
	unsigned char i;
	TMOD &= 0X0F;
	TH1=TL1=0;
	for(i=0;i<10;i++)
	{
		US_TX=1;Delay20us();US_TX=0;
	}
	TR1=1;
	while(US_RX==1&&TF1==0);
	TR1=0;
	if(TF1==1)
	{
		TF1=0;
		destance=0;
	}
	else
	{
		destance=((TH1<<8)|TL1)*(V_wave/20000.0);
	}
	destance+=compare;
	if(destance<=0) destance=0;
}
void dat_limit()
{//compare;V_wave;L_DAC;temperature_set,destance_set;
	if(destance_set<=10) 	destance_set=10;
	if(destance_set>=90) 	destance_set=90;
	
	if(temperature_set<=0) 	temperature_set=0;
	if(temperature_set>=80) 	temperature_set=80;
	
	if(compare<=-90) 	compare=-90;
	if(compare>=90) 	compare=90;
	
	if(V_wave<=10) 		V_wave=10;
	if(V_wave>=9990) 	V_wave=9990;
	
	if(L_DAC<=1) 	L_DAC=1;
	if(L_DAC>=20) 	L_DAC=20;
}
void led_p()
{
	unsigned char i,temp;
	if(led_slow) return;
	led_slow=1;
	for(i=1;i<8;i++) led_buf[i]=0;
	if(mod!=2)
	{
		led_buf[0]=0;
		if(mod==0)
		{
			temp=destance;
			for(i=0;i<8;i++) led_buf[i]=(temp>>(7-i))&0x01;
		}
		if(mod==1) led_buf[7]=1;
	}
	if((destance_set-5)<=destance&&(destance_set+5)>=destance
	  &&temperature_set>=temperature) RELAY=1;
	else RELAY=0;
}
void key_p()
{
	if(key_slow) return;
	key_slow=1;key();
	switch(key_down)
	{
		case 4:
			mod++;if(mod>=3) mod=0;
		break;
		case 5:
			if(mod==0) mod0_set++;
			if(mod==1) mod1_set++;
			if(mod==2) mod2_set++;
			if(mod0_set>=2) mod0_set=0;
			if(mod1_set>=2) mod1_set=0;
			if(mod2_set>=3) mod2_set=0;
		break;
		case 8://++
			if(mod==0) T6S_CON=1;
			if(mod==1) switch(mod1_set)
			{
				case 0:	destance_set+=10;		break;
				case 1:	temperature_set++;		break;
			}
			if(mod==2) switch(mod2_set)
			{
				case 0:	compare+=5;	break;
				case 1:	V_wave+=10;	break;
				case 2:	L_DAC++;		break;
			}
		break;
		case 9://--ADC_CON
			if(mod==0&&RECORE_FLAG==1) ADC_CON^=1;
			if(mod==1) switch(mod1_set)
			{
				case 0:	destance_set-=10;		break;
				case 1:	temperature_set--;		break;
			}
			if(mod==2) switch(mod2_set)
			{
				case 0:	compare-=5;	break;
				case 1:	V_wave-=10;	break;
				case 2:	L_DAC--;		break;
			}
		break;
	}
	dat_limit();
	if(key_down==1)
	{
		T2S_CON=1;
		Timer_2000=0;
	}
	if(key_up==1)
	{
		T2S_CON=0;
		Timer_2000=0;
	}
}
void seg_p()
{/*mod 0-测距 1-参数 2-工厂*/
	unsigned char i;
	int compare_temp;
	if(seg_slow) return;
	seg_slow=1;
	for(i=0;i<8;i++)
	{seg_buf[i]=16;point_buf[i]=0;}
	seg_buf[3]=key_down;
	if(mod==0)
	{//temperature destance
		switch(mod0_set)
		{//mod0_set 0-cm 1-m
			case 0:
				seg_buf[0]=(int)temperature/10%10;
				seg_buf[1]=(int)temperature%10;
				point_buf[1]=1;
				seg_buf[2]=(int)(temperature*10)%10;
			break;
			case 1:
				seg_buf[0]=(int)temperature/100%10;
				point_buf[0]=1;
				seg_buf[1]=(int)temperature/10%10;
				seg_buf[2]=(int)temperature%10;
			break;
		}
		seg_buf[3]=17;
		if(destance>=1000)	seg_buf[4]=(int)destance/1000%10;
		if(destance>=100) 	seg_buf[5]=(int)destance/100%10;
		if(destance>=10) 	seg_buf[6]=(int)destance/10%10;
		if(destance>=0) 	seg_buf[7]=(int)destance%10;
	}
	if(mod==1)
	{//temperature_set,destance_set
		seg_buf[0]=18;
		switch(mod1_set)
		{//0-destance_set 1- temperature_set
			case 0:
				seg_buf[1]=1;
				seg_buf[6]=destance_set/10%10;
				seg_buf[7]=destance_set%10;
			break;
			case 1:
				seg_buf[1]=2;
				seg_buf[6]=temperature_set/10%10;
				seg_buf[7]=temperature_set%10;
			break;
		}
	}
	if(mod==2)
	{//compare V_wave L_DAC
		seg_buf[0]=19;
		switch(mod2_set)
		{//0-compare 1-V_wave 2-L_DAC
			case 0:
				compare_temp=compare;
				seg_buf[1]=1;
				if(compare_temp<0)//- 17
				{
					compare_temp*=(-1);
					if(compare_temp>=100) 	seg_buf[4]=17;
					if(compare_temp>=10) 		seg_buf[5]=17;
					if(compare_temp>0) 		seg_buf[6]=17;
				}
				if(compare_temp>=100)		seg_buf[5]=compare_temp/100%10;
				if(compare_temp>=10)		seg_buf[6]=compare_temp/10%10;
				if(compare_temp>=0)		seg_buf[7]=compare_temp%10;
			break;
			case 1:
				seg_buf[1]=2;
				if(V_wave>=1000)	seg_buf[4]=(int)V_wave/1000%10;
				if(V_wave>=100)		seg_buf[5]=(int)V_wave/100%10;
				if(V_wave>=10)		seg_buf[6]=(int)V_wave/10%10;
				if(V_wave>=0)		seg_buf[7]=(int)V_wave%10;
			break;
			case 2:
				seg_buf[1]=3;
				seg_buf[6]=L_DAC/10;
				point_buf[6]=1;
				seg_buf[7]=L_DAC%10;
			break;
		}
	}
}
void Timer0_Isr(void) interrupt 1
{//seg_slow,key_slow,led_slo,dat_slow
	if(++seg_slow==50) seg_slow=0;
	if(++key_slow==100) key_slow=0;
	if(++led_slow==150) led_slow=0;
	if(++dat_slow==300) dat_slow=0;
	if(++index==8) index=0;
	seg(index,seg_buf[index],point_buf[index]);
	led(index,led_buf[index]);
	led_p();seg_p();relay(RELAY);
	if(KEY_CON) key_p();
	if(++Timer_100==100)
	{	if(mod==2) led_buf[0]^=1;
		Timer_100=0;
	}
	if(T2S_CON)
	{	if(++Timer_2000==2000)
		{
			T2S_CON=0;
			Timer_2000=0;
			data_init();
		}
	}
	else 	Timer_2000=0;
	if(T6S_CON)
	{	KEY_CON=0;
		destance_save=destance;
		if(++Timer_6000==6000)
		{
			T6S_CON=0;
			Timer_6000=0;
			RECORE_FLAG=1;
			KEY_CON=1;
		}
	}
	else 	Timer_6000=0;
}
void dat_p()
{
	double VL,VO;
	if(dat_slow) return;
	dat_slow=1;
	VO=(0.625-0.0125*(float)L_DAC)*destance_save+1.125*(float)L_DAC-6.25;
	if(destance_save>=90) VO=50;
	if(destance_save<=10) VO=10;
	if(ADC_CON) voltage_out((char)(VO*5.1));
	
	destance_read();
	
	temperature=temperature_read();
}
void main()
{
	data_init();
	init();
	while(1)
	{
		dat_p();
	}
}