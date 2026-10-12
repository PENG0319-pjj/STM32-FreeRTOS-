#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 
#include "string.h"
#include "stm32f10x.h"

#define USART_REC_LEN  			200
#define EN_USART1_RX 			1

extern char  USART_RX_BUF[USART_REC_LEN];
extern u16 USART_RX_STA;         	


#define false 0
#define true 1

#define GPS_Buffer_Length 80
#define UTCTime_Length 11
#define latitude_Length 11
#define N_S_Length 2
#define longitude_Length 12
#define E_W_Length 2 
#define speed_Length 6

typedef struct SaveData 
{
	char GPS_Buffer[GPS_Buffer_Length];//ԭʼ���ݻ�����
	char isGetData;					   //���ݽ��ձ�־
	char isParseData;					//���ݽ�����־
	char UTCTime[UTCTime_Length];		//ʱ��
	char latitude[latitude_Length];		//γ������
	char N_S[N_S_Length];				//�ϱ�γ��ʶ
	char longitude[longitude_Length];	//��������
	char speed[6];						//速度(节)
	char E_W[E_W_Length];				//��������ʶ
	char isUsefull;						//������Ч�Ա�ʶ
} _SaveData;





extern char rxdatabufer;
extern u16 point1;

void uart1_init(u32 bound);
void uart2_init(u32 bound);
void UART1_SendCode(u8 *DATA,u8 len);
void UART2_SendCode(u8 *DATA,u8 len);
void CLR_Buf(void);
void clrStruct(void);

/* ---------------- USART3 → ESP8266 ---------------- */
#define USART3_REC_LEN  100
extern char USART3_RX_BUF[USART3_REC_LEN];
extern volatile u16 USART3_RX_CNT;    /* 已收到字节数 */
extern volatile u8  USART3_RX_DONE;   /* 1 = 收到一整行(\n) */

void uart3_init(u32 bound);
void UART3_SendCode(u8 *DATA, u8 len);
void USART3_CLR_Buf(void);



#endif
