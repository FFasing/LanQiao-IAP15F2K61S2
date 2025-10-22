#include <STC15F2K60S2.H>
#include <iic.h>
#include <onewire.h>
#include <ds1302.h>

sbit COL1 = P4^4;
sbit COL2 = P4^2;
sbit COL3 = P3^5;
sbit COL4 = P3^4;
sbit ROW1 = P3^0;
sbit ROW2 = P3^1;
sbit ROW3 = P3^2;
sbit ROW4 = P3^3;

code unsigned char Seg_Wale[] ={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
code unsigned char Seg_Table[] ={
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
0x88,//A 11
0x83,//b 12
0xc6,//C 13
0xa1,//d 14
0x86,//E 15
0x8e,//F 16
0xbf,//- 17
0x89,//H 18
0x8c//P  19
};

unsigned char seg_show_mod=0;		//0-时间界面 1-回显界面 2-参数界面 3-温湿度界面
unsigned char huixian_mod=0;		//0-温度 1-湿度 2-时间
unsigned char seg_slow_down,key_slow_down;
unsigned char seg_num;
unsigned char seg_bus[]={10,10,10,10,10,10,10,10};
unsigned char seg_point[]={0,0,0,0,0,0,0,0};
unsigned char led_bus[]={0,0,0,0,0,0,0,0};
unsigned char key_val,key_old,key_down,key_up;
xdata float freq,Shi_Du,Temperature;
xdata int Temper_compare=30;
unsigned int Timer_100,Timer_1000,Timer_2000,Timer_3000;
unsigned char RB2_VO,RD1_VO=100;
unsigned char Time[3]={21,31,58};	//h,m,s
xdata float T_Rem[10],T_Max,T_AV=0.0;
xdata float S_Rem[10],S_Max,S_AV=0.0;
unsigned int chufa_num=0;
unsigned char chufa_hour,chufa_min;
bit chufa_flag,chufa_val,freq_con=0;
bit init_flag,compare_big;

void chufa_proc(void)
{
	if(chufa_flag==0) RD1_VO=DA_Read(0x41);
	if((RD1_VO>=0)&&(RD1_VO<=20)) chufa_val=1;
	if(chufa_val)
	{
		if((RD1_VO>=0)&&(RD1_VO<=20)) chufa_val=1;
		else
		{
			chufa_flag=chufa_val;
			chufa_hour=Time[0];
			chufa_min=Time[1];
			chufa_val=0;
		}	
	}
}
void led_show(unsigned char addr,enable)
{
	unsigned char temp=0x00,temp_old=0xff;
	if(enable)
		temp |= 0x01<<addr;
	else
		temp &=~ (0x01<<addr);
	if(temp!=temp_old)
	{
		P2 = P2 & 0x1f | 0x80;//1000 4
		P0 =~ temp;
		P2 = P2 & 0x1f;
		temp_old=temp;
	}
}
unsigned char key_read()
{
	unsigned char i=0;
	ROW1=ROW2=ROW3=ROW4=1;
	COL1=0;COL2=COL3=COL4=1;
	if(ROW4==0) i = 4;
	if(ROW3==0) i = 5;
	ROW1=ROW2=ROW3=ROW4=1;
	COL2=0;COL1=COL3=COL4=1;
	if(ROW4==0) i = 8;
	if(ROW3==0) i = 9;
	return i;
}
void System_Init()
{
	P2 = P2 & 0x1f | 0x80;//1000 4
	P0 = 0xff;
	P2 = P2 & 0x1f | 0xa0;//1010 5
	P0 = 0x00;
}

void seg_show(unsigned char wale,vula,point)
{
	P2 = P2 & 0x1f | 0xc0;//1100 6
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f | 0xe0;//1110 7
	P0 = Seg_Table[10];
	
	P2 = P2 & 0x1f | 0xc0;//1100 6
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f | 0xe0;//1110 7
	P0 = Seg_Table[10];
	if(point) P0 &= 0x7f;
	P0 &= Seg_Table[vula];
	P2 = 0x1f;
}
void led_proc()
{
	if(seg_show_mod==0) led_bus[0]=1;
	else led_bus[0]=0;
	if(seg_show_mod==1) led_bus[1]=1;
	else led_bus[1]=0;
	if(seg_show_mod==2) led_bus[2]=1;
	else led_bus[2]=0;
	if((int)T_Rem[chufa_num-1]>Temper_compare) compare_big=1;
	else compare_big=0;
	if(Shi_Du==0&&chufa_num!=0)  led_bus[4]=1;
	if(chufa_num>=2&&T_Rem[chufa_num-1]>T_Rem[chufa_num-2]&&S_Rem[chufa_num-1]>S_Rem[chufa_num-2])
	led_bus[5]=1;
	else led_bus[5]=0;
}
void key_proc()
{
	unsigned int i;
	if(key_slow_down) return;
	key_slow_down=1;
	
	chufa_proc();
	
	key_val = key_read();
	key_down = key_val & (key_val ^ key_old);
	key_up =~ key_val & (key_val ^ key_old);
	key_old = key_val;
	
	switch(key_down)
	{
		case 4:
			seg_show_mod++;
			if(seg_show_mod==3) seg_show_mod=0;
			huixian_mod=0;
		break;
		case 5:
			if(seg_show_mod==1)
			{
				huixian_mod++;
				if(huixian_mod==3) huixian_mod=0;
			}
		break;
		case 8:
			if(seg_show_mod==2) Temper_compare++;
			if(Temper_compare==100) Temper_compare=0;
		break;
		case 9:
			if(seg_show_mod==2) Temper_compare--;
			if(Temper_compare==-1) Temper_compare=99;
			if(seg_show_mod==1&&huixian_mod==2) 
			{
				Timer_2000=0;
				init_flag=1;
			}
		break;
	}
	if(key_up==9) 
	{
		Timer_2000=0;
		if(init_flag==0)
		{
			for(i=0;i<10;i++)
			{
				T_Rem[i]=0.0;
				S_Rem[i]=0.0;
			}
			T_Rem[10],T_Max=0.0;T_AV=0.0;
			S_Rem[10],S_Max=0.0;S_AV=0.0;
			chufa_num=0;
			chufa_hour=chufa_min=0;
		}
		init_flag=0;
	}
}
void seg_proc()
{
	unsigned char i;
	if(seg_slow_down) return;
	seg_slow_down=1;
	
	//RB2_VO=DA_Read(0x43);
	//RD1_VO=DA_Read(0x41);
	switch(seg_show_mod)
	{
		case 0:
			Rtc_Read(Time);
			seg_bus[0]=(int)Time[0]/10;
			seg_bus[1]=(int)Time[0]%10;
			
			seg_bus[3]=(int)Time[1]/10;
			seg_bus[4]=(int)Time[1]%10;
			
			seg_bus[6]=(int)Time[2]/10;
			seg_bus[7]=(int)Time[2]%10;
		break;
		
		case 1:
			if(huixian_mod==0)	//温度
			{
				seg_bus[0]=13;
				seg_bus[1]=10;
				seg_bus[2]=(int)T_Max/10%10;
				seg_bus[3]=(int)T_Max%10;
				seg_bus[4]=17;
				seg_bus[5]=(int)T_AV/100%10;
				seg_bus[6]=(int)T_AV/10%10;
				seg_point[6]=1;
				seg_bus[7]=(int)T_AV%10;
			}
			if(huixian_mod==1)	//湿度
			{
				seg_bus[0]=18;
				seg_bus[1]=10;
				seg_bus[2]=(int)S_Max/10%10;
				seg_bus[3]=(int)S_Max%10;
				seg_bus[4]=17;
				seg_bus[5]=(int)S_AV/100%10;
				seg_bus[6]=(int)S_AV/10%10;
				seg_point[6]=1;
				seg_bus[7]=(int)S_AV%10;
			}
			if(huixian_mod==2)	//时间
			{
				seg_point[6]=0;
				seg_bus[0]=16;
				seg_bus[1]=(int)chufa_num/10%10;
				seg_bus[2]=(int)chufa_num%10;
				seg_bus[3]=(int)chufa_hour/10%10;
				seg_bus[4]=(int)chufa_hour%10;
				seg_bus[5]=17;
				seg_bus[6]=(int)chufa_min/10%10;
				seg_bus[7]=(int)chufa_min%10;
			}
		break;
			
		case 2:
			for(i=0;i<8;i++)
			{
				seg_point[i]=0;
				seg_bus[i]=10;
			}
			seg_bus[0]=19;
			seg_bus[6]=(int)Temper_compare/10%10;
			seg_bus[7]=(int)Temper_compare%10;
		break;
			
		case 3:
			for(i=0;i<8;i++)
			{
				seg_point[i]=0;
				seg_bus[i]=10;
			}
			seg_bus[0]=15;
			seg_bus[3]=(int)T_Rem[chufa_num]/10%10;
			seg_bus[4]=(int)T_Rem[chufa_num]%10;
			seg_bus[5]=17;
			seg_bus[6]=(int)S_Rem[chufa_num]/10%10;
			seg_bus[7]=(int)S_Rem[chufa_num]%10;
			
			if(Shi_Du==0)
			{
				seg_bus[6]=11;
				seg_bus[7]=11;
			}
		break;
		case 4:
			seg_bus[0]=chufa_val;
			seg_bus[1]=chufa_flag;
			seg_bus[5]=(int)RD1_VO/100%10;
			seg_bus[6]=(int)RD1_VO/10%10;
			seg_bus[7]=(int)RD1_VO%10;
		break;
	}
	
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
	EA = 1;
}
void Timer0Init(void)		//1毫秒@12.000MHz
{
	TMOD &= 0xF0;		//设置定时器模式
	TMOD |= 0x05;
	TL0 = 0x00;		//设置定时初值
	TH0 = 0x00;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;
	EA = 1;
}

void T1_R(void) interrupt 3
{
	unsigned char i,state_old;
	if(++seg_slow_down==50) seg_slow_down=0;
	if(++key_slow_down==60) key_slow_down=0;
	if(++seg_num==8) seg_num=0;
	seg_show(seg_num,seg_bus[seg_num],seg_point[seg_num]);
	led_show(seg_num,led_bus[seg_num]);
	if(freq_con==1)
	if(++Timer_1000==1000)
	{
		Timer_1000=0;
		freq = (TH0<<8)|TL0;
		Shi_Du = 2*freq/45.0 + (10.0/9.0);
		if(Shi_Du>90||Shi_Du<10) Shi_Du=0;
		TH0=TL0=0;
	}
	if(chufa_flag==1)
	{
		if(seg_show_mod!=3) 
		{
			state_old=seg_show_mod;
			T_Rem[chufa_num]=Temperature;
			S_Rem[chufa_num]=Shi_Du;			
		}
		freq_con=1;
		seg_show_mod=3;
		if(++Timer_3000==3000)
		{
			Timer_3000=0;
			chufa_flag=0;
			
			T_AV=S_AV=0.0;
			for(i=0;i<=chufa_num;i++)
			{
				T_AV+=T_Rem[i];
				S_AV+=S_Rem[i];
				if(T_Rem[chufa_num]>=T_Rem[i]) T_Max=T_Rem[chufa_num];
				if(S_Rem[chufa_num]>=S_Rem[i]) S_Max=S_Rem[chufa_num];
			}
			chufa_num++;
			T_AV=T_AV*10/chufa_num*1.0;
			S_AV=S_AV*10/chufa_num*1.0;
			
			seg_show_mod=state_old;
			freq_con=0;
		}
	}
	if(init_flag==1)
	{
		if(++Timer_2000==2000)
		{
			Timer_2000=0;
			init_flag=0;
		}
	}
	if(compare_big==1)
	{
		if(++Timer_100==100)
		{
			Timer_100=0;
			led_bus[3]^=1;
		}
	}
	seg_proc();
	key_proc();
}
void main()
{
	System_Init();
	Timer0Init();
	Timer1Init();
	Rtc_Write(Time);
	while(1)
	{
		freq_con=1;
		led_proc();
		Temperature=Temperature_Read();
	}
}