#include <STC15F2K60S2.H>
#include <onewire.h>
#include <intrins.h>
#include <iic.h>
#include <stdio.h>

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
	0xff,// 16
	0xc7,//L 17
   0x8c//P 18
};
code unsigned char Seg_Wale[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char index,seg_buf[]={16,16,16,16,16,16,16,16},point_buf[]={0,0,0,0,0,0,0,0};
xdata unsigned char led_slow,Uart_slwo,dat_slow,seg_slow,key_slow,led_buf[]={0,0,0,0,0,0,0,0};
unsigned char key_val,key_old,key_down,key_up;
unsigned int Timer_1000;
bit T1000_con,DAC_Con;
unsigned char T1000_flag;//12-12 13-13

float te,temperature;
float destance;
xdata unsigned char eeprom_save[5];//0-温参 1-距参 2,3,4-change_num 
xdata unsigned char Uart_Recv[10];
xdata unsigned char Uart_Revc_Index;
xdata unsigned char Uart_Send[10];
xdata unsigned char V_out;

xdata int temperature_set=30,destance_set=35;
unsigned char temperature_set_old,destance_set_old;
unsigned char seg_mod;//0-数据 1-参数 
unsigned char mod0_set;//0-温度 1-距离 2-变更次数
unsigned char mod1_set;//0-温度 1-距离
unsigned int change_num=12345;

void send_byte(unsigned char dat)
{
   SBUF=dat;
   while(TI==0);
   TI=0;
}
void send_string(unsigned char *dat)
{
   while(*dat!='\0')
      send_byte(*dat++);
}
void Delay20ms(void)	//@20.000MHz
{
	unsigned char data i, j, k;

	_nop_();
	_nop_();
	i = 2;
	j = 134;
	k = 20;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}
void Delay20us(void)	//@20.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 97;
	while (--i);
}
void us_wale(void)
{
	unsigned char i;
	for(i=0;i<10;i++)
	{
		US_TX=1;Delay20us();
		US_TX=0;
	}
}
void UartInit(void)		//4800bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x01;		//串口1选择定时器2为波特率发生器
	AUXR &= 0xFB;		//定时器2时钟为Fosc/12,即12T
	T2L = 0xCC;		//设定定时初值
	T2H = 0xFF;		//设定定时初值
	AUXR |= 0x10;		//启动定时器2
   ES = 1;
	EA = 1;
}
void us_read(void)
{
	TMOD &= 0x0F;		//设置定时器模式
	TH1=TL1=0;
	us_wale();
	TR1=1;
	while(US_RX==1&&TF1==0);
	TR1=0;
	if(TF1==1)
	{
		TF1=0;
		destance=0;
	}
	else 
		destance=((TH1<<8)|TL1)*0.017;
   
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
	if(point) P0&=~0x80;
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
unsigned char key(void)
{
	unsigned char temp=0;
	ROW1=ROW2=ROW3=ROW4=1;
	COL3=0;COL1=COL2=COL4=1;
	if(ROW4==0) temp=12;
	if(ROW3==0) temp=13;
	COL4=0;COL1=COL2=COL3=1;
	if(ROW4==0) temp=16;
	if(ROW3==0) temp=17;
	return temp;
}
void Eeprom_Init(void)
{
   eeproom_read(eeprom_save,1,5);
   Delay20ms();
   
   temperature_set=eeprom_save[0];
   destance_set=eeprom_save[1];
   
   change_num=0;
   change_num+=eeprom_save[2]*10000;
   change_num+=eeprom_save[3]*100;
   change_num+=eeprom_save[4];
}
void Eeprom_Write(void)
{
   unsigned char temp;
   eeprom_save[0]=temperature_set;
   eeprom_save[1]=destance_set;
   temp =change_num/10000%10;
   eeprom_save[2]=temp;
   temp =change_num/1000%10*10;
   temp+=change_num/100%10;
   eeprom_save[3]=temp;
   temp =change_num/10%10*10;
   temp+=change_num%10;
   eeprom_save[4]=temp;
   eeproom_write(eeprom_save,1,5);
   Delay20ms();
}
void set_limit(void)
{
   if(temperature_set<=0) temperature_set=0;
   if(destance_set<=0) destance_set=0;
   if(temperature_set>=100) temperature_set=99;
   if(destance_set>=100) destance_set=99;
}
void led_proc(void)
{
	unsigned char i;
	if(led_slow) return;
	led_slow=0;
	for(i=0;i<8;i++) led_buf[i]=0;
	
	if(temperature_set<=temperature) led_buf[0]=1;
	if(destance_set<=destance) led_buf[1]=1;
	
	if(destance<=destance_set) 
		V_out=2;
	if(destance>destance_set)
		V_out=5;
	
	 led_buf[2]=DAC_Con;
}
void key_proc(void)
{
	if(key_slow) return;
	key_slow=1;
	key_val=key();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	
	switch(key_down)
	{
		case 13:
         T1000_flag=13;
         T1000_con=1;
			seg_mod^=1;
			mod0_set=mod1_set=0;
         if(seg_mod==1)
         {
            temperature_set_old=temperature_set;
            destance_set_old=destance_set;
         }
         if(seg_mod==0)
         {
            if(temperature_set_old!= temperature_set||destance_set_old!=destance_set)
               change_num++;
         }
         Eeprom_Write();
		break;
		case 12:
         T1000_flag=12;
         T1000_con=1;
			if(seg_mod==0) mod0_set++;
			if(seg_mod==1) mod1_set++;
			if(mod0_set>=3) mod0_set=0;
			if(mod1_set>=2) mod1_set=0;
		break;
      case 16:
         if(seg_mod==1)
         {
            if(mod1_set==0) {temperature_set-=2;}
            if(mod1_set==1) {destance_set-=5;}
         }
      break;
      case 17:
         if(seg_mod==1)
         {
            if(mod1_set==0) {temperature_set+=2;}
            if(mod1_set==1) {destance_set+=5;}
         }
      break;
	}
   set_limit();
   if(key_up==12) {T1000_con=0;T1000_flag=0;}
   if(key_up==13) {T1000_con=0;T1000_flag=0;}
}
void seg_proc(void)
{
	unsigned char i;
	if(seg_slow) return;
	seg_slow=1;
	
	for(i=0;i<8;i++) {
		seg_buf[i]=16;
		point_buf[i]=0;}
		if(seg_mod==0) switch(mod0_set){
			case 0:
				seg_buf[0]=15;
				seg_buf[4]=(int)temperature/10%10;
				seg_buf[5]=(int)temperature%10;
				point_buf[5]=1;
				seg_buf[6]=(int)(temperature*100)/10%10;
				seg_buf[7]=(int)(temperature*100)%10;
			break;
			case 1:
				seg_buf[0]=17;
				seg_buf[5]=(int)destance/100%10;
				seg_buf[6]=(int)destance/10%10;
				seg_buf[7]=(int)destance%10;
			break;
			case 2:
				seg_buf[0]=0;
				if(change_num>=10000) seg_buf[3]=change_num/10000%10;
				if(change_num>=1000) seg_buf[4]=change_num/1000%10;
				if(change_num>=100) seg_buf[5]=change_num/100%10;
				if(change_num>=10) seg_buf[6]=change_num/10%10;
				if(change_num>=0) seg_buf[7]=change_num%10;
			break;}
      if(seg_mod==1) switch(mod1_set){
			case 0:
				seg_buf[0]=18;
				seg_buf[6]=temperature_set/10%10;
				seg_buf[7]=temperature_set%10;
			break;
			case 1:
				seg_buf[0]=5;
				seg_buf[6]=destance_set/10%10;
				seg_buf[7]=destance_set%10;
			break;
      }
}
void uart_proc(void)
{
	float dat1,dat2;
	if(Uart_slwo) return;
	Uart_slwo=1;
   
	if(Uart_Revc_Index==6)
	{ 
		if(Uart_Recv[0]==0X53 && Uart_Recv[1]== 0X54
      && Uart_Recv[2]==0X5C && Uart_Recv[3]==0X72
      && Uart_Recv[4]==0X5C && Uart_Recv[5]==0X6E)
		{
			dat1=temperature;
			dat2=destance;
			sprintf(Uart_Send,"$%02d,%.2f\r\n",(unsigned int)dat2,dat1);
			send_string(Uart_Send);
		}
		else send_string("ERROR\r\n");
		Uart_Revc_Index=0;
	}
	
	if(Uart_Revc_Index==8)
	{
		if(Uart_Recv[0]=='P' && Uart_Recv[1]=='A' 
      && Uart_Recv[2]=='R' && Uart_Recv[3]=='A'
      && Uart_Recv[4]==0X5C && Uart_Recv[5]=='r'
		&& Uart_Recv[6]==0X5C && Uart_Recv[7]=='n')
		{
			sprintf(Uart_Send,"#%02d,%02d\r\n",(unsigned int)destance_set,(unsigned int)temperature_set);
			send_string(Uart_Send);
		}
		else send_string("ERROR\r\n");
		Uart_Revc_Index=0;
	}
}
void Timer0_Isr(void) interrupt 1
{
	if(++led_slow==100) led_slow=0;
	if(++dat_slow==1000) dat_slow=0;
	if(++Uart_slwo==100) Uart_slwo=0;
	if(++seg_slow==50) seg_slow=0;
	if(++key_slow==100) key_slow=0;
	if(++index==8) index=0;
	seg(index,seg_buf[index],point_buf[index]);
	led(index,led_buf[index]);
	key_proc();seg_proc();led_proc();uart_proc();
   if(T1000_con==1)
   {
      if(++Timer_1000==1000)
      {
         Timer_1000=0;
         if(T1000_flag==12)
         {
            T1000_flag=0;
            change_num=0;
            Eeprom_Write();
         }
         if(T1000_flag==13)
         {
            T1000_flag=0;
            DAC_Con^=1;
         }
      }
   }
}
void system_init()
{
   UartInit();
	Timer0_Init();
	P2=P2&0x1f|0x80;
	P0=0xff;
	P2=P2&0x1f|0xa0;
	P0=0x00;
	P2=P2&0x1f;
}
void dat_read(void)
{ 
	if(dat_slow) return;
	dat_slow=1;
	
	temperature=Temp_Read();
   us_read();
	if(DAC_Con) da_out(V_out*51);
	else da_out(0*51);
}
void main()
{  
   Eeprom_Init();
	system_init();
		dat_read();
	while(1)
	{
		dat_read();
	}
}

void Uart1_Isr(void) interrupt 4
{
   if (RI)				//检测串口1接收中断
	{  
      Uart_Recv[Uart_Revc_Index]=SBUF;
		Uart_Revc_Index++;
      RI=0;			//清除串口1接收中断请求位
	}
}
