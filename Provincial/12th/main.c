#include <STC15F2K60S2.H>
#include <onewire.h>
#include <iic.h>

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
0xff,//10
0x88,//A 11
0xc6,//C 12
0x8c,//P 13
};
code unsigned char Seg_Wale[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char key_slow_down,seg_slow_down,seg_num;
unsigned char seg_show_mod=1;	//1-温度显示 2-参数设置 3-DAC输出
unsigned char key_val,key_old,key_up,key_down;
unsigned char seg_bus[]={10,10,10,10,10,10,10,10};
unsigned char seg_point[]={0,0,0,0,0,0,0,0};
unsigned char led_bus[]={0,0,0,0,0,0,0,0};
float Temperature,Temperature_Set=25;
int Set_Prepare;
unsigned char V_out;
float V_out_show;
bit AD_mod=0;	//0-t_set(0`5)[1] 1-T()[2]

void Timer0Init(void);
void system_init();
void seg_show(unsigned char wale,vula,point);
unsigned char key_read();
void led_read(unsigned char addr,enable);

void AD_proc()
{
	float temp;
	if(AD_mod==0)
	{
		V_out=255;
		if(Temperature<Temperature_Set) V_out=0;
		V_out_show=(float)((float)V_out/255.0*5.0);
	}
	if(AD_mod==1)
	{
		temp=(Temperature*3.0/20.0-2.0);
		if(temp>=4.0) temp=4.0;
		if(temp<=2.0) temp=2.0;
		V_out_show=temp;
		V_out=(int)V_out_show*255/5;
	}
	DA_Write(V_out);
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
			if(seg_show_mod==1)
				Set_Prepare=Temperature_Set;
			seg_show_mod++;
			if(seg_show_mod==4) seg_show_mod=1;
		break;
		case 5:
			AD_mod^=1;
		break;
		case 8:
			if(seg_show_mod==2)
			{
				Set_Prepare--;
			}
		break;
		case 9:
			if(seg_show_mod==2)
			{
				Set_Prepare++;
			}
		break;
	}
	if(key_up==4&&seg_show_mod==3)  Temperature_Set=Set_Prepare;
}
void led_proc()
{
	unsigned char i;
	for(i=0;i<8;i++) led_bus[i]=0;
	if(AD_mod==0) led_bus[0]=1;
	if(seg_show_mod==1) led_bus[1]=1;
	if(seg_show_mod==2) led_bus[2]=1;
	if(seg_show_mod==3) led_bus[3]=1;
	//if(V_out_show==5) led_bus[7]=1;
}
void seg_proc()
{
	if(seg_slow_down) return;
	seg_slow_down=1;
	switch(seg_show_mod)
	{
		case 1:
			seg_bus[0]=12;
			seg_bus[1]=10;
			seg_bus[2]=10;
			seg_bus[3]=10;
			seg_bus[4]=(int)Temperature/10%10;
			seg_bus[5]=(int)Temperature%10;
				seg_point[5]=1;
			seg_bus[6]=(int)(Temperature*10.0)%10;
			seg_bus[7]=(int)(Temperature*100.0)%10;
		break;
		case 2:
			seg_bus[0]=13;
			seg_bus[1]=10;
			seg_bus[2]=10;
			seg_bus[3]=10;
			seg_bus[4]=10;
			seg_bus[5]=10;
				seg_point[5]=0;
			seg_bus[6]=(int)Set_Prepare/10%10;
			seg_bus[7]=(int)Set_Prepare%10;
		break;
		case 3:
			seg_bus[0]=11;
			seg_bus[1]=10;
			seg_bus[2]=10;
			seg_bus[3]=10;
			seg_bus[4]=10;
			seg_bus[5]=(int)V_out_show%10;
				seg_point[5]=1;
			seg_bus[6]=(int)(V_out_show*10)%10;
			seg_bus[7]=(int)(V_out_show*100)%10;
		break;
	}
}
void T0_R(void) interrupt 1
{
	if(++seg_slow_down==50) seg_slow_down=0;
	if(++key_slow_down==100) key_slow_down=0;
	if(++seg_num==8) seg_num=0;
	seg_show(seg_num,seg_bus[seg_num],seg_point[seg_num]);
	led_read(seg_num,led_bus[seg_num]);
	key_proc();
	seg_proc();
	led_proc();
}
void main()
{
	system_init();
	Timer0Init();
	while(1)
	{
		AD_proc();
		Temperature=T_Read();
	}
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
void led_read(unsigned char addr,enable)
{
	unsigned char temp=0x00,temp_old=0xff;
	if(enable) 
		temp|=0x01<<addr;
	else 
		temp&=~(0x01<<addr);
	if(temp!=temp_old)
	{
		P2 = P2 & 0x1f | 0x80;
		P0 =~ temp;
		P2 = P2 & 0x1f;
		temp_old=temp;
	}
}
void system_init()
{
	P2 = P2 & 0x1f | 0x80;
	P0 = 0xff;
	P2 = P2 & 0x1f | 0xa0;
	P0 = 0x00;
	P2 = P2 & 0x1f;
}
void seg_show(unsigned char wale,vula,point)
{
	P2 = P2 & 0x1f | 0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f | 0xe0;
	P0 = Seg_Table[10];
	
	P2 = P2 & 0x1f | 0xc0;
	P0 = Seg_Wale[wale];
	P2 = P2 & 0x1f | 0xe0;
	P0 = 0xff;
	if(point) P0 &= 0x7f;
	P0 &= Seg_Table[vula];
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
	EA =1;
	ET0 = 1;
}