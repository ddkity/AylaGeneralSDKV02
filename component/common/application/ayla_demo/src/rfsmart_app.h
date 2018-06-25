#ifndef __RFSMART_APP_H__
#define __RFSMART_APP_H__

/* 串口帧结构定义 */
#define F_HEAD_H  		(0xFF)	/* 帧头 */
#define F_HEAD_L  		(0xFF)	/* 帧尾 */
#define UARTNOP  		(0)           /*  串口接收错误或空闲	*/
#define UARTSOP_H  		(1)           /*  接收起始位		*/
#define UARTSOP_L  		(2)           /*  接收起始位		*/
#define UARTLEN_H  		(3)           /*  接收长度			*/
#define UARTLEN_L  		(4)           /*  接收长度			*/
#define UARTCMD  		(5)           /*  接收命令			*/
#define UARTDATA 		(6)           /*  接收数据			*/
#define UARTCRC  		(7)           /*  数据长度为0 		*/

#define SENDMAXLEN  (1088)
#define RECVMAXLEN  (1088)
#define ASCII_SPACE	 (0x20)	//SPACE

/* 循环buffer的相关定义 */
/* 发送循环buffer */
#define SENDBUFFERMAXLEN (10)	/* 定义发送循环buffer的大小 */
struct RingSendBuffer_t{
	unsigned short Length;		/* Data数据的长度 */
	unsigned char Data[SENDMAXLEN];	/* 缓冲数据实体，包含一帧数据的CMD字段和数据字段 */
};

/* 接收循环buffer */
#define RECVBUFFERMAXLEN (10)	/* 定义发送循环buffer的大小 */
struct RingRecvBuffer_t{
	unsigned short Length;		/* Data数据的长度 */
	unsigned char CMD;
	unsigned char Data[RECVMAXLEN];	/* 缓冲数据实体，包含一帧数据的全部字段 */
};


/* 业务逻辑CMD字段定义 */
#define GET_DEV_INFO_CMD 			(0x01)			//获取设备信息
#define GET_DEV_INFO_CMD_REQ 		(0x02)
#define WIFI_CTRL_DEV_CMD 			( 0x03)			//WIFI模组控制设备
#define WIFI_CTRL_DEV_CMD_REQ 		( 0x04)
#define WIFI_READ_DEV_STA_CMD 		( 0x05)			//WIFI模组读取设备状态
#define WIFI_READ_DEV_STA_CMD_REQ  	(0x06)
#define MCU_UP_PROPERTY_CMD		(0x07)			//MCU主动上报当前状态
#define MCU_UP_PROPERTY_CMD_REQ 	( 0x08)
#define HEARTBEAT_CMD 				( 0x09)			//心跳命令
#define HEARTBEAT_CMD_REQ 			( 0x0a)
#define RESET_WIFI_CMD 				( 0x0b)			//重置WIFI模组
#define RESET_WIFI_CMD_REQ 			( 0x0c)
#define WIFI_SNED_STA_CMD 			( 0x0d)			//推送wifi工作状态
#define WIFI_SNED_STA_CMD_REQ 		(0x0e)
#define MCU_RESTART_WIFI_CMD 		( 0x10)			//重启WIFI模组
#define MCU_RESTART_WIFI_CMD_REQ 	( 0x11)
#define WIFI_GET_DEV_ATTR_CMD 		( 0x12)			//获取设备属性列表
#define WIFI_GET_DEV_ATTR_CMD_REQ 	( 0x13)
#define MCU_GET_NTP_TIME_CMD		(0x14)	//获取NTP时间
#define MCU_GET_NTP_TIME_CMD_REQ	(0x15)
#define MCU_GET_WIFI_RSSI_CMD		(0x16)	//获取模组的信号强度
#define MCU_GET_WIFI_RSSI_CMD_REQ	(0x17)

/* 属性类型 */
#define TYPE_BOOL (0x00)
#define TYPE_INT (0x01)
#define TYPE_CENTS (0x02)
#define TYPE_UTF8 (0x03)

/* 属性方向相关定义 */
#define TO_DEVICE (0x01)
#define FROM_DEVICE (0x00)

/* 允许最大属性个数的定义 */
#define T_BOOL_NUM			(50)
#define F_BOOL_NUM			(50)
#define T_INT_NUM			(50)
#define F_INT_NUM			(50)
#define T_CENTS				(20)
#define F_CENTS				(20)
#define T_UTF8				(20)
#define F_UTF8				(22)		/* 多加version和host version两个属性 */
#define ALL_TYPE_NUM 	(T_BOOL_NUM +T_INT_NUM + T_CENTS +T_UTF8 + F_BOOL_NUM +F_INT_NUM + F_CENTS +F_UTF8)

struct AttrIndexList_t {
	unsigned char MajorIndex;	//主索引
	unsigned char MinorIndex;	//次索引
	unsigned char Type;		//属性类型
	char *AttrName;			//属性名
};


extern unsigned char SendBufferData[SENDMAXLEN];
extern unsigned char AttrNum;	/* 属性个数 */
extern unsigned short SendBufferDataLen;

#endif

