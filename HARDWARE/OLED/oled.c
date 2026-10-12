#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"
#include "delay.h"
#include "ds1302.h"
#include "stdio.h"
#include "dht11.h"
#include "led.h"
u8 OLED_GRAM[144][8];

//反显函数
void OLED_ColorTurn(u8 i)
{
	if(i==0)
		{
			OLED_WR_Byte(0xA6,OLED_CMD);//正常显示
		}
	if(i==1)
		{
			OLED_WR_Byte(0xA7,OLED_CMD);//反色显示
		}
}

//屏幕旋转180度
void OLED_DisplayTurn(u8 i)
{
	if(i==0)
		{
			OLED_WR_Byte(0xC8,OLED_CMD);//正常显示
			OLED_WR_Byte(0xA1,OLED_CMD);
		}
	if(i==1)
		{
			OLED_WR_Byte(0xC0,OLED_CMD);//反转显示
			OLED_WR_Byte(0xA0,OLED_CMD);
		}
}

//延时
void IIC_delay(void)
{
	u8 t=3;
	while(t--);
}

//起始信号
void OLED_I2C_Start(void)
{
	OLED_SDA_Set();
	OLED_SCL_Set();
	IIC_delay();
	OLED_SDA_Clr();
	IIC_delay();
	OLED_SCL_Clr();
	IIC_delay();
}

//结束信号
void OLED_I2C_Stop(void)
{
	OLED_SDA_Clr();
	OLED_SCL_Set();
	IIC_delay();
	OLED_SDA_Set();
}

//等待信号响应
void OLED_I2C_WaitAck(void) //测数据信号的电平
{
	OLED_SDA_Set();
	IIC_delay();
	OLED_SCL_Set();
	IIC_delay();
	OLED_SCL_Clr();
	IIC_delay();
}

//写入一个字节
void OLED_Send_Byte(u8 dat)
{
	u8 i;
	for(i=0;i<8;i++)
	{
		if(dat&0x80)//将dat的8位从最高位依次写入
		{
			OLED_SDA_Set();
    }
		else
		{
			OLED_SDA_Clr();
    }
		IIC_delay();
		OLED_SCL_Set();
		IIC_delay();
		OLED_SCL_Clr();//将时钟信号设置为低电平
		dat<<=1;
  }
}

//发送一个字节
//mode:数据/命令标志 0,表示命令;1,表示数据;
void OLED_WR_Byte(u8 dat,u8 mode)
{
	OLED_I2C_Start();
	OLED_Send_Byte(0x78);
	OLED_I2C_WaitAck();
	if(mode){OLED_Send_Byte(0x40);}
  else{OLED_Send_Byte(0x00);}
	OLED_I2C_WaitAck();
	OLED_Send_Byte(dat);
	OLED_I2C_WaitAck();
	OLED_I2C_Stop();
}

//开启OLED显示 
void OLED_DisPlay_On(void)
{
	OLED_WR_Byte(0x8D,OLED_CMD);//电荷泵使能
	OLED_WR_Byte(0x14,OLED_CMD);//开启电荷泵
	OLED_WR_Byte(0xAF,OLED_CMD);//点亮屏幕
}

//关闭OLED显示 
void OLED_DisPlay_Off(void)
{
	OLED_WR_Byte(0x8D,OLED_CMD);//电荷泵使能
	OLED_WR_Byte(0x10,OLED_CMD);//关闭电荷泵
	OLED_WR_Byte(0xAE,OLED_CMD);//关闭屏幕
}

//更新显存到OLED	
void OLED_Refresh(void)
{
	u8 i,n;
	for(i=0;i<8;i++)
	{
		OLED_WR_Byte(0xb0+i,OLED_CMD); //设置行起始地址
		OLED_WR_Byte(0x00,OLED_CMD);   //设置低列起始地址
		OLED_WR_Byte(0x10,OLED_CMD);   //设置高列起始地址
		OLED_I2C_Start();
		OLED_Send_Byte(0x78);
		OLED_I2C_WaitAck();
		OLED_Send_Byte(0x40);
		OLED_I2C_WaitAck();
		for(n=0;n<128;n++)
		{
			OLED_Send_Byte(OLED_GRAM[n][i]);
			OLED_I2C_WaitAck();
		}
		OLED_I2C_Stop();
  }
}

// 按像素坐标刷新矩形区域
void OLED_Refresh_Rect(u16 x, u16 y, u16 w, u16 h)
{
    u8 i, j;
    u8 page_start = y / 8;
    u8 page_end = (y + h - 1) / 8;
    u8 page_offset = y % 8;  // 页内偏移
    
    // 边界保护
    if(x > 127) return;
    if(y > 63) return;
    if(x + w > 128) w = 128 - x;
    if(y + h > 64) h = 64 - y;
    
    for(i = page_start; i <= page_end; i++)
    {
        // 设置页地址
        OLED_WR_Byte(0xb0 + i, OLED_CMD);
        
        // 设置列起始地址
        OLED_WR_Byte(x & 0x0f, OLED_CMD);
        OLED_WR_Byte(0x10 + (x >> 4), OLED_CMD);
        
        // 开始I2C传输
        OLED_I2C_Start();
        OLED_Send_Byte(0x78);
        OLED_I2C_WaitAck();
        OLED_Send_Byte(0x40);
        OLED_I2C_WaitAck();
        
        // 发送指定列的显存数据
        for(j = x; j < x + w; j++)
        {
            if(page_offset == 0 || (i > page_start && i < page_end))
            {
                // 完整页或中间页，直接发送
                OLED_Send_Byte(OLED_GRAM[j][i]);
            }
            else if(i == page_start && page_offset != 0)
            {
                // 第一页，需要屏蔽低 page_offset 位
                u8 data = OLED_GRAM[j][i] >> page_offset;
                OLED_Send_Byte(data << page_offset);
            }
            else if(i == page_end && page_offset != 0)
            {
                // 最后一页，需要屏蔽高 (8-page_offset) 位
                u8 data = OLED_GRAM[j][i] << (8 - page_offset);
                OLED_Send_Byte(data >> (8 - page_offset));
            }
            else
            {
                OLED_Send_Byte(OLED_GRAM[j][i]);
            }
            OLED_I2C_WaitAck();
        }
        
        OLED_I2C_Stop();
    }
}

//清屏函数
void OLED_Clear(void)
{
	u8 i,n;
	for(i=0;i<8;i++)
	{
	   for(n=0;n<128;n++)
			{
			 OLED_GRAM[n][i]=0;//清除所有数据
			}
  }
	OLED_Refresh();//更新显示
}

//画点 
//x:0~127
//y:0~63
//t:1 填充 0,清空	
void OLED_DrawPoint(u8 x,u8 y,u8 t)
{
	u8 i,m,n;
	i=y/8;
	m=y%8;
	n=1<<m;
	if(t){OLED_GRAM[x][i]|=n;}
	else
	{
		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
		OLED_GRAM[x][i]|=n;
		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
	}
//	OLED_Refresh();
}

//画线
//x1,y1:起点坐标
//x2,y2:结束坐标
void OLED_DrawLine(u8 x1,u8 y1,u8 x2,u8 y2,u8 mode)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1;
	uRow=x1;//画线起点坐标
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向 
	else if (delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//水平线 
	else {incy=-1;delta_y=-delta_x;}
	if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		OLED_DrawPoint(uRow,uCol,mode);//画点
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
//	OLED_Refresh();
}
//x,y:圆心坐标
//r:圆的半径
void OLED_DrawCircle(u8 x,u8 y,u8 r)
{
	int a, b,num;
    a = 0;
    b = r;
    while(2 * b * b >= r * r)      
    {
        OLED_DrawPoint(x + a, y - b,1);
        OLED_DrawPoint(x - a, y - b,1);
        OLED_DrawPoint(x - a, y + b,1);
        OLED_DrawPoint(x + a, y + b,1);
 
        OLED_DrawPoint(x + b, y + a,1);
        OLED_DrawPoint(x + b, y - a,1);
        OLED_DrawPoint(x - b, y - a,1);
        OLED_DrawPoint(x - b, y + a,1);
        
        a++;
        num = (a * a + b * b) - r*r;//计算画的点离圆心的距离
        if(num > 0)
        {
            b--;
            a--;
        }
    }
//		OLED_Refresh();
}



//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//size1:选择字体 6x8/6x12/8x16/12x24
//mode:0,反色显示;1,正常显示
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1,u8 mode)
{
	u8 i,m,temp,size2,chr1;
	u8 x0=x,y0=y;
	if(size1==8)size2=6;
	else size2=(size1/8+((size1%8)?1:0))*(size1/2);  //得到字体一个字符对应点阵集所占的字节数
	chr1=chr-' ';  //计算偏移后的值
	for(i=0;i<size2;i++)
	{
		if(size1==8)
		{temp=asc2_0806[chr1][i];} //调用0806字体
		else if(size1==12)
        {temp=asc2_1206[chr1][i];} //调用1206字体
		else if(size1==16)
        {temp=asc2_1608[chr1][i];} //调用1608字体
		else if(size1==24)
        {temp=asc2_2412[chr1][i];} //调用2412字体
		else return;
		for(m=0;m<8;m++)
		{
			if(temp&0x01)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp>>=1;
			y++;
		}
		x++;
		if((size1!=8)&&((x-x0)==size1/2))
		{x=x0;y0=y0+8;}
		y=y0;
  }
	//OLED_Refresh();
}


//显示字符串
//x,y:起点坐标  
//size1:字体大小 
//*chr:字符串起始地址 
//mode:0,反色显示;1,正常显示
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 size1,u8 mode)
{
	while((*chr>=' ')&&(*chr<='~'))//判断是不是非法字符!
	{
		OLED_ShowChar(x,y,*chr,size1,mode);
		if(size1==8)x+=6;
		else x+=size1/2;
		chr++;
  }
	//OLED_Refresh();
}

//m^n
u32 OLED_Pow(u8 m,u8 n)
{
	u32 result=1;
	while(n--)
	{
	  result*=m;
	}
	return result;
}

//显示数字
//x,y :起点坐标
//num :要显示的数字
//len :数字的位数
//size:字体大小
//mode:0,反色显示;1,正常显示
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size1,u8 mode)
{
	u8 t,temp,m=0;
	if(size1==8)m=2;
	for(t=0;t<len;t++)
	{
		temp=(num/OLED_Pow(10,len-t-1))%10;
			if(temp==0)
			{
				OLED_ShowChar(x+(size1/2+m)*t,y,'0',size1,mode);
      }
			else 
			{
			  OLED_ShowChar(x+(size1/2+m)*t,y,temp+'0',size1,mode);
			}
  }
	//OLED_Refresh();
}

//显示汉字
//x,y:起点坐标
//num:汉字对应的序号
//mode:0,反色显示;1,正常显示
void OLED_ShowChinese(u8 x,u8 y,u8 num,u8 size1,u8 mode)
{
	u8 m,temp;
	u8 x0=x,y0=y;
	u16 i,size3=(size1/8+((size1%8)?1:0))*size1;  //得到字体一个字符对应点阵集所占的字节数
	for(i=0;i<size3;i++)
	{
		if(size1==16)
				{temp=Hzk1[num][i];}//调用16*16字体
		else if(size1==24)
				{temp=Hzk2[num][i];}//调用24*24字体
		else if(size1==32)       
				{temp=Hzk3[num][i];}//调用32*32字体
		else if(size1==64)
				{temp=Hzk4[num][i];}//调用64*64字体
		else return;
		for(m=0;m<8;m++)
		{
			if(temp&0x01)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp>>=1;
			y++;
		}
		x++;
		if((x-x0)==size1)
		{x=x0;y0=y0+8;}
		y=y0;
	}
	//OLED_Refresh();
}

//num 显示汉字的个数
//space 每一遍显示的间隔
//mode:0,反色显示;1,正常显示
void OLED_ScrollDisplay(u8 num,u8 space,u8 mode)
{
	u8 i,n,t=0,m=0,r;
	while(1)
	{
		if(m==0)
		{
	    OLED_ShowChinese(128,24,t,16,mode); //写入一个汉字保存在OLED_GRAM[][]数组中
			t++;
		}
		if(t==num)
			{
				for(r=0;r<16*space;r++)      //显示间隔
				 {
					for(i=1;i<144;i++)
						{
							for(n=0;n<8;n++)
							{
								OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
							}
						}
           OLED_Refresh();
				 }
        t=0;
      }
		m++;
		if(m==16){m=0;}
		for(i=1;i<144;i++)   //实现左移
		{
			for(n=0;n<8;n++)
			{
				OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
			}
		}
		OLED_Refresh();
	}
}

//x,y：起点坐标
//sizex,sizey,图片长宽
//BMP[]：要写入的图片数组
//mode:0,反色显示;1,正常显示
void OLED_ShowPicture(u8 x,u8 y,u8 sizex,u8 sizey,u8 BMP[],u8 mode)
{
	u16 j=0;
	u8 i,n,temp,m;
	u8 x0=x,y0=y;
	sizey=sizey/8+((sizey%8)?1:0);
	for(n=0;n<sizey;n++)
	{
		 for(i=0;i<sizex;i++)
		 {
				temp=BMP[j];
				j++;
				for(m=0;m<8;m++)
				{
					if(temp&0x01)OLED_DrawPoint(x,y,mode);
					else OLED_DrawPoint(x,y,!mode);
					temp>>=1;
					y++;
				}
				x++;
				if((x-x0)==sizex)
				{
					x=x0;
					y0=y0+8;
				}
				y=y0;
     }
	 }
	OLED_Refresh();
}
//OLED的初始化
void OLED_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(OLED_SCL_GPIO_CLK|OLED_SDA_GPIO_CLK, ENABLE);	 //使能A端口时钟
	GPIO_InitStructure.GPIO_Pin = OLED_SCL_PIN;	 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 		 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
 	GPIO_Init(OLED_SCL_PROT, &GPIO_InitStructure);	  //初始化PA0,1
 	GPIO_SetBits(OLED_SCL_PROT,OLED_SCL_PIN);
	
	GPIO_InitStructure.GPIO_Pin = OLED_SDA_PIN;	
	GPIO_Init(OLED_SDA_PROT, &GPIO_InitStructure);	  //初始化PA0,1
 	GPIO_SetBits(OLED_SDA_PROT,OLED_SDA_PIN);

	delay_ms(200);
	
	OLED_WR_Byte(0xAE,OLED_CMD);//--turn off oled panel
	OLED_WR_Byte(0x00,OLED_CMD);//---set low column address
	OLED_WR_Byte(0x10,OLED_CMD);//---set high column address
	OLED_WR_Byte(0x40,OLED_CMD);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	OLED_WR_Byte(0x81,OLED_CMD);//--set contrast control register
	OLED_WR_Byte(0xCF,OLED_CMD);// Set SEG Output Current Brightness
	OLED_WR_Byte(0xA1,OLED_CMD);//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	OLED_WR_Byte(0xC8,OLED_CMD);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	OLED_WR_Byte(0xA6,OLED_CMD);//--set normal display
	OLED_WR_Byte(0xA8,OLED_CMD);//--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x3f,OLED_CMD);//--1/64 duty
	OLED_WR_Byte(0xD3,OLED_CMD);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WR_Byte(0x00,OLED_CMD);//-not offset
	OLED_WR_Byte(0xd5,OLED_CMD);//--set display clock divide ratio/oscillator frequency
	OLED_WR_Byte(0x80,OLED_CMD);//--set divide ratio, Set Clock as 100 Frames/Sec
	OLED_WR_Byte(0xD9,OLED_CMD);//--set pre-charge period
	OLED_WR_Byte(0xF1,OLED_CMD);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	OLED_WR_Byte(0xDA,OLED_CMD);//--set com pins hardware configuration
	OLED_WR_Byte(0x12,OLED_CMD);
	OLED_WR_Byte(0xDB,OLED_CMD);//--set vcomh
	OLED_WR_Byte(0x30,OLED_CMD);//Set VCOM Deselect Level
	OLED_WR_Byte(0x20,OLED_CMD);//-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WR_Byte(0x02,OLED_CMD);//
	OLED_WR_Byte(0x8D,OLED_CMD);//--set Charge Pump enable/disable
	OLED_WR_Byte(0x14,OLED_CMD);//--set(0x10) disable
	OLED_Clear();
	OLED_WR_Byte(0xAF,OLED_CMD);
	
	
}


// ===================== 时间显示封装函数 =====================

extern struct TIMEData TimeData;//全局变量

void OLED_Show_Time(u8* st,u8 time_index,u8 mode)
{
	DS1302_read_realTime();
	if(mode == 0)
	{
		OLED_ShowChinese(0,32,12,16,1); //温
		OLED_ShowChinese(16,32,14,16,1);//度
		OLED_ShowChar(32,32,':',16,1);	//:
		OLED_ShowChinese(64, 32,15,16,1);	//C
		
		//显示“湿度：”
		OLED_ShowChinese(0,48,13,16,1); //湿
		OLED_ShowChinese(16,48,14,16,1);//度
		OLED_ShowChar(32,48,':',16,1);	//:
		OLED_ShowChar(64,48 ,'%',16,1);	//%
	}
	// 显示星期
	OLED_ShowChinese(0,16,0,16,1);	// 星
	OLED_ShowChinese(16,16,1,16,1);	// 期

	

	// 显示年月日
	sprintf((char*)st,"%d  ",TimeData.year);
	OLED_ShowString(8,0,st,16,time_index!=1);
	OLED_ShowChinese(40,0,9,16,1);	// 年
	
	sprintf((char*)st,"%d  ",TimeData.month);
	if(TimeData.month/10==0)
	{
		OLED_ShowString(56,0,"0",16,time_index!=2);
		
		OLED_ShowString(64,0,st,16,time_index!=2);
		
	}
	else
	{
		OLED_ShowString(56,0,st,16,time_index!=2);
	}
	
	OLED_ShowChinese(72,0,10,16,1);	// 月
	
	sprintf((char*)st,"%d  ",TimeData.day);
	if(TimeData.day/10==0)
	{
		OLED_ShowString(88,0,"0",16,time_index!=3);
		OLED_ShowString(96,0,st,16,time_index!=3);
	}
	else
	{
		OLED_ShowString(88,0,st,16,time_index!=3);
	}
	OLED_ShowChinese(104,0,11,16,1);// 日
	
	// 显示时分秒
	sprintf((char*)st,"%d  ",TimeData.hour);
	if(TimeData.hour/10==0)
	{
		OLED_ShowString(48+8,16,"0",16,time_index!=5);
		OLED_ShowString(48+8+8,16,st,16,time_index!=5);

	}
	else
	{
		OLED_ShowString(48+8,16,st,16,time_index!=5);
	}
	OLED_ShowString(48+8+16,16,":",16,1);//时
	
	sprintf((char*)st,"%d  ",TimeData.minute);
	if(TimeData.minute/10==0)
	{
		OLED_ShowString(48+8+16+8,16,"0",16,time_index!=6);
		OLED_ShowString(48+8+16+8+8,16,st,16,time_index!=6);
	}
	else
	{
		OLED_ShowString(48+8+16+8,16,st,16,time_index!=6);
	}
	OLED_ShowString(48+8+16+8+8+8,16,":",16,1);//分
	
	if(TimeData.second>=10)sprintf((char*)st,"%d",TimeData.second);
	else sprintf((char*)st,"%d ",TimeData.second);
	OLED_ShowString(48+8+16+16+16,16,st,16,time_index!=7);
	

	switch(TimeData.week)
	{
		case 1: OLED_ShowChinese(32,16,2,16,time_index!=4); break;
		case 2: OLED_ShowChinese(32,16,3,16,time_index!=4); break;
		case 3: OLED_ShowChinese(32,16,4,16,time_index!=4); break;
		case 4: OLED_ShowChinese(32,16,5,16,time_index!=4); break;
		case 5: OLED_ShowChinese(32,16,6,16,time_index!=4); break;
		case 6: OLED_ShowChinese(32,16,7,16,time_index!=4); break;
		case 7: OLED_ShowChinese(32,16,8,16,time_index!=4); break;
	}
}


extern u8 DDD;
extern u8 temp;
extern u8 humi;

void OLED_Show_DHT(void)
{
	if(DDD >= 2)
	{
		DDD = 0;
		DHT11_Read_Data(&temp, &humi);
		OLED_ShowNum(48,32,temp,2,16,1);
		//显示湿度数据
		OLED_ShowNum(48,48,humi,2,16,1);
	}
}


//num:0:蒙德1:璃月2:稻妻3:至冬
//orien_flag	1:终点至冬	2:终点蒙德
//蒙德start==36
//稻妻start==45
void OLED_Show_ADDRESS(u8 cur, u8 next)
{
	// 显示 当前站
	for(int i=0;i<3;i++)
	{
		OLED_ShowChinese(0 + i*16, 16, 36 + cur*3 + i, 16, 1);
	}

	// 显示箭头 =>
	OLED_ShowChar(48, 16, '=', 16, 1);
	OLED_ShowChar(56, 16, '=', 16, 1);
	OLED_ShowChar(64, 16, '>', 16, 1);
	OLED_ShowChar(72, 16, '>', 16, 1);

	// 显示 下一站
	for(int i=0;i<3;i++)
	{
		OLED_ShowChinese(80 + i*16, 16, 36 + next*3 + i, 16, 1);
	}
}

//0:到蒙德了
//1:到璃月了
//2:到稻妻了
//3:到至冬了
// 显示提示：下一站 即将到站 / 已到站
void OLED_Show_NOTICE(u8 next_sta)
{
	// 直接显示“下一站”，不用方向，不乱！
	for(int i=0;i<3;i++)
	{
		OLED_ShowChinese(16 + i*16, 32, 36 + next_sta*3 + i, 16, 1);
	}
	
	// 固定显示：到了
	OLED_ShowChinese(80, 32, 59, 16, 1);
	OLED_ShowChinese(96, 32, 60, 16, 1);
}


