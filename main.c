#include "STC12C5A60S2.H"
#include "intrins.h"


typedef unsigned char u8; 	
typedef unsigned int u16;

/***********����Flash �����ȴ�ʱ�估����IAP/ISP/EEPROM �����ĳ���(����IAP_CONTR�Ĵ���)***********/
#define ENABLE_ISP 0x81     //ʵ��  22M����ʹ��

/*����ܶ˿ڶ���*/
#define DIG_PORT 	P0
#define POS_PORT	P2      

// DS1302��ض���
sbit DSIO = P1^3; 		
sbit CE   = P1^4;		// ʹ�ܣ���д��Ҫ��ֹ
sbit SCLK = P1^2;	   	

/*���������*/
unsigned char code gDuanMa[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,
					0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71};

u8 Init_Time[7];			// �����洢DS1302��7��ʱ���BCD��ֵ
u8 gTime[7];			//����ʹ��
u8 gDigValue[8];		// ��SS̬���������ʾ��8����ֵ
					
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
	

// DS1302�ڲ�ʱ��Ĵ�����ַ�������壬��Щ��ֵַ�ɲ�DS1302�����ֲ��ȡ
u8 code DS1302_READ_ADDR[3] = {0x81, 0x83, 0x85}; 
u8 code DS1302_WRITE_ADDR[3] = {0x80, 0x82, 0x84};

u8 buzzer = 0x00;

u8 time = 0;

u8 code Led[]={0xe7,0xd7,0xb7,0x77};
u8 acled[4];
char cScanIndex;

u8  cKey;							/* ��ʼ��ֵ	*/
u8  cKeyCode;						/* ��ֵ	*/
u8  cLongDelay;						/* ��������ʱ��	*/
bit   bStill;						/* �Ƿ��ɼ���־ */

u16  nDelayKey;						/* ������ʱ������Ϊ��ʱ�жϼ��ʱ��������� */


/*
**********************************************************************
*                         ��������
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
	
	/*��ⰴ����ֵ*/
	switch(cKey)
	{
	/*���°���K1����������ģʽ*/
	case 6:										
		if(bStill==0)
		{
			Set_Flag = 1;		//���Ƚ�������ģʽ1����ʾ��F1��
			bStill = 1;

			/*�жϴ洢��ʱ�������Ƿ񳬹��������*/
			if (Byte_Read(ROM_Length_Addr) >= 24)
			{
				Full_Flag = 1;		//�Ѵ������������������ʾ��FULL�����˳�����
				break;
			}
			else Full_Flag = 0;		/*δ������ʼ����һ��ʱ������Ϊ0*/
			SZ_Sec[Byte_Read(ROM_Length_Addr)] = 0;	
			SZ_Min[Byte_Read(ROM_Length_Addr)] = 0;
			//bStill = 1;
			while (Set_Flag != 3);	//�ȴ����Խ�������ģʽ3�ı�־��������ְ�������
			/**************************/
			while (1)
			{	
				/*�ж��Ƿ��������*/
				if (F1_Over)
				{
					F1_Over = 0;
					Set_Flag = 0;					
					break;
				}
				switch (cKey)		//��ⰴ��
				{
					case 5:			//���°���K2���������1
						if (bStill == 0)
						{
							if (++SZ_Sec[Byte_Read(ROM_Length_Addr)] > 59)		SZ_Sec[Byte_Read(ROM_Length_Addr)] = 0;
							bStill = 1;
						}
						break;
					case 3:			//���°���K3�����÷ּ�1
						if (bStill == 0)
						{
							
							if (++SZ_Min[Byte_Read(ROM_Length_Addr)] > 59)		SZ_Min[Byte_Read(ROM_Length_Addr)] = 0;
							bStill = 1;							
						}
						break;
					case 6:			//���°���K1���˳����ã�����ʱ��洢��EEPROM��
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
									/*д���ٲ���������ôŪ*/
									/**��/
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
		
	/*���°���K2���������ģʽ*/	
	case 5:
		if(bStill == 0)
		{
			View_Flag = 1;		//���Ƚ������ģʽ1����ʾ��F2��
			bStill = 1;
			/*�жϴ洢��ʱ�������Ƿ�Ϊ��*/
			if (Byte_Read(ROM_Length_Addr) == 0)
			{
				Empty_Flag = 1;		//�գ���ʾ��EP����empty
				break;
			}
			else Empty_Flag = 0;	/*δ��*/
			while (View_Flag != 3);		//�ȴ����Խ������ģʽ3�ı�־��������ְ�������
			while (1)
			{		
				/*�ж��Ƿ�������*/
				if (F2_Over)
				{
					F2_Over = 0;
					View_Flag = 0;
					break;
				}
				switch (cKey)		//��ⰴ��
				{
					case 5:			//���°���K2���鿴��һ������
						if (bStill == 0)
						{
							if (++check > Byte_Read(ROM_Length_Addr) - 1)	check = 0;
							bStill = 1;
						}
						break;		
					case 3:			//���°���K3���鿴��һ������
						if (bStill == 0)
						{							
							if (--check > 40)	check = Byte_Read(ROM_Length_Addr) - 1;
							bStill = 1;
						}
						break;
					case 6:			//���°���K1���˳����ģʽ
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
		Ds1302InitTime();		//DS1302��ʼ��ʱ��
		//���һ�Σ������M�룿
		//�ϵ�
		//��ʼ��0��
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
	
	P0M1=0x00; //����P0��Ϊ�������
	P0M0=0xff;
	
	P2M1=0x00; //����P2��Ϊ�������
	P2M0=0xff;
	
	Init0();	//�жϳ�ʼ��
	
	/*�ж�EEPROM��ʱ�����������Ƿ��ʼ��*/
	if (Byte_Read(ROM_Length_Addr) == 0xff)
	{
		/*û�г�ʼ������ʼ��Ϊ0*/
		Sector_Erase(ROM_Length_Addr);
		Byte_Program(ROM_Length_Addr, 0x00);
	}
	Ds1302InitTime();		//DS1302��ʼ��ʱ��
	
	while (1)
	{
		/*����ʱ�䣬����ʾ���������*/
		UpdateDigValue();
		DigDisplayTime();
		
		/*�����������õ����д���ʱ��*/
		for (i = 0 ; i < Byte_Read(ROM_Length_Addr) ; i++)
		{
			/*�жϵ�ǰʱ���Ƿ������õĴ���ʱ�����*/
			if (Ds1302ReadReg(DS1302_READ_ADDR[1]) == D2BCD(Byte_Read(ROM_Min_Addr + i)) &&	
				Ds1302ReadReg(DS1302_READ_ADDR[0]) == D2BCD(Byte_Read(ROM_Sec_Addr + i)))
			{
				/*������򿪿��Ʒ������Ŀ���*/
				on_flag = 1;
				break;
			}
			/*�������رշ�����*/
			on_flag = 0;
			
		}
		
		/*�жϷ�������־λ�Ƿ��*/
		if (on_flag)
		{
			/*��������*/
			buzzer = on;
		}		
		else
		{
			/*������Ϩ*/
			buzzer = off;
		}
		
		
		if(cKey != 0x07)
		{
			DisposeKEY();	// ��Ӧ��������     
		}						
	}
}

void Init0(void)
{
	TMOD = 0x02;		// T0��Ϊ��ʱ��ģʽ������TR0���ƴ�

	TH0 = -200;	
	//TL0 = -200;			// 
	ET0 = 1;			// �򿪶�ʱ��0�ж�����
	EA = 1;				// �����ж�
	TR0 = 1;			// ��ʼ��ʱ
}

void Ds1302WriteReg(u8 addr, u8 dat)
{
	u8 i = 0;

	CE = 0;
	_nop_();				// ��ʱ

	SCLK = 0;				// ��SCLK�õ�
	_nop_();

	CE = 1; 				// CE������ʹ��
	_nop_();

	for (i=0; i<8; i++)		// ѭ����λ����addr��8��bit����λ��ǰ
	{
		DSIO = addr & 0x01;	// LSB��λ��ʼ����
		addr >>= 1;			// �����addr����һλԭ���Ĵε�λ����µĵ�λ
		SCLK = 1;			// ����SCLK����һ�������أ�֪ͨDS1302��ȡ����
		_nop_();			// ��ʱ�ȴ�DS1302��ȡDSIO���ϵ�1λ����

		SCLK = 0;			// ���������SCLKΪ��һ��λ�Ĵ�����׼��
		_nop_();
	}
							// ѭ��������1�ֽڵļĴ�����ַ����DS1302
	for (i=0; i<8; i++)		// ѭ��д��8λ���ݣ�ע���λ��ǰ
	{
		DSIO = dat & 0x01;
		dat >>= 1;
		SCLK = 1;			//����������֪ͨDS1302��ȡ����
		_nop_();
		SCLK = 0;
		_nop_();	
	}	   					// ѭ��������1�ֽ�ֵ�ɹ�����DS1302
		 
	CE = 0;					// д���������CE�Խ�ֹ��DS1302�Ķ�д����ֹ�����д�¹�
	_nop_();
}


u8 Ds1302ReadReg(u8 addr)
{
	u8 i = 0, dat = 0, dat1 = 0;

	CE = 0;		  					// CE����Ϊ��ʼ״̬
	_nop_();

	SCLK = 0;						// SCLK����Ϊ��ʼ״̬
	_nop_();

	CE = 1;							// ����CEʹ�ܶ�DS1302�ļĴ�����дȨ��
	_nop_();

	for (i=0; i<8; i++)				// ѭ������8bit�Ĵ�����ֵַ
	{
		DSIO = addr & 0x01;			// DS1302��SPI�ӿڴ�LSB��ʼ����
		addr >>= 1;

		SCLK = 1;					// ���������أ�DS1302����������ɶ�ȡ����
		_nop_();

		SCLK = 0;					// ����SCLKΪ�¸�bit������׼��
		_nop_();
	}
	
	for (i=0; i<8; i++)				// ѭ����ȡ8bit�Ĵ���ֵ����
	{
		dat1 = DSIO;//�����λ��ʼ����
		dat = (dat>>1) | (dat1<<7);
		SCLK = 1;
		_nop_();
		SCLK = 0;//DS1302�½���ʱ����������
		_nop_();
	}

	CE = 0;
	_nop_();	//����ΪDS1302��λ���ȶ�ʱ��,����ġ�

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
		Ds1302WriteReg(0x8E,0x00);		 // ����WP�Ի�ȡдʱ��Ĵ�����Ȩ��
		//Ds1302WriteReg(WRITE_RAM_ADDR[1], Init_Time[0]);
		for (i=0; i<7; i++)			 // ����д��ʱ��
		{
			Ds1302WriteReg(DS1302_WRITE_ADDR[i],Init_Time[i]);	
		}

		Ds1302WriteReg(0x8E,0x80);		 // ʹ��WP�Խ�ֹ��ʱ��Ĵ����Ķ�д����ֹ�����д�¹�
	//}
}


void Ds1302InitTime(void)
{
	//��ʼ��ʱ��
	Init_Time[0] = 0x00;	//��
	Init_Time[1] = 0x00;	//��
	//Init_Time[2] = 0x23;	//ʱ
	
	//д��ʱ��
	Ds1302WriteTime();
}


void Ds1302ReadTime(void)
{
	u8 i = 0;

	for (i=0; i<2; i++)			//��ȡ2���ֽڵ�ʱ���źţ����
	{
		gTime[i] = Ds1302ReadReg(DS1302_READ_ADDR[i]);
	}
}


void UpdateDigValue(void)
{
	Ds1302ReadTime();	  	// ��DS1302��ȡʱ�����gTime��
 
	//gDigValue[0] = ((gTime[2] >> 4) & 0x0f);		// Hour��4λ
	//gDigValue[1] = ((gTime[2] >> 0) & 0x0f);		// Hour��4λ
	
	gDigValue[0] = ((gTime[1] >> 4) & 0x0f);		// Min��4λ
	gDigValue[1] = ((gTime[1] >> 0) & 0x0f);		// Min��4λ

	gDigValue[2] = ((gTime[0] >> 4) & 0x0f);		// Sec��4λ
	gDigValue[3] = ((gTime[0] >> 0) & 0x0f);		// Sec��4λ

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
* 			                EEPROM����
* 
*********************************************************************/

/*************�ر�IAP�����ӳ���*****************************/
void IAP_Disable()      //�ر�IAP ����, ����ص����⹦�ܼĴ���,ʹCPU ���ڰ�ȫ״̬,
{                       //һ��������IAP �������֮����ر�IAP ����,����Ҫÿ�ζ���    
	IAP_CONTR = 0;      //�ر�IAP ����
	IAP_CMD   = 0;      //������Ĵ���,ʹ����Ĵ���������,�˾�ɲ���
	IAP_TRIG = 0;       //��������Ĵ���,ʹ������Ĵ����޴���,�˾�ɲ���
	IAP_ADDRH = 0;      //�߰�λ��ַ��0
	IAP_ADDRL = 0;      //�Ͱ�λ��ַ��0
}
  
/**********EEPROM��һ�ֽ��ӳ���***********************/
unsigned char Byte_Read(unsigned int add)      //��һ�ֽڣ�����ǰ���IAP ���ܣ����:DPTR = �ֽڵ�ַ������:A = �����ֽ�
{
	IAP_DATA = 0x00;             //IAP���ݼĴ�����0
	IAP_CONTR = ENABLE_ISP;      //��IAP ����, ����Flash �����ȴ�ʱ��
	IAP_CMD = 0x01;              //IAP/ISP/EEPROM �ֽڶ�����
  
	IAP_ADDRH = (unsigned char)(add>>8);    //����Ŀ�굥Ԫ��ַ�ĸ�8 λ��ַ
	IAP_ADDRL = (unsigned char)(add&0xff);    //����Ŀ�굥Ԫ��ַ�ĵ�8 λ��ַ
  
	EA = 0;
	IAP_TRIG = 0x5a;   //���� 46h,����B9h ��ISP/IAP �����Ĵ���,ÿ�ζ������
	IAP_TRIG = 0xa5;   //���� B9h ��ISP/IAP ����������������
	_nop_();
	EA = 1;
	IAP_Disable(); //�ر�IAP ����, ����ص����⹦�ܼĴ���,ʹCPU ���ڰ�ȫ״̬,
					//һ��������IAP �������֮����ر�IAP ����,����Ҫÿ�ζ���
	return (IAP_DATA);
}
  
  
/************EEPROM�ֽڱ���ӳ���**************************/
void Byte_Program(unsigned int add,unsigned char ch)  //�ֽڱ�̣�����ǰ���IAP ���ܣ����:DPTR = �ֽڵ�ַ, A= �����ֽڵ�����
ss{
	IAP_CONTR = ENABLE_ISP;         //�� IAP ����, ����Flash �����ȴ�ʱ��
	IAP_CMD = 0x02;                 //IAP/ISP/EEPROM �ֽڱ������
  
  
	IAP_ADDRH = (unsigned char)(add>>8);    //����Ŀ�굥Ԫ��ַ�ĸ�8 λ��ַ
	IAP_ADDRL = (unsigned char)(add&0xff);    //����Ŀ�굥Ԫ��ַ�ĵ�8 λ��ַ
  
	IAP_DATA = ch;                  //Ҫ��̵��������ͽ�IAP_DATA �Ĵ���
	EA = 0;
	IAP_TRIG = 0x5a;   //���� 46h,����B9h ��ISP/IAP �����Ĵ���,ÿ�ζ������
	IAP_TRIG = 0xa5;   //���� B9h ��ISP/IAP ����������������
	_nop_();
	EA = 1;
	IAP_Disable(); //�ر�IAP ����, ����ص����⹦�ܼĴ���,ʹCPU ���ڰ�ȫ״̬,
					//һ��������IAP �������֮����ر�IAP ����,����Ҫÿ�ζ���
}
  
/*************EEPROM���������ӳ���**************************/
void Sector_Erase(unsigned int add)       //��������, ���:DPTR = ������ַ
{
	IAP_CONTR = ENABLE_ISP;         //��IAP ����, ����Flash �����ȴ�ʱ��
	IAP_CMD = 0x03;                 //IAP/ISP/EEPROM ������������
  
	IAP_ADDRH = (unsigned char)(add>>8);    //����Ŀ�굥Ԫ��ַ�ĸ�8 λ��ַ
	IAP_ADDRL = (unsigned char)(add&0xff);    //����Ŀ�굥Ԫ��ַ�ĵ�8 λ��ַ
  
	EA = 0;
	IAP_TRIG = 0x5a;   //���� 5ah,����B9h ��ISP/IAP �����Ĵ���,ÿ�ζ������
	IAP_TRIG = 0xa5;   //���� a5h ��ISP/IAP ����������������
	_nop_();
	EA = 1;
	IAP_Disable();     //�ر�IAP ����, ����ص����⹦�ܼĴ���,ʹCPU ���ڰ�ȫ״̬,
					   //һ��������IAP �������֮����ر�IAP ����,����Ҫÿ�ζ���
}

/*********************************************************************
* 
* 			                �жϲ���
* 
*********************************************************************/

void key(void)	interrupt 1
{	
	if (time++ == 5)
	{
		time = 0;
		
		DIG_PORT = 0x00;							/* ������ʾ�ٻ�λѡ */
		POS_PORT = Led[cScanIndex] | buzzer;		/* ��λѡ���� */
		
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
			cKey = P2 & 0x07;			// ȡ��ֵP10��P11��P12
			if(cKey != 0x07)nDelayKey=100;// �����ӳ�ʱ������
			else
			{
				bStill=0;
				cLongDelay=0;
			}						// �ɼ�
		}
		else		   					// �а�������DelayKey��������
		{
			nDelayKey--;
			if(nDelayKey==0)
			{
				cKeyCode = P2 & 0x07;		// ȡ��ֵP10��P11��P12
				if(cKey != cKeyCode)
				{
					cKeyCode = 0;				
				}
			}
		}
	
}