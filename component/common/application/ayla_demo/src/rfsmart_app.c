/*
huangjituan

1、编写串口相关函数接口
2、编写接收和发送的循环buffer
3、创建循环发送和接收buffer的调用线程
4、创建获取设备信息的线程，获取设备信息和属性列表功能，现在是2018年6月3日凌晨1:08分了，睡觉，明天再写
*/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h" 
#include "gpio_irq_api.h"
#include "serial_api.h"
#include "timer_api.h"
#include "rfsmart_app.h"

/* 各模块公用全局变量 */
/* 串口发送时单片机的回复命令 */
SECTION(".sdram.data") unsigned char SendRespondCMD = 0x00;
SECTION(".sdram.data") unsigned char GetDevInfoOKFlag = 0x00;			/* 获取设备信息完成标志 */
SECTION(".sdram.data") unsigned char GetDevAttrListOKFlag = 0x00;		/* 获取设备属性列表完成标志 */
/* 获取设备信息的相关定义 */
SECTION(".sdram.data") char HardVersion[15] = {0};
SECTION(".sdram.data") char SoftVersion[15] = {0};
SECTION(".sdram.data") char OEM_AP_SSID_PREFIX[16]  = {0};
SECTION(".sdram.data") unsigned char AttrNum = 0;	//属性个数
extern char oem[21];
extern char oem_model[21];
extern char demo_host_version[32];	/* property template version */

SECTION(".sdram.data") unsigned char SendBufferData[SENDMAXLEN] = {0};	/* 串口数据发送缓存区 */
SECTION(".sdram.data") unsigned short SendBufferDataLen = 0;		/* 串口数据发送长度 */

SECTION(".sdram.data") unsigned char RFLinkWIFIStatus = 0x00;		/* WIFI状态标识位 */

void UartSendFormData(const unsigned char *SendFormData, unsigned short Len);

/* 获取设备的信息并解析 */
void GetDevInfofunc(unsigned char *DataBuffer)
{
	int cnt = 0;
	int sub  = 0;
	
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//获取硬件版本
	{
		HardVersion[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//获取软件版本
	{
		SoftVersion[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	memset(demo_host_version, 0x00, sizeof(demo_host_version));
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//获取host version
	{
		demo_host_version[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	memset(oem, 0x00, sizeof(oem));
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//获取oem id
	{
		oem[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	memset(oem_model, 0x00, sizeof(oem_model));
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//获取oem model
	{
		oem_model[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	memset(OEM_AP_SSID_PREFIX, 0x00, sizeof(OEM_AP_SSID_PREFIX));
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//获取AP 的前缀
	{
		OEM_AP_SSID_PREFIX[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//获取属性的个数
	{
		AttrNum = DataBuffer[cnt];
		break;
	}
	
	printf("hardV:%s, softV:%s, hostV:%s, oemid:%s, oemmodel:%s, APSSID:%s, AttrNum:%d\n", HardVersion, SoftVersion, demo_host_version, oem, oem_model, OEM_AP_SSID_PREFIX, AttrNum);
}

/* 	接收串口数据处理函数
	CMD:命令
	Data:真正的数据
	Len:真正的数据长度
*/
void ProtocalUartData(unsigned char CMD, unsigned char *Data, unsigned short Length)
{
	switch(CMD)
	{
		/* 获取设备信息 */
		case GET_DEV_INFO_CMD_REQ :	
		{
			GetDevInfofunc(Data);
			GetDevInfoOKFlag = 0x01;
			break;
		}

		/* 获取设备属性列表 */
		case WIFI_GET_DEV_ATTR_CMD_REQ:
		{
			GetAttrList(Data);
			GetDevAttrListOKFlag = 0x01;
			break;
		}

		/* WIFI模组控制设备，下发属性 */
		case WIFI_CTRL_DEV_CMD_REQ:
		{
			//todo
			break;
		}

		/* MCU主动上报当前状态 */
		case MCU_UP_PROPERTY_CMD_REQ :
		{
			RFLinkSendPropByName(Data[0], Data[1], Data[2], (Data[3] << 8) | Data[4], &Data[5]);
			break;
		}

		/* 推送wifi工作状态 */
		case WIFI_SNED_STA_CMD_REQ :
		{
			break;
		}

		/* WIFI模组读取设备状态 */
		case  WIFI_READ_DEV_STA_CMD_REQ :	
		{
			GetAttrAllValAndSend(Data);
			break;
		}

		/* MCU重置WIFI模组 */
		case RESET_WIFI_CMD_REQ: 
		{
			MCU_ResetWIFI();
			break;
		}

		/* MCU重启WIFI模组 */
		case MCU_RESTART_WIFI_CMD_REQ :
		{
			MCU_RestartWIFI();
			break;
		}

		/* 获取系统时间 */
		case MCU_GET_NTP_TIME_CMD_REQ:
		{
			MCU_GetNTPTime(Data[0]);	/* 0:UTC时间， 1:本地时间 */
			break;
		}

		/* 获取信号强度 */
		case MCU_GET_WIFI_RSSI_CMD_REQ:
		{
			Get_WIFIRSSI();
			break;
		}
		default:
			break;
	}
	
	SendRespondCMD = CMD;
}


/////////////////// start 循环buffer //////////////////////////////////////////////////////////////////////////
/* 发送失败重发次数定义和发送时间间隔定义 */
#define SENDDELAYTIME (10)	/* 10ms检查一次 */
#define SENDGAPTIME (130)	/* 130 * 10ms = 1300ms */
#define SENDCNT		(258)	/* 发送258*10ms=2580ms左右还没有回复，发送超时，退出 */

/* 定义循环发送buffer */
SECTION(".sdram.data") struct RingSendBuffer_t RingBufferSend[SENDBUFFERMAXLEN] = {0};
SECTION(".sdram.data") unsigned char R_CurSorSend = 0;	/* 读位置 */
SECTION(".sdram.data") unsigned char W_CurSorSend = 0;	/* 写位置 */
SECTION(".sdram.data") unsigned char DataNumSend = 0;	/* 环形缓存区中的元素总数量 */
/* 定义接收循环buffer */
SECTION(".sdram.data") struct RingRecvBuffer_t RingBufferRecv[RECVBUFFERMAXLEN] = {0};
SECTION(".sdram.data") unsigned char R_CurSorRecv = 0;	/* 读位置 */
SECTION(".sdram.data") unsigned char W_CurSorRecv = 0;	/* 写位置 */
SECTION(".sdram.data") unsigned char DataNumRecv = 0;	/* 环形缓存区中的元素总数量 */


/* 互斥信号锁 */
SemaphoreHandle_t xSemaphoreSend = NULL;  
SemaphoreHandle_t xSemaphoreRecv = NULL;

/* 增加缓存区的读写下标 */
unsigned char AddRingSend (unsigned char i)
{
       return (i+1) == SENDBUFFERMAXLEN ? 0 : i+1;
}

/* 写数据到环形缓冲区 */
void WriteRingBufferSend(unsigned char *Data, unsigned short Len)
{
	if(DataNumSend < SENDBUFFERMAXLEN)
	{
		xSemaphoreTake( xSemaphoreSend, portMAX_DELAY ); 
		memcpy(RingBufferSend[W_CurSorSend].Data, Data, Len);
		RingBufferSend[W_CurSorSend].Length = Len;

		W_CurSorSend = AddRingSend(W_CurSorSend);
		DataNumSend++;
		 xSemaphoreGive( xSemaphoreSend );  
	}
	else
	{
		printf("Send Ring Buffer is full.\n");
	}
}

/* 从循环缓冲区读取数据 */
void ReadRingBufferSend(void)
{
	unsigned char ReadPos;
	int i;
	int cnt = 0;
	
	if(DataNumSend > 0)
	{
		xSemaphoreTake( xSemaphoreSend, portMAX_DELAY );
		ReadPos = R_CurSorSend;
		/* RingSendBuffer[ReadPos].Data[0]是CMD字段 */
		/* buffer业务逻辑处理 */
		SendRespondCMD = 0x00;
		while(SendRespondCMD != (RingBufferSend[ReadPos].Data[0] + 1)){
			if((cnt%SENDGAPTIME) == 0){
				UartSendFormData(RingBufferSend[ReadPos].Data, RingBufferSend[ReadPos].Length);
			}

			if(cnt > SENDCNT){
				printf("Data is Send, But MCU not Respond.CMD = %02x\n", RingBufferSend[ReadPos].Data[0]);
				break;
			}
			cnt++;
			vTaskDelay(SENDDELAYTIME);
		}
		/* 应该在处理完数据之后再移动坐标 */
		R_CurSorSend = AddRingSend(R_CurSorSend);
		DataNumSend--;
		xSemaphoreGive( xSemaphoreSend );
	}
	else
	{
		printf("Send Ring Buffer is empty.\n");
	}
}
/*********************************************************************************************************************/

/* 增加缓存区的读写下标 */
unsigned char AddRingRecv(unsigned char i)
{
       return (i+1) == RECVBUFFERMAXLEN ? 0 : i+1;
}

/* 写数据到环形缓冲区 */
void WriteRingBufferRecv(unsigned char *Data, unsigned short Len)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if(DataNumRecv < RECVBUFFERMAXLEN)
	{
		xSemaphoreTakeFromISR(xSemaphoreRecv, &xHigherPriorityTaskWoken); 
		RingBufferRecv[W_CurSorRecv].CMD = Data[4];
		memcpy(RingBufferRecv[W_CurSorRecv].Data, &Data[0], Len);
		RingBufferRecv[W_CurSorRecv].Length = Len;

		W_CurSorRecv = AddRingRecv(W_CurSorRecv);
		DataNumRecv++;
		xSemaphoreGiveFromISR(xSemaphoreRecv, &xHigherPriorityTaskWoken);
	}
	else
	{
		printf("Recv Ring Buffer is full.\n");
	}
}

/* 读数据到环形缓冲区 */
void ReadRingBufferRecv(void)
{
	unsigned char ReadPos;
	int i;
	
	if(DataNumRecv > 0)
	{
		xSemaphoreTake(xSemaphoreRecv, portMAX_DELAY); 
		ReadPos = R_CurSorRecv;

		if(RingBufferRecv[ReadPos].Length < 64){
			printf("Recv Data:");
			for(i = 0; i < RingBufferRecv[ReadPos].Length; i++)
			{
				printf("%02x ", RingBufferRecv[ReadPos].Data[i]);
			}
			printf("\n");
		}else{
			printf("Recv %d Data.\n", RingBufferRecv[ReadPos].Length);
		}

		/* 业务处理 */
		ProtocalUartData(RingBufferRecv[ReadPos].CMD, &RingBufferRecv[ReadPos].Data[5], RingBufferRecv[ReadPos].Length - 6);
		/* 应该在处理完数据之后再移动坐标 */
		R_CurSorRecv = AddRingRecv(R_CurSorRecv);
		DataNumRecv--;
		xSemaphoreGive(xSemaphoreRecv);
	}
	else
	{
		printf("Recv Ring Buffer is empty.\n");
	}
}
/////////////////// end 循环buffer/////////////////////////////////////////////////////////////////////////////


/////////////////////////////start 串口和定时器/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* 串口相关的定义 */
serial_t sobj;
#define UART_TX    PA_7
#define UART_RX    PA_6

SECTION(".sdram.data") unsigned char RecvCharTemp = 0;	/* 存放临时接收到的数据 */
SECTION(".sdram.data") unsigned char UartStatus = UARTNOP;			/* 接收状态 */
SECTION(".sdram.data") unsigned char UartRxOkFlag = 0;		/* 串口接收完成一帧数据的标志，使用完成数据之后清零 */
SECTION(".sdram.data") unsigned char RecvBuffer[RECVMAXLEN] = {0};	/* recv from mcu*/
SECTION(".sdram.data") unsigned short UartRecvSumLen = 0;			/* 串口接收的总的数据长度，每接收一字节增加1 */
SECTION(".sdram.data") unsigned short UartRecvDataLen = 0;			/* 数据包中整个数据包的长度（包括包头、包长度、 包长度、 保留位、 保留位、功能码、数据、校验和、包尾） */
SECTION(".sdram.data") unsigned char RecvCRC = 0;					/* 接收到的CRC数据*/
SECTION(".sdram.data") unsigned char CalCRC = 0;					/* 计算出来的CRC数据*/
SECTION(".sdram.data") unsigned int RecvUartTimeOutCnt = 0;	/* 串口中断接收超时标志，防止发送错误数据的时候影响下一帧的接收 */

/* SendFormData包括CMD和Data字段, len的长度是SendFormData的长度 */
void UartSendFormData(const unsigned char *SendFormData, unsigned short Len)
{
	SECTION(".sdram.data") static unsigned char SendDataTem[SENDMAXLEN] = {0};
	unsigned short SendDataLen = 0;
	unsigned char CRCTem = 0;
	unsigned short i;

	SendDataTem[SendDataLen++] = F_HEAD_H;					//包头
	SendDataTem[SendDataLen++] = F_HEAD_L;					//包头
	
	SendDataTem[SendDataLen++] = ((Len + 5)>>8) & 0xFF;		//包长高8位
	CRCTem += SendDataTem[SendDataLen - 1];
	SendDataTem[SendDataLen++] = ((Len + 5)>>0) & 0xFF;		//包长低八位
	CRCTem += SendDataTem[SendDataLen - 1];
	
	
	SendDataTem[SendDataLen++] = SendFormData[0];					//功能码
	CRCTem += SendDataTem[SendDataLen - 1];

	for(i = 0; i < Len - 1; i++){
		SendDataTem[SendDataLen++] = SendFormData[i + 1];		//数据
		CRCTem += SendDataTem[SendDataLen - 1];
	}

	SendDataTem[SendDataLen++] = CRCTem;					//校验和

	//开始发送数据
	if(SendDataLen < 64){
		printf("Send Data:");
		for(i = 0; i < SendDataLen; i++)
		{
			printf("%02x ", SendDataTem[i]);
		}
		printf("\n");
	}else{
		printf("Send %d Data.\n", SendDataLen);
	}
	
	for(i = 0; i < SendDataLen; i++)
	{
		serial_putc(&sobj, SendDataTem[i]);
	}

	memset(SendDataTem, 0x00, SENDMAXLEN);
}

/* 串口相关变量的初始化 */
void UartVariableInlt(void)
{
	RecvUartTimeOutCnt=0;
	UartRecvSumLen = 0;
	UartRecvDataLen = 0;
	CalCRC = 0;
	RecvCRC = 0;
	UartStatus = UARTNOP;
	UartRxOkFlag = 0;
	RecvCharTemp = 0;
	memset(RecvBuffer, 0x00, RECVMAXLEN);
}

/* 在定时器里面调用，接受超时 */
void UartErrorRecvTimeout(void)
{	
	if(RecvUartTimeOutCnt != 0)	
	{		
		RecvUartTimeOutCnt++;	
		if(RecvUartTimeOutCnt > 15)	/* 接收超时1.5秒 */
		{
			UartVariableInlt();
		}
	}
}

/* 串口接收中断 */
void RF_uart_irq(uint32_t id, SerialIrq event)
{
	if(event == RxIrq)
	{
		RecvCharTemp = serial_getc(&sobj);
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
				if(RecvCharTemp == F_HEAD_H)
				{
					RecvBuffer[UartRecvSumLen++] = RecvCharTemp;
					UartStatus = UARTSOP_L;
					RecvUartTimeOutCnt = 0x01;	//表示开始有数据接收
				}else{
					UartVariableInlt();
				}
				break;
			}

			case UARTSOP_L:
			{
				if(RecvCharTemp == F_HEAD_L){
					RecvBuffer[UartRecvSumLen++] = RecvCharTemp;
					UartStatus = UARTLEN_H;
				}else{
					UartVariableInlt();
				}
				break;
			}

			case UARTLEN_H:
			{
				RecvBuffer[UartRecvSumLen++] = RecvCharTemp;
				CalCRC += RecvCharTemp;
				UartRecvDataLen |= (RecvCharTemp << 8);
				UartStatus = UARTLEN_L;
				break;
			}

			case UARTLEN_L:
			{
				RecvBuffer[UartRecvSumLen++] = RecvCharTemp;
				CalCRC += RecvCharTemp;
				UartRecvDataLen |= (RecvCharTemp << 0);
				UartStatus = UARTCMD;
				break;
			}

			case UARTCMD:
			{
				RecvBuffer[UartRecvSumLen++] = RecvCharTemp;
				CalCRC += RecvCharTemp;
				UartStatus = UARTDATA;
				break;
			}

			case UARTDATA:
			{
				if(UartRecvDataLen > 6)		/*因为UartDataLen的长度除了包括数据之外还包含了其他6个字节的帧头帧尾长度等等信息*/
				{
					RecvBuffer[UartRecvSumLen++] = RecvCharTemp;
					CalCRC += RecvCharTemp;
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
				RecvBuffer[UartRecvSumLen++] = RecvCharTemp;
				RecvCRC = RecvCharTemp;

				if(RecvCRC == CalCRC)
				{
					UartRxOkFlag = 0x01;
					/* 把接收的buffer存入循环buffer中 */
					WriteRingBufferRecv(RecvBuffer, UartRecvSumLen);
				}
				else
				{
				
					printf("\n===Recv CRC Error.===Expect CRC is: %02x, But recv is %02x\n", CalCRC, RecvCRC);
				}

				/* 初始化串口接收的相关变量 */
				UartVariableInlt();
				break;
			}
			
			default:
				break;
		}
	}
}

/* 定时器的定义，用于防止接收串口超时 */
gtimer_t RFTimer0;
void RFTime0Handler(unsigned int TimeOut)
{
	UartErrorRecvTimeout();
}

/////////////////////////////end 串口和定时器/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////start 接收发送线程////////////////////////////////////////////
static void SendBufferHandler( void *pvParameters )
{
	while(1)
	{
		vTaskDelay(20);
		while(DataNumSend != 0){
			ReadRingBufferSend();
		}
	}
}

static void RecvBufferHandler( void *pvParameters )
{
	while(1)
	{
		vTaskDelay(20);
		while(DataNumRecv != 0){
			ReadRingBufferRecv();
		}
	}
}
/////////////////////////////end 接收发送线程////////////////////////////////////////////

/////////////////////////////start 业务接口函数///////////////////////////////
//获取设备信息,如果获取失败，每1秒钟发送一次
void GetDevInfo(void)
{
	int cnt = 0;
	unsigned char Data[1] = {0};

	Data[0] = GET_DEV_INFO_CMD;

	GetDevInfoOKFlag = 0x00;
	while(GetDevInfoOKFlag != 0x01)
	{
		if(cnt%(SENDCNT*2) == 0){
			WriteRingBufferSend(Data, 1);
		}

		cnt++;
		vTaskDelay(SENDDELAYTIME);
	}
}

/* 获取设备属性列表,如果获取失败，每3秒钟发送一次，直到接收到正确的数据为止 */
void GetDEVAllAttrList(unsigned char attrnum)
{
	int cnt = 0;
	unsigned char Data[2] = {0};

	Data[0] = WIFI_GET_DEV_ATTR_CMD;
	Data[1] = attrnum;
	
	GetDevAttrListOKFlag = 0x00;
	while(GetDevAttrListOKFlag != 0x01)
	{
		if(cnt%(SENDCNT*2) == 0){
			WriteRingBufferSend(Data, 2);	
		}

		cnt++;
		vTaskDelay(SENDDELAYTIME);
	}
}
/////////////////////////////end 业务接口函数///////////////////////////////


/* 获取设备属性的初始值 */
void GetDEVAllAttrVal(void)
{
	int i = 0;
	unsigned char Data[2] = {0};
	
	Data[0] = WIFI_READ_DEV_STA_CMD;
	for(i = 0; i < AttrNum; i++)
	{
		Data[1] = i;
		WriteRingBufferSend(Data, 2);
		vTaskDelay(20);	/* 防止循环buffer处理不过来，稍微延时一下 */
	}
}

/* 获取设备信息线程 */
unsigned char RFSmartInitOK_Flag = 0x00;		/* 设备初始化完成标志 */
static void GetDevInfoHandlerTask( void *pvParameters )
{
	int i;
	unsigned char RFLinkWIFIStatusTemp;
	unsigned char Data[2] = {0};
	
	//获取设备的信息
	GetDevInfo();

	for(i = 0; i < AttrNum; i++){
		//获取属性列表信息
		GetDEVAllAttrList(i);
	}

	GetDevHostVersion();

	/* 打印属性列表 */
	PrintfAttrList();
	printf("Rfsmart Init OK.\n");
	RFSmartInitOK_Flag = 0x01;		/* 设备初始化完成标志 */

	/* 检测wifi工作状态 */
	RFLinkWIFIStatusTemp = RFLinkWIFIStatus;
	Data[0] = WIFI_SNED_STA_CMD;
	Data[1] = RFLinkWIFIStatusTemp;
	WriteRingBufferSend(Data, 2);
	while(1)
	{
		vTaskDelay(100);
		if(RFLinkWIFIStatusTemp != RFLinkWIFIStatus){
			RFLinkWIFIStatusTemp = RFLinkWIFIStatus;
			Data[0] = WIFI_SNED_STA_CMD;
			Data[1] = RFLinkWIFIStatusTemp;
			WriteRingBufferSend(Data, 2);
		}
	}
	
}
void RFSmartInit(void)
{
	/* 串口初始化 */
	serial_init(&sobj,UART_TX,UART_RX);
	serial_baud(&sobj,9600);
	serial_format(&sobj, 8, ParityNone, 1);
	serial_irq_handler(&sobj, RF_uart_irq, (uint32_t)&sobj);
	serial_irq_set(&sobj, RxIrq, 1);
	serial_irq_set(&sobj, TxIrq, 1);

	/* 定时器初始化 100ms进入一次定时器中断 */
	gtimer_init(&RFTimer0, TIMER0);
	gtimer_start_periodical(&RFTimer0, 100000, (void*)RFTime0Handler, NULL);

	xSemaphoreSend = xSemaphoreCreateMutex();  
	xSemaphoreRecv = xSemaphoreCreateMutex();  

	/* 发送线程 */
	xTaskCreate( SendBufferHandler, "SendBufferHandler", 512, NULL, tskIDLE_PRIORITY + 2 + PRIORITIE_OFFSET, NULL );
	/* 接收线程 */
	xTaskCreate( RecvBufferHandler, "RecvBufferHandler", 512, NULL, tskIDLE_PRIORITY + 2 + PRIORITIE_OFFSET, NULL );

	// 创建获取设备信息线程
	xTaskCreate( GetDevInfoHandlerTask, "GetDevInfoHandlerTask", 512, NULL,  tskIDLE_PRIORITY + 2 + PRIORITIE_OFFSET, NULL );
}


