/*
huangjituan

1����д������غ����ӿ�
2����д���պͷ��͵�ѭ��buffer
3������ѭ�����ͺͽ���buffer�ĵ����߳�
4��������ȡ�豸��Ϣ���̣߳���ȡ�豸��Ϣ�������б��ܣ�������2018��6��3���賿1:08���ˣ�˯����������д
*/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h" 
#include "gpio_irq_api.h"
#include "serial_api.h"
#include "timer_api.h"
#include "rfsmart_app.h"

/* ��ģ�鹫��ȫ�ֱ��� */
/* ���ڷ���ʱ��Ƭ���Ļظ����� */
SECTION(".sdram.data") unsigned char SendRespondCMD = 0x00;
SECTION(".sdram.data") unsigned char GetDevInfoOKFlag = 0x00;			/* ��ȡ�豸��Ϣ��ɱ�־ */
SECTION(".sdram.data") unsigned char GetDevAttrListOKFlag = 0x00;		/* ��ȡ�豸�����б���ɱ�־ */
/* ��ȡ�豸��Ϣ����ض��� */
SECTION(".sdram.data") char HardVersion[15] = {0};
SECTION(".sdram.data") char SoftVersion[15] = {0};
SECTION(".sdram.data") char OEM_AP_SSID_PREFIX[16]  = {0};
SECTION(".sdram.data") unsigned char AttrNum = 0;	//���Ը���
extern char oem[21];
extern char oem_model[21];
extern char demo_host_version[32];	/* property template version */

SECTION(".sdram.data") unsigned char SendBufferData[SENDMAXLEN] = {0};	/* �������ݷ��ͻ����� */
SECTION(".sdram.data") unsigned short SendBufferDataLen = 0;		/* �������ݷ��ͳ��� */

SECTION(".sdram.data") unsigned char RFLinkWIFIStatus = 0x00;		/* WIFI״̬��ʶλ */

void UartSendFormData(const unsigned char *SendFormData, unsigned short Len);

/* ��ȡ�豸����Ϣ������ */
void GetDevInfofunc(unsigned char *DataBuffer)
{
	int cnt = 0;
	int sub  = 0;
	
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//��ȡӲ���汾
	{
		HardVersion[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//��ȡ����汾
	{
		SoftVersion[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	memset(demo_host_version, 0x00, sizeof(demo_host_version));
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//��ȡhost version
	{
		demo_host_version[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	memset(oem, 0x00, sizeof(oem));
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//��ȡoem id
	{
		oem[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	memset(oem_model, 0x00, sizeof(oem_model));
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//��ȡoem model
	{
		oem_model[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	memset(OEM_AP_SSID_PREFIX, 0x00, sizeof(OEM_AP_SSID_PREFIX));
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//��ȡAP ��ǰ׺
	{
		OEM_AP_SSID_PREFIX[sub] = DataBuffer[cnt];
		sub++;
		cnt++;
	}
	
	sub = 0;
	cnt++;
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))	//��ȡ���Եĸ���
	{
		AttrNum = DataBuffer[cnt];
		break;
	}
	
	printf("hardV:%s, softV:%s, hostV:%s, oemid:%s, oemmodel:%s, APSSID:%s, AttrNum:%d\n", HardVersion, SoftVersion, demo_host_version, oem, oem_model, OEM_AP_SSID_PREFIX, AttrNum);
}

/* 	���մ������ݴ�����
	CMD:����
	Data:����������
	Len:���������ݳ���
*/
void ProtocalUartData(unsigned char CMD, unsigned char *Data, unsigned short Length)
{
	switch(CMD)
	{
		/* ��ȡ�豸��Ϣ */
		case GET_DEV_INFO_CMD_REQ :	
		{
			GetDevInfofunc(Data);
			GetDevInfoOKFlag = 0x01;
			break;
		}

		/* ��ȡ�豸�����б� */
		case WIFI_GET_DEV_ATTR_CMD_REQ:
		{
			GetAttrList(Data);
			GetDevAttrListOKFlag = 0x01;
			break;
		}

		/* WIFIģ������豸���·����� */
		case WIFI_CTRL_DEV_CMD_REQ:
		{
			//todo
			break;
		}

		/* MCU�����ϱ���ǰ״̬ */
		case MCU_UP_PROPERTY_CMD_REQ :
		{
			RFLinkSendPropByName(Data[0], Data[1], Data[2], (Data[3] << 8) | Data[4], &Data[5]);
			break;
		}

		/* ����wifi����״̬ */
		case WIFI_SNED_STA_CMD_REQ :
		{
			break;
		}

		/* WIFIģ���ȡ�豸״̬ */
		case  WIFI_READ_DEV_STA_CMD_REQ :	
		{
			GetAttrAllValAndSend(Data);
			break;
		}

		/* MCU����WIFIģ�� */
		case RESET_WIFI_CMD_REQ: 
		{
			MCU_ResetWIFI();
			break;
		}

		/* MCU����WIFIģ�� */
		case MCU_RESTART_WIFI_CMD_REQ :
		{
			MCU_RestartWIFI();
			break;
		}

		/* ��ȡϵͳʱ�� */
		case MCU_GET_NTP_TIME_CMD_REQ:
		{
			MCU_GetNTPTime(Data[0]);	/* 0:UTCʱ�䣬 1:����ʱ�� */
			break;
		}

		/* ��ȡ�ź�ǿ�� */
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


/////////////////// start ѭ��buffer //////////////////////////////////////////////////////////////////////////
/* ����ʧ���ط���������ͷ���ʱ�������� */
#define SENDDELAYTIME (10)	/* 10ms���һ�� */
#define SENDGAPTIME (130)	/* 130 * 10ms = 1300ms */
#define SENDCNT		(258)	/* ����258*10ms=2580ms���һ�û�лظ������ͳ�ʱ���˳� */

/* ����ѭ������buffer */
SECTION(".sdram.data") struct RingSendBuffer_t RingBufferSend[SENDBUFFERMAXLEN] = {0};
SECTION(".sdram.data") unsigned char R_CurSorSend = 0;	/* ��λ�� */
SECTION(".sdram.data") unsigned char W_CurSorSend = 0;	/* дλ�� */
SECTION(".sdram.data") unsigned char DataNumSend = 0;	/* ���λ������е�Ԫ�������� */
/* �������ѭ��buffer */
SECTION(".sdram.data") struct RingRecvBuffer_t RingBufferRecv[RECVBUFFERMAXLEN] = {0};
SECTION(".sdram.data") unsigned char R_CurSorRecv = 0;	/* ��λ�� */
SECTION(".sdram.data") unsigned char W_CurSorRecv = 0;	/* дλ�� */
SECTION(".sdram.data") unsigned char DataNumRecv = 0;	/* ���λ������е�Ԫ�������� */


/* �����ź��� */
SemaphoreHandle_t xSemaphoreSend = NULL;  
SemaphoreHandle_t xSemaphoreRecv = NULL;

/* ���ӻ������Ķ�д�±� */
unsigned char AddRingSend (unsigned char i)
{
       return (i+1) == SENDBUFFERMAXLEN ? 0 : i+1;
}

/* д���ݵ����λ����� */
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

/* ��ѭ����������ȡ���� */
void ReadRingBufferSend(void)
{
	unsigned char ReadPos;
	int i;
	int cnt = 0;
	
	if(DataNumSend > 0)
	{
		xSemaphoreTake( xSemaphoreSend, portMAX_DELAY );
		ReadPos = R_CurSorSend;
		/* RingSendBuffer[ReadPos].Data[0]��CMD�ֶ� */
		/* bufferҵ���߼����� */
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
		/* Ӧ���ڴ���������֮�����ƶ����� */
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

/* ���ӻ������Ķ�д�±� */
unsigned char AddRingRecv(unsigned char i)
{
       return (i+1) == RECVBUFFERMAXLEN ? 0 : i+1;
}

/* д���ݵ����λ����� */
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

/* �����ݵ����λ����� */
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

		/* ҵ���� */
		ProtocalUartData(RingBufferRecv[ReadPos].CMD, &RingBufferRecv[ReadPos].Data[5], RingBufferRecv[ReadPos].Length - 6);
		/* Ӧ���ڴ���������֮�����ƶ����� */
		R_CurSorRecv = AddRingRecv(R_CurSorRecv);
		DataNumRecv--;
		xSemaphoreGive(xSemaphoreRecv);
	}
	else
	{
		printf("Recv Ring Buffer is empty.\n");
	}
}
/////////////////// end ѭ��buffer/////////////////////////////////////////////////////////////////////////////


/////////////////////////////start ���ںͶ�ʱ��/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* ������صĶ��� */
serial_t sobj;
#define UART_TX    PA_7
#define UART_RX    PA_6

SECTION(".sdram.data") unsigned char RecvCharTemp = 0;	/* �����ʱ���յ������� */
SECTION(".sdram.data") unsigned char UartStatus = UARTNOP;			/* ����״̬ */
SECTION(".sdram.data") unsigned char UartRxOkFlag = 0;		/* ���ڽ������һ֡���ݵı�־��ʹ���������֮������ */
SECTION(".sdram.data") unsigned char RecvBuffer[RECVMAXLEN] = {0};	/* recv from mcu*/
SECTION(".sdram.data") unsigned short UartRecvSumLen = 0;			/* ���ڽ��յ��ܵ����ݳ��ȣ�ÿ����һ�ֽ�����1 */
SECTION(".sdram.data") unsigned short UartRecvDataLen = 0;			/* ���ݰ����������ݰ��ĳ��ȣ�������ͷ�������ȡ� �����ȡ� ����λ�� ����λ�������롢���ݡ�У��͡���β�� */
SECTION(".sdram.data") unsigned char RecvCRC = 0;					/* ���յ���CRC����*/
SECTION(".sdram.data") unsigned char CalCRC = 0;					/* ���������CRC����*/
SECTION(".sdram.data") unsigned int RecvUartTimeOutCnt = 0;	/* �����жϽ��ճ�ʱ��־����ֹ���ʹ������ݵ�ʱ��Ӱ����һ֡�Ľ��� */

/* SendFormData����CMD��Data�ֶ�, len�ĳ�����SendFormData�ĳ��� */
void UartSendFormData(const unsigned char *SendFormData, unsigned short Len)
{
	SECTION(".sdram.data") static unsigned char SendDataTem[SENDMAXLEN] = {0};
	unsigned short SendDataLen = 0;
	unsigned char CRCTem = 0;
	unsigned short i;

	SendDataTem[SendDataLen++] = F_HEAD_H;					//��ͷ
	SendDataTem[SendDataLen++] = F_HEAD_L;					//��ͷ
	
	SendDataTem[SendDataLen++] = ((Len + 5)>>8) & 0xFF;		//������8λ
	CRCTem += SendDataTem[SendDataLen - 1];
	SendDataTem[SendDataLen++] = ((Len + 5)>>0) & 0xFF;		//�����Ͱ�λ
	CRCTem += SendDataTem[SendDataLen - 1];
	
	
	SendDataTem[SendDataLen++] = SendFormData[0];					//������
	CRCTem += SendDataTem[SendDataLen - 1];

	for(i = 0; i < Len - 1; i++){
		SendDataTem[SendDataLen++] = SendFormData[i + 1];		//����
		CRCTem += SendDataTem[SendDataLen - 1];
	}

	SendDataTem[SendDataLen++] = CRCTem;					//У���

	//��ʼ��������
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

/* ������ر����ĳ�ʼ�� */
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

/* �ڶ�ʱ��������ã����ܳ�ʱ */
void UartErrorRecvTimeout(void)
{	
	if(RecvUartTimeOutCnt != 0)	
	{		
		RecvUartTimeOutCnt++;	
		if(RecvUartTimeOutCnt > 15)	/* ���ճ�ʱ1.5�� */
		{
			UartVariableInlt();
		}
	}
}

/* ���ڽ����ж� */
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
					RecvUartTimeOutCnt = 0x01;	//��ʾ��ʼ�����ݽ���
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
				if(UartRecvDataLen > 6)		/*��ΪUartDataLen�ĳ��ȳ��˰�������֮�⻹����������6���ֽڵ�֡ͷ֡β���ȵȵ���Ϣ*/
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
					/* �ѽ��յ�buffer����ѭ��buffer�� */
					WriteRingBufferRecv(RecvBuffer, UartRecvSumLen);
				}
				else
				{
				
					printf("\n===Recv CRC Error.===Expect CRC is: %02x, But recv is %02x\n", CalCRC, RecvCRC);
				}

				/* ��ʼ�����ڽ��յ���ر��� */
				UartVariableInlt();
				break;
			}
			
			default:
				break;
		}
	}
}

/* ��ʱ���Ķ��壬���ڷ�ֹ���մ��ڳ�ʱ */
gtimer_t RFTimer0;
void RFTime0Handler(unsigned int TimeOut)
{
	UartErrorRecvTimeout();
}

/////////////////////////////end ���ںͶ�ʱ��/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////start ���շ����߳�////////////////////////////////////////////
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
/////////////////////////////end ���շ����߳�////////////////////////////////////////////

/////////////////////////////start ҵ��ӿں���///////////////////////////////
//��ȡ�豸��Ϣ,�����ȡʧ�ܣ�ÿ1���ӷ���һ��
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

/* ��ȡ�豸�����б�,�����ȡʧ�ܣ�ÿ3���ӷ���һ�Σ�ֱ�����յ���ȷ������Ϊֹ */
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
/////////////////////////////end ҵ��ӿں���///////////////////////////////


/* ��ȡ�豸���Եĳ�ʼֵ */
void GetDEVAllAttrVal(void)
{
	int i = 0;
	unsigned char Data[2] = {0};
	
	Data[0] = WIFI_READ_DEV_STA_CMD;
	for(i = 0; i < AttrNum; i++)
	{
		Data[1] = i;
		WriteRingBufferSend(Data, 2);
		vTaskDelay(20);	/* ��ֹѭ��buffer������������΢��ʱһ�� */
	}
}

/* ��ȡ�豸��Ϣ�߳� */
unsigned char RFSmartInitOK_Flag = 0x00;		/* �豸��ʼ����ɱ�־ */
static void GetDevInfoHandlerTask( void *pvParameters )
{
	int i;
	unsigned char RFLinkWIFIStatusTemp;
	unsigned char Data[2] = {0};
	
	//��ȡ�豸����Ϣ
	GetDevInfo();

	for(i = 0; i < AttrNum; i++){
		//��ȡ�����б���Ϣ
		GetDEVAllAttrList(i);
	}

	GetDevHostVersion();

	/* ��ӡ�����б� */
	PrintfAttrList();
	printf("Rfsmart Init OK.\n");
	RFSmartInitOK_Flag = 0x01;		/* �豸��ʼ����ɱ�־ */

	/* ���wifi����״̬ */
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
	/* ���ڳ�ʼ�� */
	serial_init(&sobj,UART_TX,UART_RX);
	serial_baud(&sobj,9600);
	serial_format(&sobj, 8, ParityNone, 1);
	serial_irq_handler(&sobj, RF_uart_irq, (uint32_t)&sobj);
	serial_irq_set(&sobj, RxIrq, 1);
	serial_irq_set(&sobj, TxIrq, 1);

	/* ��ʱ����ʼ�� 100ms����һ�ζ�ʱ���ж� */
	gtimer_init(&RFTimer0, TIMER0);
	gtimer_start_periodical(&RFTimer0, 100000, (void*)RFTime0Handler, NULL);

	xSemaphoreSend = xSemaphoreCreateMutex();  
	xSemaphoreRecv = xSemaphoreCreateMutex();  

	/* �����߳� */
	xTaskCreate( SendBufferHandler, "SendBufferHandler", 512, NULL, tskIDLE_PRIORITY + 2 + PRIORITIE_OFFSET, NULL );
	/* �����߳� */
	xTaskCreate( RecvBufferHandler, "RecvBufferHandler", 512, NULL, tskIDLE_PRIORITY + 2 + PRIORITIE_OFFSET, NULL );

	// ������ȡ�豸��Ϣ�߳�
	xTaskCreate( GetDevInfoHandlerTask, "GetDevInfoHandlerTask", 512, NULL,  tskIDLE_PRIORITY + 2 + PRIORITIE_OFFSET, NULL );
}


