#include <STC15F2K60S2.H>//1h14min
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
code unsigned char Seg_wale[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char seg_sd,key_sd;
unsigned char index,seg_bus[]={16,16,16,16,16,16,16,16};
unsigned char led_bus[]={0,0,0,0,0,0,0,0};
unsigned char key_down,key_val,key_old,key_up;
unsigned char distance_measure,distance_set;
unsigned char set;
unsigned char compute_count,compute_out,compute_save[10];
unsigned char eeprom__save1[5],eeprom__save2[5];
bit operate=0;		//0.无操作,1.加操作
unsigned char count_watch;
unsigned int Time_200;
unsigned char flash_count;
bit us_flag;

unsigned char work_mod=0;		//0-操作 1-回显 2-参数
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
void Timer0_Init(void)		//1000微秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0x00;			//设置定时器模式
	TL0 = 0x18;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;
	EA = 1;
}
void Delay20us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 57;
	while (--i);
}
void system_init()
{
	P2=P2&0x1f|0x80;
	P0=0xff;
	P2=P2&0x1f|0xa0;
	P0=0x00;
	P2=P2&0x1f;
}
unsigned char key_read()
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
void seg(unsigned char wale,val)
{
	P2=P2&0x1f|0xc0;
	P0=Seg_wale[wale];
	P2=P2&0x1f|0xe0;
	P0=Seg_Table[16];
	P2=P2&0x1f|0xc0;
	P0=Seg_wale[wale];
	P2=P2&0x1f|0xe0;
	P0=Seg_Table[val];
	P2=P2&0x1f;
	
}
void led(unsigned char addr,enable)
{
	unsigned char val,old;
	if(enable)
		val|=0x01<<addr;
	else
		val&=~(0x01<<addr);
	if(old!=val)
	{
		P2=P2&0x1f|0x80;
		P0=~val;
		old=val;
	}
}
/*-----------------------------------------*/
void us_wave()
{
	unsigned char i;
	for(i=0;i<10;i++)
	{
		US_TX=1;Delay20us();
		US_TX=0;
	}
}
unsigned char us_on()
{
	TH1=TL1=0;
	us_wave();
	TR1=1;
	while(US_RX==1&&TF1==0);
	TR1=0;
	us_flag=1;
	flash_count=0;
	if(TF1==1) 
	{
		TF1=0;
		return 0;
	}
	else 
		return ((TH1<<8)|TL1)*0.017;
}
void limit_proc()
{
	unsigned char v_out=0;
	if(set<compute_save[compute_count]) v_out=compute_save[compute_count]-set;
	v_out=v_out+v_out*0.02;
	if(v_out>=255) v_out=255;
	da_write(v_out);
}
void led_proc()
{
	unsigned char i;
	for(i=1;i<8;i++) led_bus[i]=0;
	if(work_mod==2) led_bus[6]=1;
	if(work_mod==1) led_bus[7]=1;
	if(work_mod==0) us_flag=1;
	else {us_flag=0;led_bus[0]=0;}
}
void key_proc()
{
	unsigned char i,j;
	if(key_sd) return;
	key_sd=1;
	
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	
	
	switch(key_down)
	{
		case 4:
			work_mod=0;
			compute_count++;
			if(compute_count==10) compute_count=0;
			distance_measure=us_on();
			compute_save[compute_count]=distance_measure;
			for(i=0,j=0;i<5;i++) eeprom__save1[j++]=compute_save[i];
			for(i=5,j=0;i<10;i++) eeprom__save2[j++]=compute_save[i];
			eep_write(eeprom__save1,0,5);
			Delay20ms();
			eep_write(eeprom__save2,8,5);
		break;
		case 5:
				work_mod=1;
				count_watch=compute_count;
		break;
		case 6:
			if(work_mod==2)
			{
				distance_set=set;
				work_mod=0;
			}
			if(work_mod!=2)
			{
				set=distance_set;
				work_mod=2;
			}
		break;
		case 7:
			switch(work_mod)
			{
				case 0:
					operate^=1;
					if(operate==1)
					{
						compute_out=compute_save[compute_count]+compute_save[compute_count-1];
						if(compute_count==0)
						{
							compute_out=compute_save[compute_count]+compute_save[9];
						}
					}
				break;
				case 1:
					count_watch++;
					if(count_watch==10) count_watch=0;
				break;
				case 2:
					set+=10;
					if(set>90) set=0;
				break;
			}
		break;
	}
}
void seg_proc()
{
	unsigned char i;
	if(seg_sd) return;
	seg_sd=1;
	for(i=0;i<8;i++) seg_bus[i]=16;
	switch(work_mod)
	{
		case 0:
			if(work_mod==0)
			{
				seg_bus[0]=operate;
				if(operate==0)
				{
					if(compute_count==0)
					{
						seg_bus[2]=compute_save[9]/100%10;
						seg_bus[3]=compute_save[9]/10%10;
						seg_bus[4]=compute_save[9]%10;
					}
					if(compute_count!=0)
					{
						seg_bus[2]=compute_save[compute_count-1]/100%10;
						seg_bus[3]=compute_save[compute_count-1]/10%10;
						seg_bus[4]=compute_save[compute_count-1]%10;
					}
				}
				if(operate==1)
				{
					if(compute_out>=100)
						seg_bus[2]=compute_out/100%10;
					if(compute_out>=10)
						seg_bus[3]=compute_out/10%10;
					if(compute_out>=0)
						seg_bus[4]=compute_out%10;
				}
				seg_bus[5]=compute_save[compute_count]/100%10;
				seg_bus[6]=compute_save[compute_count]/10%10;
				seg_bus[7]=compute_save[compute_count]%10;
			}
		break;
		case 1://count_watch
				seg_bus[0]=(count_watch+1)/10;
				seg_bus[1]=(count_watch+1)%10;
			seg_bus[5]=compute_save[count_watch]/100%10;
			seg_bus[6]=compute_save[count_watch]/10%10;
			seg_bus[7]=compute_save[count_watch]%10;
		break;
		case 2:
			seg_bus[0]=15;
			seg_bus[6]=set/10%10;
			seg_bus[7]=set%10;
		break;
	}
}
void Timer0_serve(void) interrupt 1
{
	if(++seg_sd==50) seg_sd=0;
	if(++key_sd==100) key_sd=0;
	if(++index==8) index=0;
	seg(index,seg_bus[index]);
	led(index,led_bus[index]);
	led_proc();key_proc();seg_proc();
	
	if(us_flag==1)
	{
		if(flash_count!=10)
		{
			if(++Time_200==200)
			{
				Time_200=0;
				flash_count++;
				led_bus[0]^=1;
			}
	}
		if(flash_count==10)
		{
			us_flag=0;
			flash_count=0;
			Time_200=0;
			led_bus[0]=0;
		}
	}
}
void main()
{
	unsigned char i,j;
	eep_read(eeprom__save1,0,5);
	Delay20ms();
	eep_read(eeprom__save2,8,5);
	Delay20ms();
	for(i=0,j=0;i<5;i++) compute_save[i]=eeprom__save1[j++];
	for(i=5,j=0;i<10;i++) compute_save[i]=eeprom__save2[j++];
	system_init();
	Timer0_Init();
	while(1)
	{	
		limit_proc();
	}
}