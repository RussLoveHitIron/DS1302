#include "STC12C5A60S2.H"
#include "intrins.h"


typedef unsigned char u8; 	
typedef unsigned int u16;

/***********定义Flash 操作等待时间及允许IAP/ISP/EEPROM 操作的常数(属于IAP_CONTR寄存器)***********/
#define ENABLE_ISP 0x81     //实测  22M可以使用

/*数码管端口定义*/
#define DIG_PORT 	P0
#define POS_PORT	P2      

// DS1302相关定义
sbit DSIO = P1^3; 		
sbit CE   = P1^4;		// 使能，读写完要禁止
sbit SCLK = P1^2;	   	

/*共阴数码管*/
unsigned char code gDuanMa[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,
					0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71};

u8 Init_Time[7];			// 用来存储DS1302中7个时间的BCD码值
u8 gTime[7];			//后续使用
u8 gDigValue[8];		// 动SS态数码管上显示的8个数值
					
u8 code READ_RAM_ADDR[1] = {0xc1};
u8 code WRITE_RAM_ADDR[1] = {0xc0};


/**********************/
/**********************/
u8 Set_Flag = 0;
u8 Full_Flag = 0;
u8 Empty_Flag = 0;
u8 View_Flag = 0;

u8 F1_Over = 0;
u8 F2_Over = 0;
u8 Set_Time[4];

u8 TimeViewing[4];
u8 check = 0;

u8 SZ_Min[24];
u8 SZ_Sec[24];


u8 Dis_F1[] = {0x40, 0x71, 0x06, 0x40};
u8 Dis_F2[] = {0x40, 0x71, 0x5b, 0x40};
u8 Dis_Full[] = {0x71, 0x3e, 0x06, 0x06};
u8 Dis_Empty[] = {0x40, 0x79, 0x73, 0x40};
/**********************/
/**********************/

u16 delay = 1000;

/****************EEPROM****************/
u16 ROM_Min_Addr = 0x0200;
u16 ROM_Sec_Addr = 0x0000;
//u16 ROM_Hour_Addr = 0x0040;
u16 ROM_Length_Addr = 0x01FF;



/**************************************/
	

// DS1302内部时间寄存器地址常量定义，这些地址值可查DS1302数据手册获取
u8 code DS1302_READ_ADDR[3] = {0x81, 0x83, 0x85}; 
u8 code DS1302_WRITE_ADDR[3] = {0x80, 0x82, 0x84};

u8 buzzer = 0x00;

u8 time = 0;

u8 code Led[]={0xe7,0xd7,0xb7,0x77};
u8 acled[4];
char cScanIndex;

u8  cKey;							/* 初始键值	*/
u8  cKeyCode;						/* 键值	*/
u8  cLongDelay;						/* 按键长按时间	*/
bit   bStill;						/* 是否松键标志 */

u16  nDelayKey;						/* 键盘延时变量，为定时中断间隔时间的整数倍 */


/*
**********************************************************************
*                         函数声明
**********************************************************************
*/
void Ds1302WriteReg(u8 addr, u8 dat);
u8 Ds1302ReadReg(u8 addr);
void Ds1302WriteTime(void);
void Ds1302InitTime(void);
void UpdateDigValue(void);
void DigDisplayTime(void);

/***********************EEPROM**************************/
void IAP_Disable();
unsigned char Byte_Read(unsigned int add);
void Byte_Program(unsigned int add,unsigned char ch);
void Sector_Erase(unsigned int add); 
void Init0(void);
/*******************************************************/

/*******************************/

void DisposeKEY()
{
	u8 temp1;
	u8 temp2;
	u8 j;
	
	/*检测按键键值*/
	switch(cKey)
	{
	/*按下按键K1，进入设置模式*/
	case 6:										
		if(bStill==0)
		{
			Set_Flag = 1;		//首先进入设置模式1，显示“F1”
			bStill = 1;

			/*判断存储的时间数据是否超过最大组数*/
			if (Byte_Read(ROM_Length_Addr) >= 24)
			{
				Full_Flag = 1;		//已达最大组数，设满，显示“FULL”并退出设置
				break;
			}
			else Full_Flag = 0;		/*未满，初始化下一组时间数据为0*/
			SZ_Sec[Byte_Read(ROM_Length_Addr)] = 0;	
			SZ_Min[Byte_Read(ROM_Length_Addr)] = 0;
			//bStill = 1;
			while (Set_Flag != 3);	//等待可以进入设置模式3的标志，以免出现按键混乱
			/**************************/
			while (1)
			{	
				/*判断是否完成设置*/
				if (F1_Over)
				{
					F1_Over = 0;
					Set_Flag = 0;					
					break;
				}
				switch (cKey)		//检测按键
				{
					case 5:			//按下按键K2，设置秒加1
						if (bStill == 0)
						{
							if (++SZ_Sec[Byte_Read(ROM_Length_Addr)] > 59)		SZ_Sec[Byte_Read(ROM_Length_Addr)] = 0;
							bStill = 1;
						}
						break;
					case 3:			//按下按键K3，设置分加1
						if (bStill == 0)
						{
							
							if (++SZ_Min[Byte_Read(ROM_Length_Addr)] > 59)		SZ_Min[Byte_Read(ROM_Length_Addr)] = 0;
							bStill = 1;							
						}
						break;
					case 6:			//按下按键K1，退出设置，并将时间存储到EEPROM中
						if (bStill == 0)
						{
							F1_Over = 1;
							bStill = 1;

							temp1 = Byte_Read(ROM_Min_Addr + Byte_Read(ROM_Length_Addr));
							temp2 = Byte_Read(ROM_Sec_Addr + Byte_Read(ROM_Length_Addr));
							if (temp1 == 0xff && temp2 == 0xff)
							{
								temp1 = SZ_Min[Byte_Read(ROM_Length_Addr)];
								temp2 = SZ_Sec[Byte_Read(ROM_Length_Addr)];
							
								Byte_Program(ROM_Min_Addr + Byte_Read(ROM_Length_Addr), temp1);
								Byte_Program(ROM_Sec_Addr + Byte_Read(ROM_Length_Addr), temp2);
							
								if (Byte_Read(ROM_Min_Addr + Byte_Read(ROM_Length_Addr)) != 0xff &&
									Byte_Read(ROM_Sec_Addr + Byte_Read(ROM_Length_Addr)) != 0xff)
								{
									/*写满再擦？？？怎么弄*/
									/**。/
									temp1 = Byte_Read(ROM_Length_Addr); 
									temp1++;
									Sector_Erase(ROM_Length_Addr);
									Byte_Program(ROM_Length_Addr, temp1);
									for (j = 0 ; j < Byte_Read(ROM_Length_Addr) ; j++)
									{
										Byte_Program(ROM_Sec_Addr + j, SZ_Sec[j]);
									}
								}
							}																		
						}
						break;
				}
			}							
		}			
		break;
		
	/*按下按键K2，进入浏览模式*/	
	case 5:
		if(bStill == 0)
		{
			View_Flag = 1;		//首先进入浏览模式1，显示“F2”
			bStill = 1;
			/*判断存储的时间数据是否为空*/
			if (Byte_Read(ROM_Length_Addr) == 0)
			{
				Empty_Flag = 1;		//空，显示“EP”即empty
				break;
			}
			else Empty_Flag = 0;	/*未空*/
			while (View_Flag != 3);		//等待可以进入浏览模式3的标志，以免出现按键混乱
			while (1)
			{		
				/*判断是否完成浏览*/
				if (F2_Over)
				{
					F2_Over = 0;
					View_Flag = 0;
					break;
				}
				switch (cKey)		//检测按键
				{
					case 5:			//按下按键K2，查看上一组数据
						if (bStill == 0)
						{
							if (++check > Byte_Read(ROM_Length_Addr) - 1)	check = 0;
							bStill = 1;
						}
						break;		
					case 3:			//按下按键K3，查看下一组数据
						if (bStill == 0)
						{							
							if (--check > 40)	check = Byte_Read(ROM_Length_Addr) - 1;
							bStill = 1;
						}
						break;
					case 6:			//按下按键K1，退出浏览模式
						if (bStill == 0)
						{
							F2_Over = 1;
							bStill = 1;							
						}
						break;
				}
			}	
		}
		break;
		
	/*case 3:
		if (bStill == 0)
		{
			Sector_Erase(ROM_Length_Addr);
			Sector_Erase(ROM_Min_Addr);
			Set_Flag = 0;
			View_Flag = 0;
			bStill = 1;
		}
		Ds1302InitTime();		//DS1302初始化时间
		//斷電一次，重新進入？
		//断电
		//初始化0？
		break;*/
		
	DigDisplayTime();
	cKey = 0;
	}
}

u8 D2BCD(u8 Dec)
{
	return (((Dec / 10) << 4) | (Dec % 10));
}

	
void Set24Time(u8 length)
{
	Set_Time[0] = gDuanMa[SZ_Min[length] / 10];
	Set_Time[1] = gDuanMa[SZ_Min[length] % 10] | 0x80;
	Set_Time[2] = gDuanMa[SZ_Sec[length] / 10];
	Set_Time[3] = gDuanMa[SZ_Sec[length] % 10];
}

void View_Time(u8 check_sign)
{
	TimeViewing[0] = gDuanMa[(Byte_Read(ROM_Min_Addr + check_sign)) / 10];
	TimeViewing[1] = gDuanMa[(Byte_Read(ROM_Min_Addr + check_sign)) % 10] | 0x80;
	TimeViewing[2] = gDuanMa[(Byte_Read(ROM_Sec_Addr + check_sign)) / 10];
	TimeViewing[3] = gDuanMa[(Byte_Read(ROM_Sec_Addr + check_sign)) % 10];
}

void main(void)
{	
	u8 i = 0, on_flag = 0;
	u8 off = 0x00;
	u8 on = 0x08;
	
	P0M1=0x00; //设置P0口为推挽输出
	P0M0=0xff;
	
	P2M1=0x00; //设置P2口为推挽输出
	P2M0=0xff;
	
	Init0();	//中断初始化
	
	/*判断EEPROM中时间组数长度是否初始化*/
	if (Byte_Read(ROM_Length_Addr) == 0xff)
	{
		/*没有初始化，初始化为0*/
		Sector_Erase(ROM_Length_Addr);
		Byte_Program(ROM_Length_Addr, 0x00);
	}
	Ds1302InitTime();		//DS1302初始化时间
	
	while (1)
	{
		/*更新时间，并显示在数码管上*/
		UpdateDigValue();
		DigDisplayTime();
		
		/*逐个检测所设置的所有打铃时间*/
		for (i = 0 ; i < Byte_Read(ROM_Length_Addr) ; i++)
		{
			/*判断当前时间是否与设置的打铃时间相符*/
			if (Ds1302ReadReg(DS1302_READ_ADDR[1]) == D2BCD(Byte_Read(ROM_Min_Addr + i)) &&	
				Ds1302ReadReg(DS1302_READ_ADDR[0]) == D2BCD(Byte_Read(ROM_Sec_Addr + i)))
			{
				/*相符，打开控制蜂鸣器的开关*/
				on_flag = 1;
				break;
			}
			/*不符，关闭蜂鸣器*/
			on_flag = 0;
			
		}
		
		/*判断蜂鸣器标志位是否打开*/
		if (on_flag)
		{
			/*蜂鸣器响*/
			buzzer = on;
		}		
		else
		{
			/*蜂鸣器熄*/
			buzzer = off;
		}
		
		
		if(cKey != 0x07)
		{
			DisposeKEY();	// 响应按键操作     
		}						
	}
}

void Init0(void)
{
	TMOD = 0x02;		// T0设为定时器模式，仅用TR0控制打开

	TH0 = -200;	
	//TL0 = -200;			// 
	ET0 = 1;			// 打开定时器0中断允许
	EA = 1;				// 打开总中断
	TR0 = 1;			// 开始计时
}

void Ds1302WriteReg(u8 addr, u8 dat)
{
	u8 i = 0;

	CE = 0;
	_nop_();				// 延时

	SCLK = 0;				// 将SCLK置低
	_nop_();

	CE = 1; 				// CE拉高以使能
	_nop_();

	for (i=0; i<8; i++)		// 循环逐位发送addr的8个bit，低位在前
	{
		DSIO = addr & 0x01;	// LSB低位开始传送
		addr >>= 1;			// 发完后addr右移一位原来的次低位变成新的低位
		SCLK = 1;			// 拉高SCLK制造一个上升沿，通知DS1302读取数据
		_nop_();			// 延时等待DS1302读取DSIO线上的1位数据

		SCLK = 0;			// 读完后，拉低SCLK为下一个位的传输做准备
		_nop_();
	}
							// 循环结束后1字节的寄存器地址传给DS1302
	for (i=0; i<8; i++)		// 循环写入8位数据，注意低位在前
	{
		DSIO = dat & 0x01;
		dat >>= 1;
		SCLK = 1;			//制造上升沿通知DS1302读取数据
		_nop_();
		SCLK = 0;
		_nop_();	
	}	   					// 循环结束后1字节值成功传给DS1302
		 
	CE = 0;					// 写入完毕拉低CE以禁止对DS1302的读写，防止意外改写事故
	_nop_();
}


u8 Ds1302ReadReg(u8 addr)
{
	u8 i = 0, dat = 0, dat1 = 0;

	CE = 0;		  					// CE设置为初始状态
	_nop_();

	SCLK = 0;						// SCLK设置为初始状态
	_nop_();

	CE = 1;							// 拉高CE使能对DS1302的寄存器读写权限
	_nop_();

	for (i=0; i<8; i++)				// 循环发送8bit寄存器地址值
	{
		DSIO = addr & 0x01;			// DS1302的SPI接口从LSB开始发送
		addr >>= 1;

		SCLK = 1;					// 制造上升沿，DS1302在上升沿完成读取动作
		_nop_();

		SCLK = 0;					// 拉低SCLK为下个bit发送做准备
		_nop_();
	}
	
	for (i=0; i<8; i++)				// 循环读取8bit寄存器值数据
	{
		dat1 = DSIO;//从最低位开始接收
		dat = (dat>>1) | (dat1<<7);
		SCLK = 1;
		_nop_();
		SCLK = 0;//DS1302下降沿时，放置数据
		_nop_();
	}

	CE = 0;
	_nop_();	//以下为DS1302复位的稳定时间,必须的。

	SCLK = 1;
	_nop_();

	DSIO = 0;
	_nop_();

	DSIO = 1;
	_nop_();

	return dat;	
}


void Ds1302WriteTime(void)
{
	u8 i = 0;
		
	/*Init_Time[0] = Ds1302ReadReg(READ_RAM_ADDR[1]);
	if (Init_Time[0] == 0xff)
	{
		;
	}
	else
	{*/		
		Ds1302WriteReg(0x8E,0x00);		 // 禁用WP以获取写时间寄存器的权限
		//Ds1302WriteReg(WRITE_RAM_ADDR[1], Init_Time[0]);
		for (i=0; i<7; i++)			 // 依次写入时间
		{
			Ds1302WriteReg(DS1302_WRITE_ADDR[i],Init_Time[i]);	
		}

		Ds1302WriteReg(0x8E,0x80);		 // 使能WP以禁止对时间寄存器的读写，防止意外改写事故
	//}
}


void Ds1302InitTime(void)
{
	//初始化时间
	Init_Time[0] = 0x00;	//秒
	Init_Time[1] = 0x00;	//分
	//Init_Time[2] = 0x23;	//时
	
	//写入时间
	Ds1302WriteTime();
}


void Ds1302ReadTime(void)
{
	u8 i = 0;

	for (i=0; i<2; i++)			//读取2个字节的时钟信号：秒分
	{
		gTime[i] = Ds1302ReadReg(DS1302_READ_ADDR[i]);
	}
}


void UpdateDigValue(void)
{
	Ds1302ReadTime();	  	// 从DS1302读取时间存入gTime中
 
	//gDigValue[0] = ((gTime[2] >> 4) & 0x0f);		// Hour高4位
	//gDigValue[1] = ((gTime[2] >> 0) & 0x0f);		// Hour低4位
	
	gDigValue[0] = ((gTime[1] >> 4) & 0x0f);		// Min高4位
	gDigValue[1] = ((gTime[1] >> 0) & 0x0f);		// Min低4位

	gDigValue[2] = ((gTime[0] >> 4) & 0x0f);		// Sec高4位
	gDigValue[3] = ((gTime[0] >> 0) & 0x0f);		// Sec低4位

}

void DigDisplayTime(void)
{
	acled[0] = gDuanMa[gDigValue[0]];
	acled[1] = gDuanMa[gDigValue[1]] | 0x80;
	acled[2] = gDuanMa[gDigValue[2]];
	acled[3] = gDuanMa[gDigValue[3]];
}

/*********************************************************************
* 
* 			                EEPROM部分
* 
*********************************************************************/

/*************关闭IAP功能子程序*****************************/
void IAP_Disable()      //关闭IAP 功能, 清相关的特殊功能寄存器,使CPU 处于安全状态,
{                       //一次连续的IAP 操作完成之后建议关闭IAP 功能,不需要每次都关    
	IAP_CONTR = 0;      //关闭IAP 功能
	IAP_CMD   = 0;      //清命令寄存器,使命令寄存器无命令,此句可不用
	IAP_TRIG = 0;       //清命令触发寄存器,使命令触发寄存器无触发,此句可不用
	IAP_ADDRH = 0;      //高八位地址清0
	IAP_ADDRL = 0;      //低八位地址清0
}
  
/**********EEPROM读一字节子程序***********************/
unsigned char Byte_Read(unsigned int add)      //读一字节，调用前需打开IAP 功能，入口:DPTR = 字节地址，返回:A = 读出字节
{
	IAP_DATA = 0x00;             //IAP数据寄存器清0
	IAP_CONTR = ENABLE_ISP;      //打开IAP 功能, 设置Flash 操作等待时间
	IAP_CMD = 0x01;              //IAP/ISP/EEPROM 字节读命令
  
	IAP_ADDRH = (unsigned char)(add>>8);    //设置目标单元地址的高8 位地址
	IAP_ADDRL = (unsigned char)(add&0xff);    //设置目标单元地址的低8 位地址
  
	EA = 0;
	IAP_TRIG = 0x5a;   //先送 46h,再送B9h 到ISP/IAP 触发寄存器,每次都需如此
	IAP_TRIG = 0xa5;   //送完 B9h 后，ISP/IAP 命令立即被触发起动
	_nop_();
	EA = 1;
	IAP_Disable(); //关闭IAP 功能, 清相关的特殊功能寄存器,使CPU 处于安全状态,
					//一次连续的IAP 操作完成之后建议关闭IAP 功能,不需要每次都关
	return (IAP_DATA);
}
  
  
/************EEPROM字节编程子程序**************************/
void Byte_Program(unsigned int add,unsigned char ch)  //字节编程，调用前需打开IAP 功能，入口:DPTR = 字节地址, A= 须编程字节的数据
ss{
	IAP_CONTR = ENABLE_ISP;         //打开 IAP 功能, 设置Flash 操作等待时间
	IAP_CMD = 0x02;                 //IAP/ISP/EEPROM 字节编程命令
  
  
	IAP_ADDRH = (unsigned char)(add>>8);    //设置目标单元地址的高8 位地址
	IAP_ADDRL = (unsigned char)(add&0xff);    //设置目标单元地址的低8 位地址
  
	IAP_DATA = ch;                  //要编程的数据先送进IAP_DATA 寄存器
	EA = 0;
	IAP_TRIG = 0x5a;   //先送 46h,再送B9h 到ISP/IAP 触发寄存器,每次都需如此
	IAP_TRIG = 0xa5;   //送完 B9h 后，ISP/IAP 命令立即被触发起动
	_nop_();
	EA = 1;
	IAP_Disable(); //关闭IAP 功能, 清相关的特殊功能寄存器,使CPU 处于安全状态,
					//一次连续的IAP 操作完成之后建议关闭IAP 功能,不需要每次都关
}
  
/*************EEPROM擦除扇区子程序**************************/
void Sector_Erase(unsigned int add)       //擦除扇区, 入口:DPTR = 扇区地址
{
	IAP_CONTR = ENABLE_ISP;         //打开IAP 功能, 设置Flash 操作等待时间
	IAP_CMD = 0x03;                 //IAP/ISP/EEPROM 扇区擦除命令
  
	IAP_ADDRH = (unsigned char)(add>>8);    //设置目标单元地址的高8 位地址
	IAP_ADDRL = (unsigned char)(add&0xff);    //设置目标单元地址的低8 位地址
  
	EA = 0;
	IAP_TRIG = 0x5a;   //先送 5ah,再送B9h 到ISP/IAP 触发寄存器,每次都需如此
	IAP_TRIG = 0xa5;   //送完 a5h 后，ISP/IAP 命令立即被触发起动
	_nop_();
	EA = 1;
	IAP_Disable();     //关闭IAP 功能, 清相关的特殊功能寄存器,使CPU 处于安全状态,
					   //一次连续的IAP 操作完成之后建议关闭IAP 功能,不需要每次都关
}

/*********************************************************************
* 
* 			                中断部分
* 
*********************************************************************/

void key(void)	interrupt 1
{	
	if (time++ == 5)
	{
		time = 0;
		
		DIG_PORT = 0x00;							/* 先清显示再换位选 */
		POS_PORT = Led[cScanIndex] | buzzer;		/* 送位选数据 */
		
		if (Set_Flag)	
		{
			switch (Set_Flag)
			{
				case 1:						
					DIG_PORT = Dis_F1[cScanIndex++];
					if (delay-- == 0)
					{				
						Set_Flag = 2;
						delay = 1000;
					
					}
					break;
				case 2:
					if (Full_Flag)
					{
						DIG_PORT = Dis_Full[cScanIndex++];
						if (delay-- == 0)
						{				
							Set_Flag = 0;
							delay = 1000;					
						}
					}
					else	Set_Flag = 3;
					break;
				case 3:
					Set24Time(Byte_Read(ROM_Length_Addr));
					DIG_PORT = Set_Time[cScanIndex++];
					break;					
			}			
		}
		
		else if (View_Flag)
		{
			switch (View_Flag)
			{
				case 1:						
					DIG_PORT = Dis_F2[cScanIndex++];
					if (delay-- == 0)
					{				
						View_Flag = 2;
						delay = 1000;
					
					}
					break;
				case 2:
					if (Empty_Flag)
					{
						DIG_PORT = Dis_Empty[cScanIndex++];
						if (delay-- == 0)
						{				
							View_Flag = 0;
							delay = 1000;					
						}
					}
					else	View_Flag = 3;
					break;
				case 3:
					View_Time(check); 		
					DIG_PORT = TimeViewing[cScanIndex++];
					break;					
			}
		}
		else	
		{
			DIG_PORT = acled[cScanIndex++];						
		}
		cScanIndex &= 3;				
		
	}

	/*********************************************************/
		
		if(nDelayKey==0)
		{
			cKey = P2 & 0x07;			// 取键值P10、P11、P12
			if(cKey != 0x07)nDelayKey=100;// 设置延迟时间削颤
			else
			{
				bStill=0;
				cLongDelay=0;
			}						// 松键
		}
		else		   					// 有按键利用DelayKey按键消颤
		{
			nDelayKey--;
			if(nDelayKey==0)
			{
				cKeyCode = P2 & 0x07;		// 取键值P10、P11、P12
				if(cKey != cKeyCode)
				{
					cKeyCode = 0;				
				}
			}
		}
	
}