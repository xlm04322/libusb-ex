

#include "com_example_brooklynxu_myapplication_MainActivity.h"


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include <unistd.h>

#define msleep(msecs) usleep(1000*msecs)


#if !defined(bool)
#define bool int
#endif
#if !defined(true)
#define true (1 == 1)
#endif
#if !defined(false)
#define false (!true)
#endif


#include "libusb.h"
#include "libcdrom.h"
#include "stdio.h"
//#ifdef __ANDROID__
#include <android/log.h>
//#endif
//#include <cutils/Log.h>
//#include <utils/Log.h>


#define usb_interface interface

// Global variables
static bool binary_dump = false;
static bool extra_info = false;
static bool force_device_request = false;	// For WCID descriptor queries
static const char* binary_name = NULL;

static int perr(char const *format, ...)
{
	va_list args;
	int r;

	va_start (args, format);
	r = vfprintf(stderr, format, args);
	va_end(args);

	return r;
}

#define ERR_EXIT(errcode) do { perr("   %s\n", libusb_strerror((enum libusb_error)errcode)); return -1; } while (0)
#define CALL_CHECK(fcall) do { r=fcall; if (r < 0) ERR_EXIT(r); } while (0);
#define B(x) (((x)!=0)?1:0)
#define be_to_int32(buf) (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|(buf)[3])

#define RETRY_MAX                     5
#define REQUEST_SENSE_LENGTH          0x12
#define INQUIRY_LENGTH                0x24
#define READ_CAPACITY_LENGTH          0x08

// HID Class-Specific Requests values. See section 7.2 of the HID specifications
#define HID_GET_REPORT                0x01
#define HID_GET_IDLE                  0x02
#define HID_GET_PROTOCOL              0x03
#define HID_SET_REPORT                0x09
#define HID_SET_IDLE                  0x0A
#define HID_SET_PROTOCOL              0x0B
#define HID_REPORT_TYPE_INPUT         0x01
#define HID_REPORT_TYPE_OUTPUT        0x02
#define HID_REPORT_TYPE_FEATURE       0x03

// Mass Storage Requests values. See section 3 of the Bulk-Only Mass Storage Class specifications
#define BOMS_RESET                    0xFF
#define BOMS_GET_MAX_LUN              0xFE

// Section 5.1: Command Block Wrapper (CBW)
struct command_block_wrapper {
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
};

// Section 5.2: Command Status Wrapper (CSW)
struct command_status_wrapper {
	uint8_t dCSWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
};

static uint8_t cdb_length[256] = {
//	 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  0
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  1
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  2
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  3
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  4
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  5
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  6
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  7
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  8
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  9
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  A
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  B
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  C
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  D
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  E
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  F
};


static enum test_type {
	USE_GENERIC,
	USE_PS3,
	USE_XBOX,
	USE_SCSI,
	USE_HID,
} test_mode;

//#define LIBCDROM_DEBUG
#define libCdrom_log_err(...)		__android_log_print(ANDROID_LOG_ERROR,"libcdrom-jni",__VA_ARGS__);
#define libCdrom_log_debug(...)		__android_log_print(ANDROID_LOG_DEBUG,"libcdrom-jni",__VA_ARGS__);

static uint16_t VID, PID;

static scsi_device_para cdrom_para;
static scsi_track_para track_para;

#define AUTO_DETACH_KERNEL	0

scsi_inquiry_resp scsi_inquiry_r;
static int send_mass_storage_command(libusb_device_handle *handle, uint8_t endpoint, uint8_t lun,
	uint8_t *cdb, uint8_t direction, int data_length, uint32_t *ret_tag);
static int get_mass_storage_status(libusb_device_handle *handle, uint8_t endpoint, uint32_t expected_tag);
static void get_sense(libusb_device_handle *handle, uint8_t endpoint_in, uint8_t endpoint_out);
static void display_buffer_hex(unsigned char *buffer, unsigned size);

static int cmd_scsi_inquiry(libusb_device_handle *handle, uint8_t endpoint_in,uint8_t endpoint_out, uint8_t lun)
{
	int ret;
	uint8_t cdb[16] = {0};	// SCSI Command Descriptor Block
	uint8_t *buffer = NULL;
	uint32_t expected_tag;
	int size = 0;
	int i = 0;
	char vid[9], pid[9], rev[5];

	// Send Inquiry
	printf("scsi_inquiry:\n");
	buffer = malloc(INQUIRY_LENGTH);
	if(buffer == NULL)
	{
		printf("buffer malloc fail [%s , %d]\n",__FUNCTION__,__LINE__);
		return -1;
	}
	memset(buffer, 0, INQUIRY_LENGTH);
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = 0x12;	// Inquiry
	cdb[4] = INQUIRY_LENGTH;

	send_mass_storage_command(handle, endpoint_out, lun, cdb, LIBUSB_ENDPOINT_IN, INQUIRY_LENGTH, &expected_tag);
	ret = libusb_bulk_transfer(handle, endpoint_in, (unsigned char*)&buffer, INQUIRY_LENGTH, &size, 1000);
	if(ret < 0)
	{
		printf("request inquiry failed [%s , %d]\n",__FUNCTION__,__LINE__);
		free(buffer);
		goto FAIL;
	}
	printf("   received %d bytes\n", size);
	
	scsi_inquiry_r.additional_length = buffer[4] +4;

	// The following strings are not zero terminated
	for (i=0; i<8; i++) {
		scsi_inquiry_r.Vendor_Identification[i] = buffer[8+i];
		scsi_inquiry_r.Product_Identification[i*2] = buffer[16+i];
		scsi_inquiry_r.Product_Identification[i*2+1] = buffer[16+i+1];
		scsi_inquiry_r.Product_REVISION_LEVEL[i/2] = buffer[32+i/2];	// instead of another loop
	}
	memcpy(vid,scsi_inquiry_r.Vendor_Identification,8);
	memcpy(pid,scsi_inquiry_r.Product_Identification,16);
	memcpy(rev,scsi_inquiry_r.Product_REVISION_LEVEL,4);
	vid[8] = 0;
	pid[8] = 0;
	rev[4] = 0;
	printf("   VID:PID:REV \"%8s\":\"%16s\":\"%4s\"\n", vid, pid, rev);
	if (get_mass_storage_status(handle, endpoint_in, expected_tag) == -2) {
		get_sense(handle, endpoint_in, endpoint_out);
	}
	free(buffer);
	buffer = NULL;
	
	// inquiry again
	if(scsi_inquiry_r.additional_length > INQUIRY_LENGTH)
	{
		printf("scsi_inquiry again:\n");
		buffer = malloc(scsi_inquiry_r.additional_length);
		if(buffer == NULL)
		{
			printf("buffer malloc fail [%s , %d]\n",__FUNCTION__,__LINE__);
			return -1;
		}
		memset(buffer, 0, scsi_inquiry_r.additional_length);
		memset(cdb, 0, sizeof(cdb));
		cdb[0] = 0x12;	// Inquiry
		cdb[4] = scsi_inquiry_r.additional_length;

		send_mass_storage_command(handle, endpoint_out, lun, cdb, LIBUSB_ENDPOINT_IN, INQUIRY_LENGTH, &expected_tag);
		ret = libusb_bulk_transfer(handle, endpoint_in, (unsigned char*)&buffer, INQUIRY_LENGTH, &size, 1000);
		if(ret < 0)
		{
			printf("request inquiry failed [%s , %d]\n",__FUNCTION__,__LINE__);
			free(buffer);
			goto FAIL;
		}
		if (get_mass_storage_status(handle, endpoint_in, expected_tag) == -2) {
			get_sense(handle, endpoint_in, endpoint_out);
		}

		printf("   received %d bytes\n", size);
		free(buffer);
		buffer = NULL;

	}

	return 0;
FAIL:
	return -1;
}

// eject cd
static int cmd_scsi_start_stop_unit(libusb_device_handle *handle, uint8_t endpoint_in,uint8_t endpoint_out, uint8_t lun)
{
	int ret;
	uint8_t cdb[16] = {0};	// SCSI Command Descriptor Block
	uint32_t expected_tag;
	int size = 0;
	int i = 0;

	// CMD    1b 00 00 00  02 00        START UNIT
	printf("cmd_scsi_start_stop_unit:\n");
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = SCSI_CMD_START_STOP_UNIT;	// START UNIT
	cdb[4] = 0x02;	// LoEj:1 support load & unload Start:0->unload 1->load

	send_mass_storage_command(handle, endpoint_out, lun, cdb, LIBUSB_ENDPOINT_OUT, 0, &expected_tag);
	sleep(4);
	if (get_mass_storage_status(handle, endpoint_in, expected_tag) == -2) {
		get_sense(handle, endpoint_in, endpoint_out);
	}

	return 0;
}
//CMD    be 04 00 00  cc 0b 00 00  READ CD
//		01 f0 00 00
//sectorType:001->CD_DA Starting Logical Block Address->00 00 cc 0b
//Transfer Length->00 00 01   
//field_control->f0
//SYNC->1  Header Codes->11 User Data->1
//Sub_channel_Selection_Bits 0
//in SCSI Multimedia Commands - 2 (MMC-2) P200 Table 225 - Number of Bytes Returned Based on Data Selection Field
//Data to be transferred    				Byte9  CD-DA   Mode1  Mode2  Mode2Form1  Mode2Form2
//User Data                       				10h     2352     2048     2336    2048              2328
//	................................................................................................................
//All Headers & user data + EDC/ECC 		78h     2352     2340     2340    2340              2340
//.....................................................................................................................
static int cmd_scsi_read_cd(libusb_device_handle *handle, uint8_t endpoint_in,uint8_t endpoint_out, uint8_t lun, 
		uint32_t start_block_add, uint32_t block_len, uint8_t *buffer, uint32_t buffer_len)
{
	int ret_status;
	int i,j = 0;
	unsigned int track_block_size = block_len;
	unsigned int track_read_block = block_len;
	//unsigned int track_read_size = track_read_block*2048;
	unsigned int track_read_size = track_read_block*CD_DA_BYTES_BY_ONE_FRAME;  // cd-da length
	unsigned int track_read_add = start_block_add;
	uint8_t cdb[16] = {0};	// SCSI Command Descriptor Block
	int ret;
	uint32_t expected_tag;
	int size = 0;
	
	//read track1 music data
	if(buffer == NULL)
	{
		libCdrom_log_err("[%s,  %d] buffer is NULL return\n",__FUNCTION__,__LINE__);
		return -1;
	}
	//if(buffer_len < track_read_size)
	//{
	//	printf("[%s,  %d] buffer_len is less than track_read_size[%d,  %d], will occur overflow\n",__FUNCTION__,__LINE__,buffer_len,track_read_size);
	//	return -2;
	//}
	memset(buffer, 0, buffer_len);
	//for(i=0;i<track_block_size;i+=track_read_block)
	//{
		memset(cdb, 0, sizeof(cdb));
		cdb[0] = 0xbe;
		cdb[1] = READ_CD_SECTOR_TYPE_CD_DA<<2;//0x04;
		cdb[2] = (unsigned char)((track_read_add)>>24)&0xFF;
		cdb[3] = (unsigned char)((track_read_add)>>16)&0xFF;
		cdb[4] = (unsigned char)((track_read_add)>>8)&0xFF;
		cdb[5] = (unsigned char)((track_read_add)>>0)&0xFF;
		cdb[6] = 0;
		cdb[7] = 0;
		cdb[8] = track_read_block;
		//cdb[9] = 0xf0;
		cdb[9] = 0x78;
		
		#ifdef LIBCDROM_DEBUG
		libCdrom_log_err("==>read add:[%02x %02x %02x %02x]\n",cdb[2],cdb[3],cdb[4],cdb[5]);
		#endif
		send_mass_storage_command(handle, endpoint_out, lun, cdb, LIBUSB_ENDPOINT_IN, track_read_size, &expected_tag);
		libusb_bulk_transfer(handle, endpoint_in, buffer, track_read_size, &size, 5000);
		#ifdef LIBCDROM_DEBUG
		libCdrom_log_err("   READ CD: received %d bytes\n", size);
		#endif
		//sleep(3);
		if (get_mass_storage_status(handle, endpoint_in, expected_tag) == -2) {
			get_sense(handle, endpoint_in, endpoint_out);
			return -1;
		}
		else
		{
			//display_buffer_hex(data, size);
		}

		#ifdef READ_CD_DEBUG
		j++;
		if(j==10)
		{
			//break;
		}
		#endif
	//}
	
	
	return size;
}

static int cmd_scsi_read_capacity(libusb_device_handle *handle, uint8_t endpoint_in,uint8_t endpoint_out, uint8_t lun)
{
	uint8_t buffer[64];
	uint32_t i, max_lba, block_size;
	double device_size;
	uint8_t cdb[16] = {0};	// SCSI Command Descriptor Block
	int ret;
	uint32_t expected_tag;
	int size = 0;

	
	// Read capacity
	printf("Reading Capacity:\n");
	memset(buffer, 0, sizeof(buffer));
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = SCSI_CMD_READ_CAPACITY;	// Read Capacity

	send_mass_storage_command(handle, endpoint_out, lun, cdb, LIBUSB_ENDPOINT_IN, READ_CAPACITY_LENGTH, &expected_tag);
	ret = libusb_bulk_transfer(handle, endpoint_in, (unsigned char*)&buffer, READ_CAPACITY_LENGTH, &size, 1000);
	printf("   received %d bytes\n", size);
	if(ret != 0)
	{
		printf("[%s,  %d] cdrom maybe ont insert \n",__FUNCTION__,__LINE__);
		return -1;
	}
	max_lba = be_to_int32(&buffer[0]);
	block_size = be_to_int32(&buffer[4]);
	device_size = ((double)(max_lba+1))*block_size/(1024*1024*1024);
	printf("   Max LBA: %08X, Block Size: %08X (%.2f GB)\n", max_lba, block_size, device_size);
	if (get_mass_storage_status(handle, endpoint_in, expected_tag) == -2) {
		get_sense(handle, endpoint_in, endpoint_out);
	}

	return 0;
}

static int cmd_scsi_read_disc_information(libusb_device_handle *handle, uint8_t endpoint_in,uint8_t endpoint_out, uint8_t lun)
{
	uint8_t buffer[64];
	uint32_t i, max_lba, block_size;
	double device_size;
	uint8_t cdb[16] = {0};	// SCSI Command Descriptor Block
	int ret;
	uint32_t expected_tag;
	int size = 0;

	// Read capacity
	printf("cmd_scsi_read_disc_information:\n");
	memset(buffer, 0, sizeof(buffer));
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = SCSI_CMD_READ_DISC_INFORMATION;	// cmd_scsi_read_disc_information

	return 0;
	
}

// MSF:1->read minute&second&fram(1second=75frames)  0->read logical add   2->read both
//CMD    43 02 00 00  00 00 00 03  READ TOC/PMA 
//24 00
// bit3:00 format Type just equal to 00  ,other format TBD
// bit2:00 MSF=0;  02 MSF=1;
static int cmd_scsi_read_toc_pma_atip(libusb_device_handle *handle, uint8_t endpoint_in,uint8_t endpoint_out, uint8_t lun,
								int MSF, scsi_track_para *track_para)
{
	int i;
	uint32_t tmp_add,tmp_next_add,tmp_len;
	uint32_t tmp_min,tmp_next_min,tmp_sec,tmp_next_sec,tmp_time_len;
	int read_flag = MSF;
	uint8_t *data = NULL;
	uint8_t cdb[16] = {0};	// SCSI Command Descriptor Block
	int ret;
	uint32_t expected_tag;
	int size = 0;

	printf("=========>xlm\n");
	if(read_flag > 1)
		read_flag = 2;
	scsi_toc_union_resp resp;
	scsi_toc_command  command;
	data = (unsigned char*) malloc( TOC_RESP_MAX_LEN);
	if(data == NULL)
	{
		printf("[%s,  %d] malloc fail \n",__FUNCTION__,__LINE__);
		return -1;
	}
TOC:
	memset(data, 0, (TOC_RESP_MAX_LEN));
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = SCSI_CMD_READ_TOC_PMA_ATIP;
	if(read_flag == 0)
	{
		cdb[1] = 0x00;
	}
	else if(read_flag == 1)
	{
		cdb[1] = 0x02;
	}
	else
	{
		cdb[1] = 0x02;
	}
	cdb[7] = 0x05;
	cdb[8] = 0x54;
	send_mass_storage_command(handle, endpoint_out, lun, cdb, LIBUSB_ENDPOINT_IN, cdb[7]<<8|cdb[8], &expected_tag);
	libusb_bulk_transfer(handle, endpoint_in, data, cdb[7]<<8|cdb[8], &size, 5000);
	if (get_mass_storage_status(handle, endpoint_in, expected_tag) == -2) 
	{
		get_sense(handle, endpoint_in, endpoint_out);
		free(data);
		return -1;
	} 
	else 
	{
		display_buffer_hex(data, size);
	}
	resp.toc_resp.toc_data_length[0] = data[0];
	resp.toc_resp.toc_data_length[1] = data[1];
	resp.toc_resp.first_track_number = data[2];
	resp.toc_resp.last_track_number = data[3];
	memcpy(&(resp.toc_resp.descriptor_0000b[0].reserved),&data[4],(resp.toc_resp.last_track_number+1)*8);
	if(track_para)
	{
		track_para->track_total = resp.toc_resp.last_track_number;
	}
	if(read_flag == 0)
	{
		printf("********** [read_flag : %d]\n",read_flag);
		for(i=0;i<resp.toc_resp.last_track_number;i++)
		{
			if(track_para)
			{
				track_para->track_info[i].track_number = resp.toc_resp.descriptor_0000b[i].track_number;
				track_para->track_info[i].start_logical_block_add[0] = resp.toc_resp.descriptor_0000b[i].logical_block_address[0];
				track_para->track_info[i].start_logical_block_add[1] = resp.toc_resp.descriptor_0000b[i].logical_block_address[1];
				track_para->track_info[i].start_logical_block_add[2] = resp.toc_resp.descriptor_0000b[i].logical_block_address[2];
				track_para->track_info[i].start_logical_block_add[3] = resp.toc_resp.descriptor_0000b[i].logical_block_address[3];

				tmp_add = track_para->track_info[i].start_logical_block_add[0]<<24 |
						track_para->track_info[i].start_logical_block_add[1]<<16 |
						track_para->track_info[i].start_logical_block_add[2]<<8 |
						track_para->track_info[i].start_logical_block_add[3];
				tmp_next_add = resp.toc_resp.descriptor_0000b[i+1].logical_block_address[0]<<24 |
						resp.toc_resp.descriptor_0000b[i+1].logical_block_address[1]<<16 |
						resp.toc_resp.descriptor_0000b[i+1].logical_block_address[2]<<8 |
						resp.toc_resp.descriptor_0000b[i+1].logical_block_address[3];
				tmp_len = tmp_next_add - tmp_add;
				track_para->track_info[i].transfer_length[0] = tmp_len>>24;
				track_para->track_info[i].transfer_length[1] = (tmp_len&0x00FF0000)>>16;
				track_para->track_info[i].transfer_length[2] = (tmp_len&0x0000FF00)>>8;
				track_para->track_info[i].transfer_length[3] = (tmp_len&0x000000FF);

				
				
			}
			printf("track number : [%d]\n",resp.toc_resp.descriptor_0000b[i].track_number);
			printf("track address MSB...LSB: [%02x %02x %02x %02x]  len: [0x%08x]\n",resp.toc_resp.descriptor_0000b[i].logical_block_address[0],
									resp.toc_resp.descriptor_0000b[i].logical_block_address[1],
									resp.toc_resp.descriptor_0000b[i].logical_block_address[2],
									resp.toc_resp.descriptor_0000b[i].logical_block_address[3],
									tmp_len);
		}
		printf("track number : [%d]\n",resp.toc_resp.descriptor_0000b[i].track_number);
		printf("track address MSB...LSB: [%02x %02x %02x %02x] \n",resp.toc_resp.descriptor_0000b[i].logical_block_address[0],
								resp.toc_resp.descriptor_0000b[i].logical_block_address[1],
								resp.toc_resp.descriptor_0000b[i].logical_block_address[2],
								resp.toc_resp.descriptor_0000b[i].logical_block_address[3]);
		printf("**********\n\n");
	}
	else //if(read_flag == 1)
	{
		printf("********** [read_flag : %d]\n",read_flag);
		for(i=0;i<resp.toc_resp.last_track_number;i++)
		{
			if(track_para)
			{
				track_para->track_info[i].track_number = resp.toc_resp.descriptor_0000b[i].track_number;
				if(resp.toc_resp.descriptor_0000b[i+1].MSF[2] >= resp.toc_resp.descriptor_0000b[i].MSF[2])
				{
					track_para->track_info[i].time_minute = resp.toc_resp.descriptor_0000b[i].MSF[1+1] - resp.toc_resp.descriptor_0000b[i].MSF[1];
					track_para->track_info[i].time_second = resp.toc_resp.descriptor_0000b[i+1].MSF[2] - resp.toc_resp.descriptor_0000b[i].MSF[2];
				}
				else
				{
					track_para->track_info[i].time_minute = resp.toc_resp.descriptor_0000b[i].MSF[1+1] - resp.toc_resp.descriptor_0000b[i].MSF[1] -1;
					track_para->track_info[i].time_second = resp.toc_resp.descriptor_0000b[i+1].MSF[2] + 60 - resp.toc_resp.descriptor_0000b[i].MSF[2];
				}
				track_para->track_info[i].time_frame = 0;
			}
			printf("track number : [%d]\n",resp.toc_resp.descriptor_0000b[i].track_number);
			printf("track Min Sec Frame MSB...LSB: [%02x %02x %02x %02x]\n",resp.toc_resp.descriptor_0000b[i].MSF[0],
									resp.toc_resp.descriptor_0000b[i].MSF[1],
									resp.toc_resp.descriptor_0000b[i].MSF[2],
									resp.toc_resp.descriptor_0000b[i].MSF[3]);
		}
		printf("track number : [%d]\n",resp.toc_resp.descriptor_0000b[i].track_number);
		printf("track Min Sec Frame MSB...LSB: [%02x %02x %02x %02x]\n",resp.toc_resp.descriptor_0000b[i].MSF[0],
								resp.toc_resp.descriptor_0000b[i].MSF[1],
								resp.toc_resp.descriptor_0000b[i].MSF[2],
								resp.toc_resp.descriptor_0000b[i].MSF[3]);
		printf("**********\n\n");
	}

	if(read_flag == 2)
	{
		read_flag = 0;
		goto TOC;
	}

	free(data);

	return 0;
}

int grab_track_by_number_to_file(int track_number, FILE *fd)
{
	int i = 0;
	int ret= 0;
	uint32_t start_block_add = 0;
	uint32_t block_len;
	uint8_t * buffer = NULL;
	uint32_t buffer_len = 0;

	#define READ_BLOCKS_ONETIME		0x10
	
	if(track_number < 1 || track_number > TRACK_MAX_NUMBER)
	{
		libCdrom_log_err("track number exceed limit\n");
		return -1;
	}

	if(track_para.track_total == 0)
	{
		libCdrom_log_err("track total is 0 , no track\n");
		return -1;
	}

	if(fd != NULL)
	{
		libCdrom_log_err("no save to file\n");
	}
	start_block_add = track_para.track_info[track_number-1].start_logical_block_add[0]<<24 |
					track_para.track_info[track_number-1].start_logical_block_add[1]<<16 |
					track_para.track_info[track_number-1].start_logical_block_add[2]<<8 |
					track_para.track_info[track_number-1].start_logical_block_add[3];
	block_len = track_para.track_info[track_number-1].transfer_length[0] << 24 |
				track_para.track_info[track_number-1].transfer_length[1] << 16 |
				track_para.track_info[track_number-1].transfer_length[2] << 8 |
				track_para.track_info[track_number-1].transfer_length[3];
	buffer_len = READ_BLOCKS_ONETIME * CD_DA_BYTES_BY_ONE_FRAME;	// maybe buffer_len is very long, consider mutiple read cd
	buffer = malloc(buffer_len);
	if(buffer == NULL)
	{	
		libCdrom_log_err("[%s,  %d] buffer is NULL return\n",__FUNCTION__,__LINE__);
		return -1;
	}
	for(i = start_block_add; i<block_len+start_block_add ;i+=READ_BLOCKS_ONETIME)
	{
		if((i+READ_BLOCKS_ONETIME) > (block_len+start_block_add))
		{
			ret = cmd_scsi_read_cd(cdrom_para.handle, cdrom_para.endpoint_in, cdrom_para.endpoint_out, cdrom_para.lun, i,  (block_len+start_block_add-i),  buffer,  buffer_len);
		}
		else
		{
			ret = cmd_scsi_read_cd(cdrom_para.handle, cdrom_para.endpoint_in, cdrom_para.endpoint_out, cdrom_para.lun, i,  READ_BLOCKS_ONETIME,  buffer,  buffer_len);
		}
		if(ret > 0)
		{
			if(fd != NULL)
			{
				if (fwrite(buffer, 1, (size_t)ret, fd) != (unsigned int)ret) {
					perr("   unable to write binary data\n");
				}
			}
			else
			{
//				libCdrom_log_err("no save to file\n");
			}
		}
		else
		{
			libCdrom_log_err("read cd error\n");
		}
	}

	libCdrom_log_err("grab_track_by_number_to_file [%d] success\n",track_number);
	return 0;
}


static void display_buffer_hex(unsigned char *buffer, unsigned size)
{
	unsigned i, j, k;

	for (i=0; i<size; i+=16) {
		printf("\n  %08x  ", i);
		for(j=0,k=0; k<16; j++,k++) {
			if (i+j < size) {
				printf("%02x", buffer[i+j]);
			} else {
				printf("  ");
			}
			printf(" ");
		}
		printf(" ");
		for(j=0,k=0; k<16; j++,k++) {
			if (i+j < size) {
				if ((buffer[i+j] < 32) || (buffer[i+j] > 126)) {
					printf(".");
				} else {
					printf("%c", buffer[i+j]);
				}
			}
		}
	}
	printf("\n" );
}

static char* uuid_to_string(const uint8_t* uuid)
{
	static char uuid_string[40];
	if (uuid == NULL) return NULL;
	sprintf(uuid_string, "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
		uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
	return uuid_string;
}


static int send_mass_storage_command(libusb_device_handle *handle, uint8_t endpoint, uint8_t lun,
	uint8_t *cdb, uint8_t direction, int data_length, uint32_t *ret_tag)
{
	static uint32_t tag = 1;
	uint8_t cdb_len;
	int i, r, size;
	struct command_block_wrapper cbw;

	if (cdb == NULL) {
		return -1;
	}

	if (endpoint & LIBUSB_ENDPOINT_IN) {
		perr("send_mass_storage_command: cannot send command on IN endpoint\n");
		return -1;
	}

	cdb_len = cdb_length[cdb[0]];
	if ((cdb_len == 0) || (cdb_len > sizeof(cbw.CBWCB))) {
		perr("send_mass_storage_command: don't know how to handle this command (%02X, length %d)\n",
			cdb[0], cdb_len);
		return -1;
	}

	memset(&cbw, 0, sizeof(cbw));
	cbw.dCBWSignature[0] = 'U';
	cbw.dCBWSignature[1] = 'S';
	cbw.dCBWSignature[2] = 'B';
	cbw.dCBWSignature[3] = 'C';
	*ret_tag = tag;
	cbw.dCBWTag = tag++;
	cbw.dCBWDataTransferLength = data_length;
	cbw.bmCBWFlags = direction;
	cbw.bCBWLUN = lun;
	// Subclass is 1 or 6 => cdb_len
	cbw.bCBWCBLength = cdb_len;
	memcpy(cbw.CBWCB, cdb, cdb_len);

	i = 0;
	do {
		// The transfer length must always be exactly 31 bytes.
		r = libusb_bulk_transfer(handle, endpoint, (unsigned char*)&cbw, 31, &size, 1000);
		//r = libusb_bulk_transfer(handle, endpoint, (unsigned char*)&cdb, cdb_len, &size, 1000);
		if (r == LIBUSB_ERROR_PIPE) {
			libusb_clear_halt(handle, endpoint);
		}
		i++;
	} while ((r == LIBUSB_ERROR_PIPE) && (i<RETRY_MAX));
	if (r != LIBUSB_SUCCESS) {
		perr("   send_mass_storage_command: %s\n", libusb_strerror((enum libusb_error)r));
		return -1;
	}

	printf("   sent %d CDB bytes\n", cdb_len);
	return 0;
}

static int test_unit_ready_command(libusb_device_handle *handle, uint8_t endpoint_in,uint8_t endpoint_out, uint8_t lun)
{
	uint8_t cdb[16];	// SCSI Command Descriptor Block
	uint8_t sense[18];
	uint32_t expected_tag;
	int size;
	int rc;

	// Request Sense
	printf("test_unit_ready_command:\n");
	memset(sense, 0, sizeof(sense));
	memset(cdb, 0, sizeof(cdb));
	//cdb[0] = 0x03;	// Request Sense
	//cdb[4] = REQUEST_SENSE_LENGTH;

	rc = send_mass_storage_command(handle, endpoint_out, 0, cdb, LIBUSB_ENDPOINT_OUT, 0, &expected_tag);
	if(rc == 0)
	{
		printf("    ready\n");
	}
	else
	{
		printf("    not ready\n");
	}
	if (get_mass_storage_status(handle, endpoint_in, expected_tag) == -2) {
		get_sense(handle, endpoint_in, endpoint_out);
	}

	return rc;
}

static int get_mass_storage_status(libusb_device_handle *handle, uint8_t endpoint, uint32_t expected_tag)
{
	int i, r, size;
	struct command_status_wrapper csw;

	// The device is allowed to STALL this transfer. If it does, you have to
	// clear the stall and try again.
	i = 0;
	do {
		r = libusb_bulk_transfer(handle, endpoint, (unsigned char*)&csw, 13, &size, 1000);
		if (r == LIBUSB_ERROR_PIPE) {
			libusb_clear_halt(handle, endpoint);
		}
		i++;
	} while ((r == LIBUSB_ERROR_PIPE) && (i<RETRY_MAX));
	if (r != LIBUSB_SUCCESS) {
		perr("   get_mass_storage_status: %s\n", libusb_strerror((enum libusb_error)r));
		return -1;
	}
	if (size != 13) {
		perr("   get_mass_storage_status: received %d bytes (expected 13)\n", size);
		return -1;
	}
	if (csw.dCSWTag != expected_tag) {
		perr("   get_mass_storage_status: mismatched tags (expected %08X, received %08X)\n",
			expected_tag, csw.dCSWTag);
		return -1;
	}
	// For this test, we ignore the dCSWSignature check for validity...
	printf("   Mass Storage Status: %02X (%s)\n", csw.bCSWStatus, csw.bCSWStatus?"FAILED":"Success");
	if (csw.dCSWTag != expected_tag)
		return -1;
	if (csw.bCSWStatus) {
		// REQUEST SENSE is appropriate only if bCSWStatus is 1, meaning that the
		// command failed somehow.  Larger values (2 in particular) mean that
		// the command couldn't be understood.
		if (csw.bCSWStatus == 1)
			return -2;	// request Get Sense
		else
			return -1;
	}

	// In theory we also should check dCSWDataResidue.  But lots of devices
	// set it wrongly.
	return 0;
}

static void get_sense(libusb_device_handle *handle, uint8_t endpoint_in, uint8_t endpoint_out)
{
	uint8_t cdb[16];	// SCSI Command Descriptor Block
	uint8_t sense[18];
	uint32_t expected_tag;
	int size;
	int rc;

	// Request Sense
	printf("Request Sense:\n");
	memset(sense, 0, sizeof(sense));
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = 0x03;	// Request Sense
	cdb[4] = REQUEST_SENSE_LENGTH;

	send_mass_storage_command(handle, endpoint_out, 0, cdb, LIBUSB_ENDPOINT_IN, REQUEST_SENSE_LENGTH, &expected_tag);
	rc = libusb_bulk_transfer(handle, endpoint_in, (unsigned char*)&sense, REQUEST_SENSE_LENGTH, &size, 1000);
	if (rc < 0)
	{
		printf("libusb_bulk_transfer failed: %s\n", libusb_error_name(rc));
		return;
	}
	printf("   received %d bytes\n", size);

	if ((sense[0] != 0x70) && (sense[0] != 0x71)) {
		perr("   ERROR No sense data\n");
	} else {
		perr("   ERROR Sense: %02X %02X %02X\n", sense[2]&0x0F, sense[12], sense[13]);
	}
	// Strictly speaking, the get_mass_storage_status() call should come
	// before these perr() lines.  If the status is nonzero then we must
	// assume there's no data in the buffer.  For xusb it doesn't matter.
	get_mass_storage_status(handle, endpoint_in, expected_tag);
}

static int s_init_done = 0;


int libcdrom_init(void)
{
	int ret = 0;

	libusb_device_handle *handle;
	libusb_device *dev;
	uint8_t bus, port_path[8];
	struct libusb_bos_descriptor *bos_desc;
	struct libusb_config_descriptor *conf_desc;
	const struct libusb_endpoint_descriptor *endpoint;
	int i, j, k, r;
	int iface, nb_ifaces, first_iface = -1;
	struct libusb_device_descriptor dev_desc;
	const char* speed_name[5] = { "Unknown", "1.5 Mbit/s (USB LowSpeed)", "12 Mbit/s (USB FullSpeed)",
		"480 Mbit/s (USB HighSpeed)", "5000 Mbit/s (USB SuperSpeed)"};
	char string[128];
	uint8_t string_index[3];	// indexes of the string descriptors
	uint8_t endpoint_in = 0, endpoint_out = 0;	// default IN and OUT endpoints


	uint16_t vid = 0x1bcf; 
	uint16_t pid = 0x0c31;
		
	r = libusb_init(NULL);
	if (r < 0)
		return r;

	libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_INFO);


	printf("Opening device %04X:%04X...\n", vid, pid);
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);

	if (handle == NULL) {
		perr("  Failed.\n");
		return -1;
	}

	dev = libusb_get_device(handle);
	bus = libusb_get_bus_number(dev);

	printf("\nReading device descriptor:\n");
	CALL_CHECK(libusb_get_device_descriptor(dev, &dev_desc));
	printf("            length: %d\n", dev_desc.bLength);
	printf("      device class: %d\n", dev_desc.bDeviceClass);
	printf("               S/N: %d\n", dev_desc.iSerialNumber);
	printf("           VID:PID: %04X:%04X\n", dev_desc.idVendor, dev_desc.idProduct);
	printf("         bcdDevice: %04X\n", dev_desc.bcdDevice);
	printf("   iMan:iProd:iSer: %d:%d:%d\n", dev_desc.iManufacturer, dev_desc.iProduct, dev_desc.iSerialNumber);
	printf("          nb confs: %d\n", dev_desc.bNumConfigurations);
	// Copy the string descriptors for easier parsing
	string_index[0] = dev_desc.iManufacturer;
	string_index[1] = dev_desc.iProduct;
	string_index[2] = dev_desc.iSerialNumber;


	printf("\nReading first configuration descriptor:\n");
	CALL_CHECK(libusb_get_config_descriptor(dev, 0, &conf_desc));
	nb_ifaces = conf_desc->bNumInterfaces;
	printf("             nb interfaces: %d\n", nb_ifaces);
	if (nb_ifaces > 0)
		first_iface = conf_desc->usb_interface[0].altsetting[0].bInterfaceNumber;
	for (i=0; i<nb_ifaces; i++) {
		printf("              interface[%d]: id = %d\n", i,
			conf_desc->usb_interface[i].altsetting[0].bInterfaceNumber);
		for (j=0; j<conf_desc->usb_interface[i].num_altsetting; j++) {
			printf("interface[%d].altsetting[%d]: num endpoints = %d\n",
				i, j, conf_desc->usb_interface[i].altsetting[j].bNumEndpoints);
			printf("   Class.SubClass.Protocol: %02X.%02X.%02X\n",
				conf_desc->usb_interface[i].altsetting[j].bInterfaceClass,
				conf_desc->usb_interface[i].altsetting[j].bInterfaceSubClass,
				conf_desc->usb_interface[i].altsetting[j].bInterfaceProtocol);
			if ( (conf_desc->usb_interface[i].altsetting[j].bInterfaceClass == LIBUSB_CLASS_MASS_STORAGE)
			  && ( (conf_desc->usb_interface[i].altsetting[j].bInterfaceSubClass == 0x01)
			  || (conf_desc->usb_interface[i].altsetting[j].bInterfaceSubClass == 0x06) )
			  && (conf_desc->usb_interface[i].altsetting[j].bInterfaceProtocol == 0x50) ) {
				// Mass storage devices that can use basic SCSI commands
				test_mode = USE_SCSI;
				printf("this is cdrom\n");
			}
			else
			{
				printf("this is not cdrom\n");
			}
			for (k=0; k<conf_desc->usb_interface[i].altsetting[j].bNumEndpoints; k++) {
				endpoint = &conf_desc->usb_interface[i].altsetting[j].endpoint[k];
				printf("       endpoint[%d].address: %02X\n", k, endpoint->bEndpointAddress);
				// Use the first interrupt or bulk IN/OUT endpoints as default for testing
				if ((endpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) & (LIBUSB_TRANSFER_TYPE_BULK | LIBUSB_TRANSFER_TYPE_INTERRUPT)) {
					if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
						if (!endpoint_in)
							endpoint_in = endpoint->bEndpointAddress;
					} else {
						if (!endpoint_out)
							endpoint_out = endpoint->bEndpointAddress;
					}
				}
			}
		}
	}
	libusb_free_config_descriptor(conf_desc);

	if(test_mode == USE_SCSI)
	{
		libusb_set_auto_detach_kernel_driver(handle, 1);
		
		for (iface = 0; iface < nb_ifaces; iface++)
		{
			printf("\nClaiming interface %d...\n", iface);
			r = libusb_claim_interface(handle, iface);
			if (r != LIBUSB_SUCCESS) {
				perr("   Failed.\n");
			}
		}

		printf("\nReading string descriptors:\n");
		for (i=0; i<3; i++) {
			if (string_index[i] == 0) {
				continue;
			}
			if (libusb_get_string_descriptor_ascii(handle, string_index[i], (unsigned char*)string, 128) >= 0) {
				printf("   String (0x%02X): \"%s\"\n", string_index[i], string);
			}
		}
		// Read the OS String Descriptor
		if (libusb_get_string_descriptor_ascii(handle, 0xEE, (unsigned char*)string, 128) >= 0) {
			printf("   String (0x%02X): \"%s\"\n", 0xEE, string);
		}

			cdrom_para.handle = handle;
			cdrom_para.nb_ifaces = nb_ifaces;
			cdrom_para.endpoint_in = endpoint_in;
			cdrom_para.endpoint_out = endpoint_out;

		printf("\n");
		return 0;
	}
	else
	{
		printf("Closing device...\n");
		libusb_close(handle);
		return -1;
	}

}


void libcdrom_uninit(void)
{
	int iface;
	
	for (iface = 0; iface<cdrom_para.nb_ifaces; iface++) {
		printf("Releasing interface %d...\n", iface);
		libusb_release_interface(cdrom_para.handle, iface);
	}

	printf("Closing device...\n");
	libusb_close(cdrom_para.handle);
}

jint  Java_com_example_brooklynxu_myapplication_MainActivity_getCdromTrackCount
  (JNIEnv *env, jobject jobj)
{
	int count = 0;
	int ret;

	if(s_init_done == 0)
	{
		ret = libcdrom_init();
		__android_log_print(ANDROID_LOG_ERROR,"cdrom-jni","libcdrom_init [%d]\n",ret);
		if(ret == 0)
		{
			s_init_done = 1;
			cmd_scsi_read_toc_pma_atip(cdrom_para.handle, cdrom_para.endpoint_in, cdrom_para.endpoint_out, cdrom_para.lun,
				2, &track_para);
		}
		else
		{
			s_init_done = 0;
		}
	}
	else
	{
		if(track_para.track_total == 0)
		{
			cmd_scsi_read_toc_pma_atip(cdrom_para.handle, cdrom_para.endpoint_in, cdrom_para.endpoint_out, cdrom_para.lun,
				2, &track_para);
		}
	}

	count = track_para.track_total;
	__android_log_print(ANDROID_LOG_ERROR,"cdrom-jni","track_total [%d]\n",count);
	return count;
}


static char sVer[20] = {0};
jint  Java_com_example_brooklynxu_myapplication_MainActivity_getVersion
  (JNIEnv *env, jobject jobj)
{
	const struct libusb_version* version;
	unsigned int ver;

	version = libusb_get_version();
	printf("Using libusb                        v%d.%d.%d.%d\n\n", version->major, version->minor, version->micro, version->nano);

	ver = version->micro; 
	//sprintf(sVer,"%d.%d.%d.%d",version->major, version->minor, version->micro, version->nano);
	return ver;
}

extern void set_linux_usb_fd(int fd);
void test_linux_usb_fd(int fd)
{
	char dev_name[64];
	unsigned char desc[4096];
	int desc_length;
	int writeable;


	desc_length = read(fd, desc, sizeof(desc));
	__android_log_print(ANDROID_LOG_ERROR,"cdrom-jni","usb_device_new read returned %d errno %d\n", desc_length, errno);
	if (desc_length < 0)
	{
		__android_log_print(ANDROID_LOG_ERROR,"cdrom-jni","read faild  test_linux_usb_fd\n");
	}
	else
	{
		display_buffer_hex(desc, desc_length);
	}

//	strncpy(dev_name, dev_name, sizeof(device->dev_name) - 1);
//	device->fd = fd;
//	device->desc_length = length;
//	// assume we are writeable, since usb_device_get_fd will only return writeable fds
//	device->writeable = 1;

}
void  Java_com_example_brooklynxu_myapplication_MainActivity_setCdromFd
  (JNIEnv *env, jobject jobj, jint fd)
{
	int fd_dup;
	if(fd > 0)
	{
		fd_dup = dup(fd);
//		ALOGE("fd fd_dup[%d, %d]\n",fd,fd_dup);
//		ALOGE("fd fd_dup[%d, %d]\n",fd,fd_dup);
		__android_log_print(ANDROID_LOG_ERROR,"cdrom-jni","fd fd_dup[%d, %d]\n",fd,fd_dup);
		if (lseek(fd_dup, 0, SEEK_SET) != 0)
		{
//			ALOGE("fd_dup failed !\n");
			__android_log_print(ANDROID_LOG_ERROR,"cdrom-jni","fd_dup failed !\n");
		}
		else
		{
//			ALOGE("fd_dup success !\n");
			__android_log_print(ANDROID_LOG_ERROR,"cdrom-jni","fd_dup success !\n");

			test_linux_usb_fd(fd_dup);
		}
		set_linux_usb_fd(fd_dup);
	//	if(fd > 0)
	//		s_init_done = 1;
	}
}

jint Java_com_example_brooklynxu_myapplication_MainActivity_grabCdromTrackByNumber
  (JNIEnv *env, jobject jobj, jint number)
{
	grab_track_by_number_to_file(number, NULL);

	return 0;
}
