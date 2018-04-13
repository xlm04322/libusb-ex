

#ifndef CDROM_H
#define CDROM_H


#ifdef __cplusplus
extern "C" {
#endif

/*COMMAND LIST  just define some used more*/
//WORKING DRAFT INFORMATION TECHNOLOGY -SCSI Multimedia Commands - 2 (MMC-2)
//Information technology -SCSI Primary Commands - 2 (SPC-2)
#define SCSI_CMD_TEST_UNIT_READY		  		0x00		//SPC	7.25
#define SCSI_CMD_REQUEST_SENSE  					0x03		//SPC	7.20
#define SCSI_CMD_FORMAT_UNIT					0x04		//SECTION 6.1.3
#define SCSI_CMD_INQUIRY							0x12		//SPC	7.3
#define SCSI_CMD_START_STOP_UNIT		  		0x1B		//SPC	
#define SCSI_CMD_PLAY_PREVENT_ALLOW_MEDIUMREMOVAL 	0x1E		//SPC	7.12
#define SCSI_CMD_READ_FORMAT_CAPACITIES  		0x23		//SECTION 6.1.20
#define SCSI_CMD_READ_CAPACITY  					0x25		//SECTION 6.1.17
#define SCSI_CMD_READ_10						0x28		//SBC
#define SCSI_CMD_WRITE_10						0x2A		//SECTION 6.1.41
#define SCSI_CMD_SEEK_10		  					0x2B		//SBC	
#define SCSI_CMD_ERASE_10						0x2C		//SBC
#define SCSI_CMD_WRITE_AND_VERIFY_10			0x2E		//SECTION 6.1.42
#define SCSI_CMD_VERIFY_10		  				0x2F		//SPC	
#define SCSI_CMD_SYNCHRONIZE_CACHE		  		0x35		//SECTION 6.1.40
#define SCSI_CMD_WRITE_BUFFER					0x3B		//SPC	7.26
#define SCSI_CMD_READ_BUFFER					0x3C		//SPC	7.13
#define SCSI_CMD_READ_SUB_CHANNEL  				0x42		//SECTION 6.1.23
#define SCSI_CMD_READ_TOC_PMA_ATIP  			0x43		//SECTION 6.1.24
#define SCSI_CMD_READ_HEADER  					0x44		//SECTION 6.1.21
#define SCSI_CMD_PLAY_AUDIO_10 					0x45		//SECTION 6.1.10
#define SCSI_CMD_GET_CONFIGRATION				0x46		//SECTION 6.1.4
#define SCSI_CMD_PLAY_AUDIO_MSF 				0x47		//SECTION 6.1.12
#define SCSI_CMD_GET_EVENT_OR_STATUS			0x4A		//SECTION 6.1.5
#define SCSI_CMD_PAUSE_RESUME					0x4B		//SECTION 6.1.9
#define SCSI_CMD_STOP_PLAY_SCAN		  			0x4E		//SECTION 6.1.39
#define SCSI_CMD_READ_DISC_INFORMATION  		0x51		//SECTION 6.1.18
#define SCSI_CMD_READ_TRACK_INFORMATION  		0x52		//SECTION 6.1.26
#define SCSI_CMD_RESERVE_TRACK  					0x53		//SECTION 6.1.29
#define SCSI_CMD_SEND_OPC_INFORMATION		  	0x54		//SECTION 6.1.35
#define SCSI_CMD_MODE_SELECT_10					0x55		//SPC	7.7
#define SCSI_CMD_REPAIR_TRACK  					0x58		//SECTION 6.1.27
#define SCSI_CMD_READ_MASTER_CUE  				0x59		//SECTION 6.1.22
#define SCSI_CMD_MODE_SENSE_10					0x5A		//SPC	7.9
#define SCSI_CMD_CLOSE_TRACK_OR_SESSION		0x5B		//SECTION 6.1.2
#define SCSI_CMD_READ_BUFFER_CAPACITY  			0x5C		//SECTION 6.1.14
#define SCSI_CMD_SEND_CUE_SHEET		  			0x5D		//SECTION 6.1.31
#define SCSI_CMD_BLAND							0xA1		//SECTION 6.1.1
#define SCSI_CMD_SEND_EVENT		  				0xA2		//SECTION 6.1.33
#define SCSI_CMD_SEND_KEY		  				0xA3		//SECTION 6.1.34
#define SCSI_CMD_REPORT_KEY  					0xA4		//SECTION 6.1.28
#define SCSI_CMD_PLAY_AUDIO_12 					0xA5		//SECTION 6.1.11
#define SCSI_CMD_LOAD_UNLOAD_MEDIUM			0xA6		//SECTION 6.1.7
#define SCSI_CMD_SET_READ_AHEAD		  			0xA7		//SECTION 6.1.37
#define SCSI_CMD_READ_12						0xA8		//SBC
#define SCSI_CMD_GET_PERFORMANCE				0xAC		//SECTION 6.1.6
#define SCSI_CMD_READ_DVD_STRUCTURE  			0xAD		//SECTION 6.1.19
#define SCSI_CMD_SEND_DVD_STRUCTURE		  	0xAD		//SECTION 6.1.32
#define SCSI_CMD_SET_STREAMING		  			0xB6		//SECTION 6.1.38
#define SCSI_CMD_READ_CD_MSF  					0xB9		//SECTION 6.1.16
#define SCSI_CMD_SCAN		  					0xBA		//SECTION 6.1.30
#define SCSI_CMD_SET_CD_SPEED		  			0xBB		//SECTION 6.1.36
#define SCSI_CMD_PLAY_CD 						0xBC		//SECTION 6.1.13
#define SCSI_CMD_MECHANISM_STATUS				0xBD		//SECTION 6.1.8
#define SCSI_CMD_READ_CD  						0xBE		//SECTION 6.1.15

//read cd cmd P173
#define READ_CD_SECTOR_TYPE_ALL					0x00	
#define READ_CD_SECTOR_TYPE_CD_DA				0x01
#define READ_CD_SECTOR_TYPE_MODE1				0x02
#define READ_CD_SECTOR_TYPE_MODE2_FORMLESS	0x03
#define READ_CD_SECTOR_TYPE_MODE2_FORM1		0x04
#define READ_CD_SECTOR_TYPE_MODE2_FORM2		0x05
//#define READ_CD_SECTOR_TYPE_RESERVED			0x05

#define CD_DA_BYTES_BY_ONE_FRAME		2352		//p38 p196
#define MODE1_BYTES_BY_ONE_FRAME		2048		//p38
#define MODE2_FORMLESS_BYTES_BY_ONE_FRAME		2336		//p38
#define MODE2_FORM1_BYTES_BY_ONE_FRAME		2048		//p38
#define MODE2_FORM2_BYTES_BY_ONE_FRAME		2328		//p38

#define TOC_RESP_MAX_LEN	(65535)	//0xFFFF
#define TOC_COMMAND_FORMAT_TOC	0x00
#define TOC_COMMAND_FORMAT_SESSION_INFO	0x01
#define TOC_COMMAND_FORMAT_FULL_TOC	0x02
#define TOC_COMMAND_FORMAT_PMA	0x03
#define TOC_COMMAND_FORMAT_ATIP	0x04
#define TOC_COMMAND_FORMAT_CD_TEXT	0x05

#define TOC_COMMAND_MSF(x)	((!!x)<<0x01)

#define TRACK_MAX_NUMBER	40

//xlm
//eg:CMD    12 00 00 00  24 00        INQUIRY
typedef struct
{
	uint8_t	operation_code;
	uint8_t	CmdDT_EVPD;	//CmdDT:bit1 EVPD:bit0 ;default 00
	uint8_t	page_operationCode;
	uint8_t	reserved;
	uint8_t	allocation_length;
	uint8_t	control;
}scsi_inquiry_cmd;
typedef struct
{
	uint8_t	peripheral_qualifier_deviceType;	//qualifier:bit7...5  deviceType:bit4...bit0	0
	uint8_t	RMB;	//rmb:bit7	1
	uint8_t	version;	//			2
	uint8_t	AERC_Ob_No_HiSup_RDF;	//aerc:bit7 Obsolete:bit6 NormACA:bit5 Hisup:bit4 response data format:bit3...bit0

	uint8_t	additional_length;	//(allocation_length-4)??additional_length;if == ok;if < should inquiry again;	4
	uint8_t	SCCS;	//SCCS:bit7
	uint8_t	BQUE_EncServ_VS;	//
	uint8_t	RelAdr;	//		7
	
	uint8_t	Vendor_Identification[8];	//	8...15
	uint8_t	Product_Identification[16];	//	16...31
	uint8_t	Product_REVISION_LEVEL[4];	//	32...35
	uint8_t	Vendor_specific[20];	//		36...55

	uint8_t	reseved1;	//		56
	uint8_t	reseved2;	//		57
	uint8_t	VERSION_DESCRIPTOR_N[8][2];	//		58..59 60..61 ... 72...73
	uint8_t	reseved3;	//		74
	uint8_t	*vendor_data;	// .....
	
	
}scsi_inquiry_resp;
typedef struct 
{
	uint8_t	operation_code;
	uint8_t 	MSFbit;	// bit1
	uint8_t 	fomat;	// bit3...0
	uint8_t 	reserved1;
	
	uint8_t 	reserved2;	
	uint8_t 	reserved3;	
	uint8_t 	track_or_session_number;	
	
	uint8_t 	allocation_length[2];		// MSB...LSB
	//uint8_t 	allocation_length_lsb;		// MSB...LSB
	
	uint8_t 	control;	
}scsi_toc_command;

// format 0000b
typedef struct
{
	uint8_t 	reserved;
	uint8_t 	ADR_CONTROL;	//adr:7...4   control 3...0
	uint8_t 	track_number;	// max 256 
	uint8_t 	reserved2;
	
	union{
		uint8_t 	logical_block_address[4];	// MSB...LSB	ifMSF bit=1  start_logical_block_add==>MSF
		uint8_t	MSF[4]; //MSB...LSB
	};
}scsi_toc_track_discriptor_format_0000b;
typedef struct
{
	uint8_t 	reserved;
	uint8_t 	ADR_CONTROL;	//adr:7...4   control 3...0
	uint8_t 	track_number;	// max 256  First Track Number In Last Complete Session (Hex)
	uint8_t 	reserved2;
	
	union{
		uint8_t 	logical_block_address[4];	// MSB...LSB Logical Block Address of First Track in Last Session
		uint8_t	MSF[4]; //MSB...LSB
	};
}scsi_toc_track_discriptor_format_0001b;
typedef struct
{
	uint8_t 	session_number;
	uint8_t 	ADR_CONTROL;	//adr:7...4   control 3...0
	uint8_t 	TNO;	// 
	uint8_t 	POINT;	// 
	
	uint8_t 	Min;
	uint8_t 	Sec;	//
	uint8_t 	Frame;	// 
	uint8_t 	Zero;	// 

	uint8_t 	PMIN;	//
	uint8_t 	PSEC;	// 
	uint8_t 	PFRAME;	// 
}scsi_toc_track_discriptor_format_0010b;
typedef struct
{
	uint8_t 	reserved;
	uint8_t 	ADR_CONTROL;	//adr:7...4   control 3...0
	uint8_t 	TNO;	// 
	uint8_t 	POINT;	// 
	
	uint8_t 	Min;
	uint8_t 	Sec;	//
	uint8_t 	Frame;	// 
	uint8_t 	Zero;	// 

	uint8_t 	PMIN;	//
	uint8_t 	PSEC;	// 
	uint8_t 	PFRAME;	// 
}scsi_toc_track_discriptor_format_0011b;
typedef struct 
{
	uint8_t	toc_data_length[2];	// MSB...LSB
	uint8_t 	first_track_number;
	uint8_t 	last_track_number;
	union{
		scsi_toc_track_discriptor_format_0000b	descriptor_0000b[TRACK_MAX_NUMBER];	// 256*8 =2048   65535/8=8192
		scsi_toc_track_discriptor_format_0001b	descriptor_0001b;	// 256*8 =2048   65535/8=8192
		scsi_toc_track_discriptor_format_0010b	descriptor_0010b[TRACK_MAX_NUMBER];	// 256*8 =2048   65535/8=8192
		scsi_toc_track_discriptor_format_0011b	descriptor_0011b[TRACK_MAX_NUMBER];	// 256*8 =2048   65535/8=8192
	};
}scsi_toc_resp;
// format 0001b
typedef struct
{
	uint8_t 	reserved;
	uint8_t 	ADR_CONTROL;	//adr:7...4   control 3...0
	uint8_t 	track_session_number;	// max 256 
	uint8_t 	reserved2;
	
	uint8_t 	logical_block_address[4];	// MSB...LSB
}scsi_toc_session_discriptor;
typedef struct 
{
	uint8_t	toc_session_data_length[2];
	uint8_t 	first_session_number;
	uint8_t 	last_session_number;
	scsi_toc_session_discriptor	descriptor[TOC_RESP_MAX_LEN/8];	// 256*8 =2048   65535/8=8192
}scsi_toc_session_resp;

// format 0010b
typedef struct
{
	uint8_t 	session_number;
	uint8_t 	ADR_CONTROL;	//adr:7...4   control 3...0
	uint8_t 	TNO;	// max 256 
	uint8_t 	POINT;
	
	uint8_t 	Min;
	uint8_t 	Sec;	//adr:7...4   control 3...0
	uint8_t 	Frame;	// max 256 
	uint8_t 	Zero;
	
	uint8_t 	PMin;
	uint8_t 	PSec;	//adr:7...4   control 3...0
	uint8_t 	PFrame;	// max 256 
	
}scsi_toc_full_discriptor;
typedef struct 
{
	uint8_t	toc_full_data_length[2];
	uint8_t 	first_full_number;
	uint8_t 	last_full_number;
	scsi_toc_full_discriptor	descriptor;	// 256*8 =2048   65535/8=8192
}scsi_toc_full_resp;

// format 0011b
typedef struct
{
	uint8_t 	reserved;
	uint8_t 	ADR_CONTROL;	//adr:7...4   control 3...0
	uint8_t 	TNO;	// max 256 
	uint8_t 	POINT;
	
	uint8_t 	Min;
	uint8_t 	Sec;	//adr:7...4   control 3...0
	uint8_t 	Frame;	// max 256 
	uint8_t 	Zero;
	
	uint8_t 	PMin;
	uint8_t 	PSec;	//adr:7...4   control 3...0
	uint8_t 	PFrame;	// max 256 
	
}scsi_toc_pma_discriptor;
typedef struct 
{
	uint8_t	pma_data_length[2];
	uint8_t 	reserved1;
	uint8_t 	reserved2;
	scsi_toc_pma_discriptor	descriptor;	// 256*8 =2048   65535/8=8192
}scsi_toc_pma_resp;


typedef struct 
{
	union {
		scsi_toc_resp toc_resp;
		scsi_toc_resp toc_session_resp;
		scsi_toc_resp toc_full_resp;
	};
}scsi_toc_union_resp;


typedef struct
{
	uint8_t	operation_code;	//0xbe
	uint8_t	sectorType_reladr;		// secotorType:bit4..2  reladr:bit0
	uint8_t	start_logical_block_add[4];	//MSB...LSB  ifMSF bit=1  start_logical_block_add==>MSF
	uint8_t	transfer_length[3];		//MSB...LSB
	uint8_t	field_control;	//sync:bit7  header_codes:bit6..5  user_data:bit4 EDC&ECC bit3 error:bit2..1 
	uint8_t	Sub_channel_Selection_Bits;	// bit2...0
	uint8_t	control;
}scsi_read_cd_command;

typedef struct
{
	libusb_device_handle *handle;
	uint8_t			nb_ifaces;
	uint8_t			lun;	// default 0
	uint8_t			endpoint_in;
	uint8_t			endpoint_out;
}scsi_device_para;
typedef struct
{
	uint8_t			track_number;
	uint8_t			start_logical_block_add[4];	// MSB...LSB
	uint8_t			transfer_length[4];			// MSB...LSB
	uint8_t			time_minute;	// 
	uint8_t			time_second;	//
	uint8_t			time_frame;	// 1second = 75 frames
	
}track_information;
typedef struct
{
	uint8_t 			track_total;
	track_information	track_info[TRACK_MAX_NUMBER];
}scsi_track_para;

#ifdef __cplusplus
}
#endif

#endif
