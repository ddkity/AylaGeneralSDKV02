/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */

/*
 * Ayla device agent demo of a simple lights and buttons evaluation board
 * using the "simple" property manager.
 *
 * The property names are chosen to be compatible with the Ayla Control
 * App.  E.g., the LED property is Blue_LED even though the color is yellow.
 * Button1 sends the Blue_button property, even though the button is white.
 */
#define HAVE_UTYPES
#include "lwip/ip_addr.h"

#include <ayla/utypes.h>
#include <ayla/log.h>
#include <sys/types.h>
#include <ada/libada.h>
#include <ada/sprop.h>
#include <ada/task_label.h>
#include "conf.h"
#include "demo.h"
//#include "led_key.h"
#include "rfsmart_app.h"

#define BUILD_PROGNAME "ayla_ledevb_demo"
#define BUILD_VERSION " WV:1.0.2"
#define BUILD_STRING	BUILD_VERSION" "__DATE__" "__TIME__

SECTION(".sdram.data") const char mod_sw_build[] = BUILD_STRING;
SECTION(".sdram.data") const char mod_sw_version[] = BUILD_PROGNAME " " BUILD_STRING;

extern char HardVersion[15];
extern char SoftVersion[15];
extern unsigned char RFLinkWIFIStatus;		/* WIFI״̬��ʶλ */
/* �����ź��� */
extern SemaphoreHandle_t xSemaphoreSend;  
extern SemaphoreHandle_t xSemaphoreRecv;

/*
 * The oem and oem_model strings determine the host name for the
 * Ayla device service and the device template on the service.
 *
 * If these are changed, the encrypted OEM secret must be re-encrypted
 * unless the oem_model was "*" (wild-card) when the oem_key was encrypted.
 */
SECTION(".sdram.data") char oem[21] = {0};
SECTION(".sdram.data") char oem_model[21] = {0};
SECTION(".sdram.data") char version[64] = {0};
SECTION(".sdram.data") char demo_host_version[32] = {0};	/* property template version */

SECTION(".sdram.data") struct ada_sprop demo_props[ALL_TYPE_NUM] = {0};		/* �����б� */
SECTION(".sdram.data") struct AttrIndexList_t AttrIndexList[ALL_TYPE_NUM] = {0}; 	/* ���������б� */
SECTION(".sdram.data") char AttrNameTemp[ALL_TYPE_NUM][64] = {0x00};	/* ��ʱ��������� */

//ATLV_BOOL
SECTION(".sdram.data") unsigned char t_bool[T_BOOL_NUM];
SECTION(".sdram.data") unsigned char t_bool_index = 0x00;
SECTION(".sdram.data") unsigned char f_bool[F_BOOL_NUM];
SECTION(".sdram.data") unsigned char f_bool_index = 0x00;
//ATLV_INT
SECTION(".sdram.data") int t_int[T_INT_NUM];
SECTION(".sdram.data") unsigned char t_int_index = 0x00;
SECTION(".sdram.data") int f_int[F_INT_NUM];
SECTION(".sdram.data") unsigned char f_int_index = 0x00;
//ATLV_CENTS
SECTION(".sdram.data") int t_decimal[T_CENTS];
SECTION(".sdram.data") unsigned char t_decimal_index = 0x00;
SECTION(".sdram.data") int f_decimal[F_CENTS];
SECTION(".sdram.data") unsigned char f_decimal_index = 0x00;
//ATLV_UTF8
SECTION(".sdram.data") char t_string[T_UTF8][1024 + 1];
SECTION(".sdram.data") unsigned char t_string_index = 0x00;
SECTION(".sdram.data") char f_string[F_UTF8][1024 + 1];
SECTION(".sdram.data") unsigned char f_string_index = 0x00;

/* ��ȡ�豸��������״̬�ɹ��ı�־ */
SECTION(".sdram.data") unsigned char IsGetDevStaOK = 0;

static enum ada_err prop_send_by_name(const char *name)
{
	enum ada_err err;

	err = ada_sprop_send_by_name(name);
	if (err) {
		log_put(LOG_INFO "demo: %s: send of %s: err %d",
		    __func__, name, err);
	}
	return err;
}

/* ���Կ��ƻص����� */
static enum ada_err t_property_set(struct ada_sprop *sprop, const void *buf, size_t len)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	unsigned char AttrMinorIndex = 0x00;	//���Դ�����ֵ
	unsigned short cnt = 0x00;

	/* ��ȡ����״̬�ɹ�֮ǰ�Ȳ�Ҫ�·����� */
	if(IsGetDevStaOK != 1)
	{
		printf("ignore %s attr.\n", sprop->name);
		return AE_OK;
	}

	SendBufferData[cnt++] = WIFI_CTRL_DEV_CMD;
	
	if(sprop->type == ATLV_BOOL)
	{
		ret = ada_sprop_set_bool(sprop, buf, len);
		if (ret) {
			return ret;
		}

		/* �������б���Ѱ������������ȷ�����ĸ����� */
		for(i = 0; i < AttrNum; i++)
		{
			if(memcmp(sprop->name, demo_props[i].name, strlen( demo_props[i].name)) == 0)
				break;
		}
		//printf("this is the %d attr.\n", i);
		printf("%s set to %u\n", sprop->name, *(u8 *)sprop->val);
		SendBufferData[cnt++] = TYPE_BOOL;					/* �������� */
		SendBufferData[cnt++] = i;							/* ���������� */
		AttrMinorIndex = AttrIndexList[i].MinorIndex;
		SendBufferData[cnt++] = t_bool[AttrMinorIndex];			/* ����ֵ */
		SendBufferDataLen = cnt;
	}

	else if(sprop->type == ATLV_INT)
	{
		ret = ada_sprop_set_int(sprop, buf, len);
		if (ret) {
			return ret;
		}

		for(i = 0; i < AttrNum; i++)
		{
			if(memcmp(sprop->name, demo_props[i].name, strlen( demo_props[i].name)) == 0)
				break;
		}
		//printf("this is the %d attr.\n", i);
		printf("%s set to %d\n", sprop->name,  *(u32 *)sprop->val);
		SendBufferData[cnt++] = TYPE_INT;					/* �������� */
		SendBufferData[cnt++] = i;						/* ���������� */
		AttrMinorIndex = AttrIndexList[i].MinorIndex;
		SendBufferData[cnt++] = (t_int[AttrMinorIndex] >> 24) & 0xFF;		/* ����ֵ ���ģʽ�����ֽ���ǰ */
		SendBufferData[cnt++] = (t_int[AttrMinorIndex] >> 16) & 0xFF;
		SendBufferData[cnt++] = (t_int[AttrMinorIndex] >> 8) & 0xFF;
		SendBufferData[cnt++] = (t_int[AttrMinorIndex] >> 0)& 0xFF;
		SendBufferDataLen = cnt;
	}

	else if(sprop->type == ATLV_CENTS)
	{
		
		ret = ada_sprop_set_int(sprop, buf, len);
		if (ret) {
			return ret;
		}

		for(i = 0; i < AttrNum; i++)
		{
			if(memcmp(sprop->name, demo_props[i].name, strlen( demo_props[i].name)) == 0)
				break;
		}
		//printf("this is the %d attr.\n", i);
		printf("%s set to %d\n", sprop->name, *(int *)sprop->val);

		AttrMinorIndex = AttrIndexList[i].MinorIndex;
		SendBufferData[cnt++] = TYPE_CENTS;					/* �������� */
		SendBufferData[cnt++] = i;						/* ���������� */
		SendBufferData[cnt++] = (t_decimal[AttrMinorIndex] >> 24) & 0xFF;		/* ����ֵ */
		SendBufferData[cnt++] = (t_decimal[AttrMinorIndex] >> 16) & 0xFF;		/* ����ֵ */
		SendBufferData[cnt++] = (t_decimal[AttrMinorIndex] >> 8) & 0xFF;		/* ����ֵ */
		SendBufferData[cnt++] = (t_decimal[AttrMinorIndex] >> 0) & 0xFF;		/* ����ֵ */
		SendBufferDataLen = cnt;
	}

	else if(sprop->type == ATLV_UTF8)
	{
		ret = ada_sprop_set_string(sprop, buf, len);
		if (ret) {
			return ret;
		}

		for(i = 0; i < AttrNum; i++)
		{
			if(memcmp(sprop->name, demo_props[i].name, strlen( demo_props[i].name)) == 0)
				break;
		}
		//printf("this is the %d attr.\n", i);
		printf("%s set to %s.data len is  %d\n",  sprop->name, (unsigned char *)sprop->val,  len);
		AttrMinorIndex = AttrIndexList[i].MinorIndex;
		
		SendBufferData[cnt++] = TYPE_UTF8;
		SendBufferData[cnt++] = i;
		for(j = 0; j < len; j++)
		{
			SendBufferData[cnt++] = t_string[AttrMinorIndex][j];
		}
		SendBufferDataLen = cnt;
	}

	WriteRingBufferSend(SendBufferData, SendBufferDataLen);
	memset(SendBufferData, 0x00, SENDMAXLEN);
	return AE_OK;
}

/* ��ȡ���Ե��б���ʼ��demo_props */
void GetAttrList(unsigned char *DataBuffer)
{
	unsigned char TempIndex = 0x00;
	unsigned char IsToDevAttr = 0x00;	/* ��־���������to device�� */
	int cnt = 0;
	int sub = 0;
	
	//��ȡ���Եķ���
	IsToDevAttr = DataBuffer [cnt];
	cnt++;

	//��ȡĿǰ�ǵڼ�������
	TempIndex = DataBuffer [cnt];
	cnt++;

	//��ȡ��������
	if(DataBuffer [cnt] == TYPE_BOOL)
	{
		if(IsToDevAttr == 0x01){						//to device
			demo_props[TempIndex].type = ATLV_BOOL;
			demo_props[TempIndex].val = &t_bool[t_bool_index];
			demo_props[TempIndex].val_len = sizeof(t_bool[t_bool_index]);
			demo_props[TempIndex].get = ada_sprop_get_bool;
			demo_props[TempIndex].set = t_property_set;
			
			AttrIndexList[TempIndex].MajorIndex = TempIndex;
			AttrIndexList[TempIndex].MinorIndex = t_bool_index;
			AttrIndexList[TempIndex].Type = TYPE_BOOL;
			t_bool_index++;
			
		}else if(IsToDevAttr == 0x00){					//from device
			demo_props[TempIndex].type = ATLV_BOOL;
			demo_props[TempIndex].val = &f_bool[f_bool_index];
			demo_props[TempIndex].val_len = sizeof(f_bool[f_bool_index]);
			demo_props[TempIndex].get = ada_sprop_get_bool;
			demo_props[TempIndex].set = NULL;

			AttrIndexList[TempIndex].MajorIndex = TempIndex;
			AttrIndexList[TempIndex].MinorIndex = f_bool_index;
			AttrIndexList[TempIndex].Type = TYPE_BOOL;
			f_bool_index++;
		}
	}

	else if(DataBuffer [cnt] == TYPE_INT)
	{
		if(IsToDevAttr == 0x01)
		{
			demo_props[TempIndex].type = ATLV_INT;
			demo_props[TempIndex].val = &t_int[t_int_index];
			demo_props[TempIndex].val_len = sizeof(t_int[t_int_index]);
			demo_props[TempIndex].get = ada_sprop_get_int;
			demo_props[TempIndex].set = t_property_set;

			AttrIndexList[TempIndex].MajorIndex = TempIndex;
			AttrIndexList[TempIndex].MinorIndex = t_int_index;
			AttrIndexList[TempIndex].Type = TYPE_INT;
			t_int_index++;
		}
		else if(IsToDevAttr == 0x00){
			demo_props[TempIndex].type = ATLV_INT;
			demo_props[TempIndex].val = &f_int[f_int_index];
			demo_props[TempIndex].val_len = sizeof(f_int[f_int_index]);
			demo_props[TempIndex].get = ada_sprop_get_int;
			demo_props[TempIndex].set = NULL;

			AttrIndexList[TempIndex].MajorIndex = TempIndex;
			AttrIndexList[TempIndex].MinorIndex = f_int_index;
			AttrIndexList[TempIndex].Type = TYPE_INT;
			f_int_index++;
		}
	}

	else if(DataBuffer [cnt] == TYPE_CENTS)
	{
		if(IsToDevAttr == 0x01)
		{
			demo_props[TempIndex].type = ATLV_CENTS;
			demo_props[TempIndex].val = &t_decimal[t_decimal_index];
			demo_props[TempIndex].val_len = sizeof(t_decimal[t_decimal_index]);
			demo_props[TempIndex].get = ada_sprop_get_int;
			demo_props[TempIndex].set = t_property_set;

			AttrIndexList[TempIndex].MajorIndex = TempIndex;
			AttrIndexList[TempIndex].MinorIndex = t_decimal_index;
			AttrIndexList[TempIndex].Type = TYPE_CENTS;
			t_decimal_index++;
		}else if(IsToDevAttr == 0x00)
		{
			demo_props[TempIndex].type = ATLV_CENTS;
			demo_props[TempIndex].val = &f_decimal[f_decimal_index];
			demo_props[TempIndex].val_len = sizeof(f_decimal[f_decimal_index]);
			demo_props[TempIndex].get = ada_sprop_get_int;
			demo_props[TempIndex].set = NULL;

			AttrIndexList[TempIndex].MajorIndex = TempIndex;
			AttrIndexList[TempIndex].MinorIndex = f_decimal_index;
			AttrIndexList[TempIndex].Type = TYPE_CENTS;
			f_decimal_index++;
		}
	}

	else if(DataBuffer [cnt] == TYPE_UTF8)
	{
		if(IsToDevAttr == 0x01)
		{
			demo_props[TempIndex].type = ATLV_UTF8;
			demo_props[TempIndex].val = &t_string[t_string_index][0];
			demo_props[TempIndex].val_len = sizeof(t_string[t_string_index]);
			demo_props[TempIndex].get = ada_sprop_get_string;
			demo_props[TempIndex].set = t_property_set;

			AttrIndexList[TempIndex].MajorIndex = TempIndex;
			AttrIndexList[TempIndex].MinorIndex = t_string_index;
			AttrIndexList[TempIndex].Type = TYPE_UTF8;
			t_string_index++;
		}else if(IsToDevAttr == 0x00)
		{
			demo_props[TempIndex].type = ATLV_UTF8;
			demo_props[TempIndex].val = &f_string[f_string_index][0];
			demo_props[TempIndex].val_len = sizeof(f_string[f_string_index]);
			demo_props[TempIndex].get = ada_sprop_get_string;
			demo_props[TempIndex].set = NULL;

			AttrIndexList[TempIndex].MajorIndex = TempIndex;
			AttrIndexList[TempIndex].MinorIndex = f_string_index;
			AttrIndexList[TempIndex].Type = TYPE_UTF8;
			f_string_index++;
		}
	}

	cnt++;
	sub = 0;

	//��ȡ������
	while((DataBuffer[cnt] != ASCII_SPACE) && (DataBuffer[cnt] != 0x00))
	{
		AttrNameTemp[TempIndex][sub] =  DataBuffer[cnt];
		cnt++;
		sub++;
	}
	demo_props[TempIndex].name = AttrNameTemp[TempIndex];
	AttrIndexList[TempIndex].AttrName = AttrNameTemp[TempIndex];
}

/* ����version�����Ժ�oem_host_version������ */
void GetDevHostVersion(void)
{
	demo_props[AttrNum].name = "oem_host_version";
	demo_props[AttrNum].type = ATLV_UTF8;
	demo_props[AttrNum].val = demo_host_version;
	demo_props[AttrNum].val_len = sizeof(demo_host_version);
	demo_props[AttrNum].get = ada_sprop_get_string;
	demo_props[AttrNum].set = NULL;
	AttrIndexList[AttrNum].MajorIndex = AttrNum;
	AttrIndexList[AttrNum].MinorIndex = f_string_index;
	AttrIndexList[AttrNum].Type = TYPE_UTF8;
	AttrIndexList[AttrNum].AttrName = "oem_host_version";
	f_string_index++;


	demo_props[AttrNum + 1].name = "version";
	demo_props[AttrNum + 1].type = ATLV_UTF8;
	demo_props[AttrNum + 1].val = version;
	demo_props[AttrNum + 1].val_len = sizeof(version);
	demo_props[AttrNum + 1].get = ada_sprop_get_string;
	demo_props[AttrNum + 1].set = NULL;
	AttrIndexList[AttrNum + 1].MajorIndex = AttrNum + 1;
	AttrIndexList[AttrNum + 1].MinorIndex = f_string_index;
	AttrIndexList[AttrNum + 1].Type = TYPE_UTF8;
	AttrIndexList[AttrNum + 1].AttrName = "version";
	f_string_index++;
}


/* ��ӡ�����б� */
void PrintfAttrList(void)
{
	int i = 0;

	printf("\n\nAll Recv attr:\n");
	while(demo_props[i].name != NULL)
	{
		printf("\t\t%s, \t\t\t\t%02x\n", demo_props[i].name, demo_props[i].type);
		i++;
	}
	printf("have %d attr\n\n", i);

	i = 0;
	printf("\n\nIndex List.\n");
	while(AttrIndexList[i].AttrName != NULL)
	{
		printf("\t\t%s, \t\t\t\t%d, \t%d, \t%02x\n", AttrIndexList[i].AttrName, AttrIndexList[i].MajorIndex, AttrIndexList[i].MinorIndex, AttrIndexList[i].Type);
		i++;
	}
	printf("have %d attr list\n\n", i);
}

/*
	Dir:���Է���
	AttrType:��������
	AttrNameIndex:��������λ��
	Len:����ֵ Data�ĳ���
	Data:���Ե�ֵ
*/
void RFLinkSendPropByName(unsigned char Dir, unsigned char AttrType, unsigned char AttrNameIndex, unsigned short Len, unsigned char *Data)
{
	unsigned char AttrNameMinorIndex = 0x00;	/* ���������ڵĴ����� */
	int err;
	unsigned char SendData[6];
	AttrNameMinorIndex = AttrIndexList[AttrNameIndex].MinorIndex;

	switch(AttrType){
		case TYPE_BOOL:
		{
			if(Dir == TO_DEVICE){
				t_bool[AttrNameMinorIndex] = Data[0];
			}else if(Dir == FROM_DEVICE){
				f_bool[AttrNameMinorIndex] = Data[0];
			}
			
			break;
		}

		case TYPE_INT:
		{
			if(Dir == TO_DEVICE){
				t_int[AttrNameMinorIndex] = (Data[0] << 24) | (Data[1] << 16) | (Data[2] << 8) | (Data[3] << 0);
			}else if(Dir == FROM_DEVICE){
				f_int[AttrNameMinorIndex] = (Data[0] << 24) | (Data[1] << 16) | (Data[2] << 8) | (Data[3] << 0);
			}
			break;
		}

		case TYPE_CENTS:
		{
			if(Dir == TO_DEVICE){
				t_decimal[AttrNameMinorIndex] = (Data[0] << 24) | (Data[1] << 16) | (Data[2] << 8) | (Data[3] << 0);
			}else if(Dir == FROM_DEVICE){
				f_decimal[AttrNameMinorIndex] = (Data[0] << 24) | (Data[1] << 16) | (Data[2] << 8) | (Data[3] << 0);
			}
			break;
		}

		case TYPE_UTF8:
		{
			if(Dir == TO_DEVICE){
				memset(t_string[AttrNameMinorIndex], 0x00, 1024);
				memcpy(t_string[AttrNameMinorIndex], Data, Len);
			}else if(Dir == FROM_DEVICE){
				memset(f_string[AttrNameMinorIndex], 0x00, 1024);
				memcpy(f_string[AttrNameMinorIndex], Data, Len);
			}
			break;
		}

		default:
			break;
	}


	if((AttrType == TYPE_UTF8) && (Dir == TO_DEVICE)){
		printf("UTF8 TO: Attr Send: %s, Val = %s\n", AttrIndexList[AttrNameIndex].AttrName, t_string[AttrNameMinorIndex]);
	}else if((AttrType == TYPE_UTF8) && (Dir == FROM_DEVICE)){
		printf("UTF8 From: Attr Send: %s, Val = %s\n", AttrIndexList[AttrNameIndex].AttrName, f_string[AttrNameMinorIndex]);
	}else{
		printf("Attr Send: %s, Val = %d\n", AttrIndexList[AttrNameIndex].AttrName, *(int *)demo_props[AttrNameIndex].val);
	}
	
	err = prop_send_by_name(AttrIndexList[AttrNameIndex].AttrName);
	SendData[0] = MCU_UP_PROPERTY_CMD;
	SendData[1] = err >> 24;
	SendData[2] = err >> 16;
	SendData[3] = err >> 8;
	SendData[4] = err >> 0;
	SendData[5] = AttrNameIndex;
	//WriteRingBufferSend(SendData, 6);
	xSemaphoreTake( xSemaphoreSend, portMAX_DELAY );
	UartSendFormData(SendData, 6);
	xSemaphoreGive( xSemaphoreSend );
	
}

void GetAttrAllValAndSend(unsigned char *DataBuffer)
{
	unsigned char TempIndex = 0x00;
	int TempVal = 0;
	unsigned short ValLen = 0;

	//��ȡĿǰ�ǵڼ�������
	TempIndex = DataBuffer [2];

	//��ȡ��������
	if(DataBuffer [1] == TYPE_BOOL)
	{
		*(unsigned char *)demo_props[TempIndex].val = DataBuffer[5];	//����ֵ
	}

	else if(DataBuffer [1] == TYPE_INT)
	{
		TempVal = (DataBuffer[5] << 24) | (DataBuffer[6] << 16) | (DataBuffer[7] << 8) | (DataBuffer[8] << 0);
		*(int *)demo_props[TempIndex].val = TempVal;
	}

	else if(DataBuffer [1] == TYPE_CENTS)
	{
		TempVal = (DataBuffer[5] << 24) | (DataBuffer[6] << 16) | (DataBuffer[7] << 8) | (DataBuffer[8] << 0);
		*(int *)demo_props[TempIndex].val = TempVal;
	}

	else if(DataBuffer [1] == TYPE_UTF8)
	{
		ValLen = (DataBuffer[3] << 8) | (DataBuffer[4] << 0);
		memcpy(demo_props[TempIndex].val, &DataBuffer[5], ValLen);
	}

	//������ֵ�ϴ����ƶ�
	prop_send_by_name(demo_props[TempIndex].name);
	vTaskDelay(10);
	if(DataBuffer [1] == TYPE_BOOL){
		printf("bool, Init %s Value, value = %d\n", demo_props[TempIndex].name, *(unsigned char *)demo_props[TempIndex].val);
	}else if(DataBuffer [1] == TYPE_INT){
		printf("int, Init %s Value, value = %d\n", demo_props[TempIndex].name, *(int *)demo_props[TempIndex].val);
	}else if(DataBuffer [1] == TYPE_CENTS){
		printf("cents, Init %s Value, value = %d\n", demo_props[TempIndex].name, *(int *)demo_props[TempIndex].val);
	}else if(DataBuffer [1] == TYPE_UTF8){
		printf("utf8 Init %s Value, value = %s\n", demo_props[TempIndex].name, demo_props[TempIndex].val);
	}

	/* �豸�ϵ�����ȫ�����µ��ƶ��������ñ�־λ���Ա��ƶ��ܹ��·����� */
	if(TempIndex >= (AttrNum - 1))
		IsGetDevStaOK = 1;
}

void MCU_ResetWIFI(void)
{
	int cnt = 0;
	unsigned char Data[1] = {0};
	
	Data[0] = RESET_WIFI_CMD;
	//WriteRingBufferSend(Data, 1);
	xSemaphoreTake( xSemaphoreSend, portMAX_DELAY );
	UartSendFormData(Data, 1);
	xSemaphoreGive( xSemaphoreSend );
	
	vTaskDelay(100);
	for(cnt = 0; cnt < 11; cnt++)
	{
		adw_wifi_profile_erase(cnt);
	}
	
	conf_save_config();
	printf("MCU erase Wifi profile ok.\n");
	ada_conf_reset(0);
}

void MCU_RestartWIFI(void)
{
	int cnt = 0;
	unsigned char Data[1] = {0};

	Data[0] = MCU_RESTART_WIFI_CMD;
	//WriteRingBufferSend(Data, 1);
	xSemaphoreTake( xSemaphoreSend, portMAX_DELAY );
	UartSendFormData(Data, 1);
	xSemaphoreGive( xSemaphoreSend );
	
	vTaskDelay(100);
	printf("MCU restart Wifi mode ok.\n");
	ada_conf_reset(0);
}

/* MCU��ȡʱ�� */
/* ����0Ϊ��ȡutcʱ�䣬 Ϊ1��ȡ����ʱ�� */
void MCU_GetNTPTime(unsigned char zone)
{
	unsigned int NTP_utc = 0;	//utcʱ��
	unsigned int NTP_local = 0;	//����ʱ��
	struct clock_info clk;
	unsigned char NTPBuffer[13] = {0};
	

	NTP_utc = clock_utc();	//��ȡutcʱ��
	NTP_local = clock_local(&NTP_utc);//��ȡ����ʱ��

	if(zone == 0x00)	/* ��ȡUTCʱ�� */
	{
		clock_fill_details(&clk, NTP_utc);
		NTPBuffer[0] = MCU_GET_NTP_TIME_CMD;
		NTPBuffer[1] = clk.year >> 8;
		NTPBuffer[2] = clk.year >> 0;
		NTPBuffer[3] = clk.month;
		NTPBuffer[4] = clk.days;
		NTPBuffer[5] = clk.day_of_week;
		NTPBuffer[6] = clk.hour;
		NTPBuffer[7] = clk.min;
		NTPBuffer[8] = clk.sec;
		NTPBuffer[9] = (NTP_utc >> 24) & 0xFF;
		NTPBuffer[10] = (NTP_utc >> 16) & 0xFF;
		NTPBuffer[11] = (NTP_utc >> 8) & 0xFF;
		NTPBuffer[12] = (NTP_utc >> 0) & 0xFF;
		//WriteRingBufferSend(NTPBuffer, 13);
		xSemaphoreTake( xSemaphoreSend, portMAX_DELAY );
		UartSendFormData(NTPBuffer, 13);
		xSemaphoreGive( xSemaphoreSend );
	}
	else if(zone == 0x01)	/* ��ȡ����ʱ�� */
	{
		clock_fill_details(&clk, NTP_local);
		NTPBuffer[0] = MCU_GET_NTP_TIME_CMD;
		NTPBuffer[1] = clk.year >> 8;
		NTPBuffer[2] = clk.year >> 0;
		NTPBuffer[3] = clk.month;
		NTPBuffer[4] = clk.days;
		NTPBuffer[5] = clk.day_of_week;
		NTPBuffer[6] = clk.hour;
		NTPBuffer[7] = clk.min;
		NTPBuffer[8] = clk.sec;
		NTPBuffer[9] = (NTP_local >> 24) & 0xFF;
		NTPBuffer[10] = (NTP_local >> 16) & 0xFF;
		NTPBuffer[11] = (NTP_local >> 8) & 0xFF;
		NTPBuffer[12] = (NTP_local >> 0) & 0xFF;
		//WriteRingBufferSend(NTPBuffer, 13);
		xSemaphoreTake( xSemaphoreSend, portMAX_DELAY );
		UartSendFormData(NTPBuffer, 13);
		xSemaphoreGive( xSemaphoreSend );
	}
}

void Get_WIFIRSSI(void)
{
	int WifiSignal;	//WIFI���ź�����
	unsigned char SendBuff[5] = {0x00};
	
	adap_net_get_signal(&WifiSignal);	//��ȡ�ź�����
	SendBuff[0] = MCU_GET_WIFI_RSSI_CMD;
	SendBuff[1] = (WifiSignal >> 24) & 0xFF;
	SendBuff[2] = (WifiSignal >> 16) & 0xFF;
	SendBuff[3] = (WifiSignal >> 8) & 0xFF;
	SendBuff[4] = (WifiSignal >> 0) & 0xFF;
	
	//WriteRingBufferSend(SendBuff, 5);
	xSemaphoreTake( xSemaphoreSend, portMAX_DELAY );
	UartSendFormData(SendBuff, 5);
	xSemaphoreGive( xSemaphoreSend );
}


/*
 * Initialize property manager.
 */
void demo_init(void)
{
	ada_sprop_mgr_register("ledevb", demo_props, ARRAY_LEN(demo_props));
}

void demo_idle(void)
{

	vTaskDelay(500);
	log_thread_id_set(TASK_LABEL_DEMO);
	taskstat_dbg_start();

	strcat(version, "HV:");
	strcat(version, HardVersion);
	strcat(version, " ");
	strcat(version, "SV:");
	strcat(version, SoftVersion);
	strcat(version, BUILD_STRING);

	printf("version:%s\n", version);

	prop_send_by_name("oem_host_version");
	prop_send_by_name("version");

	while (1) 
	{
		vTaskDelay(100);

		/* ��ȡ�豸״̬,���ϴ����ƶ� */
		if((ada_sprop_dest_mask & NODES_ADS) && (ada_conf.reg_user != 0) && (RFLinkWIFIStatus & 0x02))
		{
			if(IsGetDevStaOK == 0)
			{
				vTaskDelay(3000);
				GetDEVAllAttrVal();
				vTaskDelay(2000);
				//IsGetDevStaOK = 1;
			}
		}
		
		if((ada_sprop_dest_mask & NODES_ADS)){		/* ���Ƴɹ�Bit2����Ϊ1 */
			RFLinkWIFIStatus |= 0x04;
		}
		else{									/* ����ʧ��Bit2����Ϊ0 */
			IsGetDevStaOK = 0;
			RFLinkWIFIStatus &= ~(0x04);
		}

		if(ada_conf.reg_user != 0){					/* �󶨳ɹ�Bit3����Ϊ1 */
			RFLinkWIFIStatus |= 0x08;
		}else{
			IsGetDevStaOK = 0;
			RFLinkWIFIStatus &= ~(0x08);				/* ��ʧ��Bit3����Ϊ0 */
		}

		/* ����·��ʧ�� */
		if((RFLinkWIFIStatus & 0x02) == 0)
		{
			IsGetDevStaOK = 0;
		}
		
	}
}

