#ifndef __RFSMART_APP_H__
#define __RFSMART_APP_H__

/* ����֡�ṹ���� */
#define F_HEAD_H  		(0xFF)	/* ֡ͷ */
#define F_HEAD_L  		(0xFF)	/* ֡β */
#define UARTNOP  		(0)           /*  ���ڽ��մ�������	*/
#define UARTSOP_H  		(1)           /*  ������ʼλ		*/
#define UARTSOP_L  		(2)           /*  ������ʼλ		*/
#define UARTLEN_H  		(3)           /*  ���ճ���			*/
#define UARTLEN_L  		(4)           /*  ���ճ���			*/
#define UARTCMD  		(5)           /*  ��������			*/
#define UARTDATA 		(6)           /*  ��������			*/
#define UARTCRC  		(7)           /*  ���ݳ���Ϊ0 		*/

#define SENDMAXLEN  (1088)
#define RECVMAXLEN  (1088)
#define ASCII_SPACE	 (0x20)	//SPACE

/* ѭ��buffer����ض��� */
/* ����ѭ��buffer */
#define SENDBUFFERMAXLEN (10)	/* ���巢��ѭ��buffer�Ĵ�С */
struct RingSendBuffer_t{
	unsigned short Length;		/* Data���ݵĳ��� */
	unsigned char Data[SENDMAXLEN];	/* ��������ʵ�壬����һ֡���ݵ�CMD�ֶκ������ֶ� */
};

/* ����ѭ��buffer */
#define RECVBUFFERMAXLEN (10)	/* ���巢��ѭ��buffer�Ĵ�С */
struct RingRecvBuffer_t{
	unsigned short Length;		/* Data���ݵĳ��� */
	unsigned char CMD;
	unsigned char Data[RECVMAXLEN];	/* ��������ʵ�壬����һ֡���ݵ�ȫ���ֶ� */
};


/* ҵ���߼�CMD�ֶζ��� */
#define GET_DEV_INFO_CMD 			(0x01)			//��ȡ�豸��Ϣ
#define GET_DEV_INFO_CMD_REQ 		(0x02)
#define WIFI_CTRL_DEV_CMD 			( 0x03)			//WIFIģ������豸
#define WIFI_CTRL_DEV_CMD_REQ 		( 0x04)
#define WIFI_READ_DEV_STA_CMD 		( 0x05)			//WIFIģ���ȡ�豸״̬
#define WIFI_READ_DEV_STA_CMD_REQ  	(0x06)
#define MCU_UP_PROPERTY_CMD		(0x07)			//MCU�����ϱ���ǰ״̬
#define MCU_UP_PROPERTY_CMD_REQ 	( 0x08)
#define HEARTBEAT_CMD 				( 0x09)			//��������
#define HEARTBEAT_CMD_REQ 			( 0x0a)
#define RESET_WIFI_CMD 				( 0x0b)			//����WIFIģ��
#define RESET_WIFI_CMD_REQ 			( 0x0c)
#define WIFI_SNED_STA_CMD 			( 0x0d)			//����wifi����״̬
#define WIFI_SNED_STA_CMD_REQ 		(0x0e)
#define MCU_RESTART_WIFI_CMD 		( 0x10)			//����WIFIģ��
#define MCU_RESTART_WIFI_CMD_REQ 	( 0x11)
#define WIFI_GET_DEV_ATTR_CMD 		( 0x12)			//��ȡ�豸�����б�
#define WIFI_GET_DEV_ATTR_CMD_REQ 	( 0x13)
#define MCU_GET_NTP_TIME_CMD		(0x14)	//��ȡNTPʱ��
#define MCU_GET_NTP_TIME_CMD_REQ	(0x15)
#define MCU_GET_WIFI_RSSI_CMD		(0x16)	//��ȡģ����ź�ǿ��
#define MCU_GET_WIFI_RSSI_CMD_REQ	(0x17)

/* �������� */
#define TYPE_BOOL (0x00)
#define TYPE_INT (0x01)
#define TYPE_CENTS (0x02)
#define TYPE_UTF8 (0x03)

/* ���Է�����ض��� */
#define TO_DEVICE (0x01)
#define FROM_DEVICE (0x00)

/* ����������Ը����Ķ��� */
#define T_BOOL_NUM			(50)
#define F_BOOL_NUM			(50)
#define T_INT_NUM			(50)
#define F_INT_NUM			(50)
#define T_CENTS				(20)
#define F_CENTS				(20)
#define T_UTF8				(20)
#define F_UTF8				(22)		/* ���version��host version�������� */
#define ALL_TYPE_NUM 	(T_BOOL_NUM +T_INT_NUM + T_CENTS +T_UTF8 + F_BOOL_NUM +F_INT_NUM + F_CENTS +F_UTF8)

struct AttrIndexList_t {
	unsigned char MajorIndex;	//������
	unsigned char MinorIndex;	//������
	unsigned char Type;		//��������
	char *AttrName;			//������
};


extern unsigned char SendBufferData[SENDMAXLEN];
extern unsigned char AttrNum;	/* ���Ը��� */
extern unsigned short SendBufferDataLen;

#endif

