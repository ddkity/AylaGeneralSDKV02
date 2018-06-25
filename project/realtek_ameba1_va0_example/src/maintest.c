/* ����demo */
#include "device.h"
#include "serial_api.h"
#include "main.h"
#include "timer_api.h"
#include "rfsmart_app.h"
serial_t	sobj;
gtimer_t RFTimer0;
#define UART_TX    PA_7
#define UART_RX    PA_6


/* 	���մ������ݴ�����
	CMD:����
	Data:����������
	Len:���������ݳ���
*/
void ProtocalUartDataTest(unsigned char CMD, unsigned char *Data, unsigned short Length)
{	
	switch(CMD)
	{
		/* ��ȡ�豸����Ϣ */
		case GET_DEV_INFO_CMD_REQ:
		{
			break;
		}

		/* ��ȡ�豸�����б� */
		case WIFI_GET_DEV_ATTR_CMD_REQ:
		{
			break;
		}

		/* WIFIģ���ȡ�豸�������Ե�״̬ */
		case  WIFI_READ_DEV_STA_CMD_REQ:
		{
			break;
		}

		/* WIFIģ������豸 */
		case  WIFI_CTRL_DEV_CMD_REQ:	
		{	
			break;
		}

		/* MCU�����ϱ���ǰ״̬ */
		case MCU_UP_PROPERTY_CMD_REQ:
		{
			break;
		}

		/* ����wifi����״̬ */
		case WIFI_SNED_STA_CMD_REQ:
		{
			break;
		}

		case RESET_WIFI_CMD_REQ: 			//����WIFIģ��
		{
			break;
		}

		case MCU_RESTART_WIFI_CMD_REQ:		//����WIFIģ��
		{
			break;
		}

		case MCU_GET_NTP_TIME_CMD_REQ:
		{
			break;
		}

		case MCU_GET_WIFI_RSSI_CMD_REQ:
		{
			break;
		}

		default:
			break;
	}
}

/* ���ڽ������ݴ��� */
void UartHandler(unsigned char Data)
{
	int  i;
	
	switch(UartStatus)
	{
		case UARTNOP:
		{
			if(UartRxOkFlag){
				break;
			}else{
				UartStatus = UARTSOP_H;
			}
		}

		case UARTSOP_H:
		{
			if(Data == F_HEAD_H)
			{
				RecvBuffer[UartRecvSumLen++] = Data;
				UartStatus = UARTSOP_L;
				RecvUartTimeOutCnt = 0x01;	/* ��ʼ�д������ݽ��� */
			}
			break;
		}

		case UARTSOP_L:
		{
			if(Data == F_HEAD_L){
				RecvBuffer[UartRecvSumLen++] = Data;
				UartStatus = UARTLEN_H;
			}
			break;
		}

		case UARTLEN_H:
		{
			RecvBuffer[UartRecvSumLen++] = Data;
			CalCRC += Data;
			UartRecvDataLen |= (Data << 8);
			UartStatus = UARTLEN_L;
			break;
		}

		case UARTLEN_L:
		{
			RecvBuffer[UartRecvSumLen++] = Data;
			CalCRC += Data;
			UartRecvDataLen |= (Data << 0);
			UartStatus = UARTCMD;
			break;
		}

		case UARTCMD:
		{
			RecvBuffer[UartRecvSumLen++] = Data;
			CalCRC += Data;
			UartStatus = UARTDATA;
			break;
		}

		case UARTDATA:
		{
			if(UartRecvDataLen > 6)		/*��ΪUartDataLen�ĳ��ȳ��˰�������֮�⻹����������6���ֽڵ�֡ͷ���ȵȵ���Ϣ*/
			{
				RecvBuffer[UartRecvSumLen++] = Data;
				CalCRC += Data;
				UartRecvDataLen--;
				break;
			}
			else
			{
				UartStatus = UARTCRC;
			}
		}

		case UARTCRC:
		{
			RecvBuffer[UartRecvSumLen++] = Data;
			RecvCRC = Data;

			if(RecvCRC == CalCRC){
				UartRxOkFlag = 0x01;	/* ������ɱ�־ */
				
				/* ҵ���� */
				if(UartRecvSumLen < 64){
					printf("Recv Data:");	
					for(i = 0; i < UartRecvSumLen; i++)
					{
						printf("%x ", RecvBuffer[i]);
					}
					printf("\n");
				}else{
					printf("Recv %d Data.\n", UartRecvSumLen);
				}

				ProtocalUartDataTest(RecvBuffer[4],& RecvBuffer[5], UartRecvSumLen - 6);
			}else{
				printf("\n===Recv CRC Error.===Expect CRC is: 0x%x, But recv is0x%x\n", CalCRC, RecvCRC);
				printf("Recv Data:");	
				for(i = 0; i < UartRecvSumLen; i++)
				{
					printf("%x ", RecvBuffer[i]);
				}
				printf("\n");
			}
			UartVariableInlt();
			break;
		}
		
		default:
			break;
	}
}

/* ���ڽ����ж����ݽ��սӿ� */
void RFUartIrq(uint32_t id, SerialIrq event)
{
	unsigned char RecvCharTemp = 0;
	
	if(event == RxIrq)
	{
		RecvCharTemp= serial_getc(&sobj);
		UartHandler(RecvCharTemp);
	}
}

void UartInit(void)
{
	serial_init(&sobj,UART_TX,UART_RX);
	serial_baud(&sobj,9600);
	serial_format(&sobj, 8, ParityNone, 1);
	serial_irq_handler(&sobj, RFUartIrq, (uint32_t)&sobj);
	serial_irq_set(&sobj, RxIrq, 1);
	serial_irq_set(&sobj, TxIrq, 1);
}

/* 1ms ��ʱ����ʵ�� */
void MCUDelayMs(unsigned int ms)
{
	int i = 0;
	
	for(i = 0; i < ms; i++){
		HalDelayUs(1000);
	}
}

/************************************* 
*	���ڷ������� 
CMDTem:			���͵�����
SendFormData:	���͵����������ݣ�Data��
Len:			SendFormData�ĳ���
*************************************/
void UartSendFormData(unsigned char CMDTem, const unsigned char *SendFormData, unsigned short Len)
{
	static unsigned char SendDataTem[SENDMAXLEN + 6] = {0};	/* ������Ҫ�ѱ�������Ϊstatic������Ƭ��ջ�ռ䲻����Ļ��ᵼ��ջ��� */
	unsigned short SendDataLen = 0;
	unsigned char CRCTem = 0;
	unsigned short i;
	
	SendDataTem[SendDataLen++] = F_HEAD_H;					//��ͷ
	SendDataTem[SendDataLen++] = F_HEAD_L;					//��ͷ

	Len = Len + 6;
	SendDataTem[SendDataLen++] = (Len >> 8) & 0xFF;		//������8λ
	CRCTem += SendDataTem[SendDataLen - 1];

	SendDataTem[SendDataLen++] = Len & 0xFF;		//������8λ
	CRCTem += SendDataTem[SendDataLen - 1];
	
	SendDataTem[SendDataLen++] = CMDTem;					//������
	CRCTem += SendDataTem[SendDataLen - 1];
	
	for(i = 0; i < Len - 6; i++){
		SendDataTem[SendDataLen++] = SendFormData[i];		//����
		CRCTem += SendDataTem[SendDataLen - 1];
	}

	SendDataTem[SendDataLen++] = CRCTem;					//У���
	/* ��ʼ�������� */
	if(Len < 64){
		printf("Send Data:");
		for(i = 0; i < Len; i++)
		{
			printf("%x ", SendDataTem[i]);
		}
		printf("\n");
	}else{
		printf("Send %d Data.\n", Len);
	}


	MCUDelayMs(20);		/* �Ӹ���ʱ����ֹ�������·���ʱ�����ϻش���wifiģ�鵼��ģ�鶪���� */
	for(i = 0; i < Len; i++)
	{
		/* ���ڷ������ݽӿ� */
		serial_putc(&sobj, SendDataTem[i]);
	}
	memset(SendDataTem, 0x00, SENDMAXLEN + 6);
}

int main(void)
{
	UartInit();
	while(1)
	{
		
	}
}

