/* 测试demo */
#include "device.h"
#include "serial_api.h"
#include "main.h"
#include "timer_api.h"
#include "rfsmart_app.h"
serial_t	sobj;
gtimer_t RFTimer0;
#define UART_TX    PA_7
#define UART_RX    PA_6


/* 	接收串口数据处理函数
	CMD:命令
	Data:真正的数据
	Len:真正的数据长度
*/
void ProtocalUartDataTest(unsigned char CMD, unsigned char *Data, unsigned short Length)
{	
	switch(CMD)
	{
		/* 获取设备的信息 */
		case GET_DEV_INFO_CMD_REQ:
		{
			break;
		}

		/* 获取设备属性列表 */
		case WIFI_GET_DEV_ATTR_CMD_REQ:
		{
			break;
		}

		/* WIFI模组读取设备所有属性的状态 */
		case  WIFI_READ_DEV_STA_CMD_REQ:
		{
			break;
		}

		/* WIFI模组控制设备 */
		case  WIFI_CTRL_DEV_CMD_REQ:	
		{	
			break;
		}

		/* MCU主动上报当前状态 */
		case MCU_UP_PROPERTY_CMD_REQ:
		{
			break;
		}

		/* 推送wifi工作状态 */
		case WIFI_SNED_STA_CMD_REQ:
		{
			break;
		}

		case RESET_WIFI_CMD_REQ: 			//重置WIFI模组
		{
			break;
		}

		case MCU_RESTART_WIFI_CMD_REQ:		//重启WIFI模组
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

/* 串口接收数据处理 */
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
				RecvUartTimeOutCnt = 0x01;	/* 表开始有串口数据接收 */
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
			if(UartRecvDataLen > 6)		/*因为UartDataLen的长度除了包括数据之外还包含了其他6个字节的帧头长度等等信息*/
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
				UartRxOkFlag = 0x01;	/* 接收完成标志 */
				
				/* 业务处理 */
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

/* 串口接收中断数据接收接口 */
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

/* 1ms 延时函数实现 */
void MCUDelayMs(unsigned int ms)
{
	int i = 0;
	
	for(i = 0; i < ms; i++){
		HalDelayUs(1000);
	}
}

/************************************* 
*	串口发送数据 
CMDTem:			发送的命令
SendFormData:	发送的真正的数据，Data域
Len:			SendFormData的长度
*************************************/
void UartSendFormData(unsigned char CMDTem, const unsigned char *SendFormData, unsigned short Len)
{
	static unsigned char SendDataTem[SENDMAXLEN + 6] = {0};	/* 这里需要把变量定义为static，否则单片机栈空间不够大的话会导致栈溢出 */
	unsigned short SendDataLen = 0;
	unsigned char CRCTem = 0;
	unsigned short i;
	
	SendDataTem[SendDataLen++] = F_HEAD_H;					//包头
	SendDataTem[SendDataLen++] = F_HEAD_L;					//包头

	Len = Len + 6;
	SendDataTem[SendDataLen++] = (Len >> 8) & 0xFF;		//包长高8位
	CRCTem += SendDataTem[SendDataLen - 1];

	SendDataTem[SendDataLen++] = Len & 0xFF;		//包长低8位
	CRCTem += SendDataTem[SendDataLen - 1];
	
	SendDataTem[SendDataLen++] = CMDTem;					//功能码
	CRCTem += SendDataTem[SendDataLen - 1];
	
	for(i = 0; i < Len - 6; i++){
		SendDataTem[SendDataLen++] = SendFormData[i];		//数据
		CRCTem += SendDataTem[SendDataLen - 1];
	}

	SendDataTem[SendDataLen++] = CRCTem;					//校验和
	/* 开始发送数据 */
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


	MCUDelayMs(20);		/* 加个延时，防止有数据下发的时候马上回传到wifi模组导致模组丢数据 */
	for(i = 0; i < Len; i++)
	{
		/* 串口发送数据接口 */
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

