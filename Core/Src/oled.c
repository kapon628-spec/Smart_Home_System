/**
 * 基于硬件IIC通信的OLED驱动程序
 * API完全打包好的直接选择你的硬件IIC口即可
 *
 */
#include "main.h"
#include "i2c.h"
#include "stdio.h"
#include "oled_font.h"
#include "oled.h"


/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
	 uint8_t *pData;
	 pData = &Command;
	 HAL_I2C_Mem_Write(&Hardware_IIC_No,0x78,0x00,I2C_MEMADD_SIZE_8BIT,pData,1,100);
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
	 uint8_t *pData;
	 pData = &Data;
	 HAL_I2C_Mem_Write(&Hardware_IIC_No,0x78,0x40,I2C_MEMADD_SIZE_8BIT,pData,1,100);

}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置低4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置高4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
 *  @brief OLED显示模拟量
 *  @param Line起始位置，范围0~3
 *  @param Colum 起始列位置，0~7
 *  @param CHN 汉字序列
 */
void OLED_ShowChinese(uint8_t Line, uint8_t Column, uint8_t CHN)
{
		uint8_t i;
		OLED_SetCursor(Line*2, Column*16);		//设置光标位置在上半部分
		for (i = 0; i < 16; i++)
		{
			OLED_WriteData(GBK[CHN][i]);			//显示上半部分内容
		}
		OLED_SetCursor((Line*2)+1,Column*16);	//设置光标位置在下半部分
		for (i = 0; i < 16; i++)
		{
			OLED_WriteData(GBK[CHN][i + 16]);		//显示下半部分内容
		}
}

/**
 *  @brief OLED显示模拟量
 *  @param Pic第几张图片
 *  @param Colum 起始列位置，0~7
 *
 */
void OLED_ShowPicture(uint8_t Pic)
{
	uint8_t i,j;
	Pic=Pic*8;
	for(j=0;j<8;j++)
	{
		OLED_SetCursor(j,0);//开始显示
		for (i = 0; i < 128; i++)
				{
					OLED_WriteData(Picture[j+Pic][i]);			//显示上半部分内容
				}
	}
}
/**
  * @brief  OLED初始化
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{

	HAL_Delay(200);
	OLED_WriteCommand(0xAE);	//关闭显示

	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);

	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);

	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);

	OLED_WriteCommand(0x40);	//设置显示开始行

	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置

	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);

	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示

	OLED_Clear();				//OLED清屏
}
/*
*  显示浮点数
*  @param Line起始位置，范围1~4
*  @param Colum 起始列位置，1~16
*  @Length_N   整数位
*  @Length_M   小数位
*/
void OLED_ShowFloatNum(uint8_t Line, uint8_t Column, float Num,uint8_t Length_N,uint8_t Length_M)
{

	uint32_t N,Num_zheng;
	float Num_f;

	N=1;

	Num_zheng=Num;
	OLED_ShowNum(Line, Column,Num_zheng,Length_N);
	OLED_ShowString(Line,Column+Length_N,".");

	Num_f=Num-Num_zheng;

	for(uint8_t i=0;i<Length_M;i++)
	{
		N=N*10;
	}
	Num_zheng=Num_f*N;
	OLED_ShowNum(Line, Column+Length_N+1,Num_zheng,Length_M);

}
void OLED_star(void)
{
	  OLED_ShowString(1,1,(char*) "0123456789ABCDEF");
	  OLED_ShowString(2,1,(char*) "1              F");
	  OLED_ShowString(3,1,(char*) "2              F");
	  OLED_ShowString(4,1,(char*) "3              F");
	  HAL_Delay(100);
	  OLED_Clear();
	  OLED_ShowChinese(0,0,0);//仪
	  OLED_ShowChinese(0,1,1);//电

	  OLED_ShowChinese(1,0,1);//电
	  OLED_ShowChinese(1,1,2);//压
	  OLED_ShowChinese(1,2,5);//值
	  OLED_ShowString(2,7,(char*) ":");

	  OLED_ShowChinese(2,0,3);//温
	  OLED_ShowChinese(2,1,4);//度
	  OLED_ShowChinese(2,2,5);//值
	  OLED_ShowString(3,7,(char*) ":");
	  OLED_ShowChinese(2,7,6);//℃

	  HAL_Delay(100);
	  OLED_Clear();
	  OLED_ShowPicture(0);
}
