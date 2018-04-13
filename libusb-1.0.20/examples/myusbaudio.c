

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include "libusb.h"
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#include <time.h>
#include <sys/time.h>


#if defined(_WIN32)
#define msleep(msecs) Sleep(msecs)
#else
#include <unistd.h>
#define msleep(msecs) usleep(1000*msecs)
#endif

#if defined(_MSC_VER)
#define snprintf _snprintf
#define putenv _putenv
#endif

#if !defined(bool)
#define bool int
#endif
#if !defined(true)
#define true (1 == 1)
#endif
#if !defined(false)
#define false (!true)
#endif


#define AUTO_DETACH_KERNEL	0

#define XMOS_USB_VID			0x20b1
#define XMOS_USB_PID			0x000a

#define usb_interface interface


//#define USB_AUDIO_CLASS_VERSION		1
#define USB_AUDIO_CLASS_VERSION		2

/*
•  Input Terminal (IT)
•  Output Terminal (OT)
•  Mixer Unit (MU)
•  Selector Unit (SU)
•  Feature Unit (FU)
•  Sampling Rate Converter Unit
•  Effect Unit (EU)
•  Processing Unit (PU)
•  Extension Unit (XU)

•  Clock Source (CS)
•  Clock Selector (CX)
•  Clock Multiplier (CM)
*/

/*	4.1  Audio Channel Cluster Descriptor		3.7.2.3 Audio Channel Cluster Format     */
typedef struct 
{
	uint8_t	bNrChannels;		// Number   Number of logical output channels in the Terminal's output audio channel cluster.
	uint32_t	bmChannelConfig;	// Bitmap   Describes the spatial location of the logical channels.
	uint8_t	iChannelNames;	// Index  Index of a string descriptor, describing the name of the first logical channel.
}Audio_Channel_Cluster_Descriptor;

/*
4.2  Device Descriptor(uac 2.0)			4.1 Device Descriptor(uac 1.0)
uac2.0  ºÍuac1.0Ö÷ÒªÍ¨¹ýbDeviceClass/bDeviceSubClass/bDeviceProtocolÀ´Çø±ð
ÔÙÍ¨¹ýinterfaceÖÐµÄbFunctionClass          1 Audio(Interface Association)»ò
bInterfaceClass         1 Audio(Interface Descriptor)À´
È·¶¨ÊÇuacÉè±¸ÖÐaudioÉè±¸
{
	  bLength;
	  bDescriptorType;
	  bcdUSB;
	  bDeviceClass;		(0xEF)		(0)
	  bDeviceSubClass;	(0x02)		(0)
	  bDeviceProtocol;	(0x01)		(0)
	  bMaxPacketSize0;
	  idVendor;
	  idProduct;
	  bcdDevice;
	  iManufacturer;
	  iProduct;
	  iSerialNumber;
	  bNumConfigurations;
}
	
audio functionality is always considered to reside at the interface level
*/
struct libusb_device_descriptor uac_dev_desc;

/*
4.3  Device_Qualifier Descriptor(uac2.0)
the bDeviceClass, bDeviceSubClass and bDeviceProtocol fields of the Device_Qualifier descriptor must also
USB Device Class Definition for Audio Devices
Release 2.0  May 31, 2006  46
be set to 0xEF, 0x02, and 0x01 respectively
*/
#define LIBUSB_DT_QUALIFIER		0x06
typedef struct 
{
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint16_t		bcdUSB;
	uint8_t		bDeviceClass;
	uint8_t		bDeviceSubClass;
	uint8_t		bDeviceProtocol;
	uint8_t		bMaxPacketSize0;
	uint8_t		bNumConfigurations;
	uint8_t		bReserved;
	
}Device_Qualifier_Descriptor;

/*
4.4  Configuration Descriptor	4.2 Configuration Descriptor(uac1.0)	
*/
struct libusb_config_descriptor *uac_conf_desc = NULL;

/*
4.5  Other_Speed_Configuration Descriptor(uac2.0)
*/
#define LIBUSB_DT_OTHER_SPEED_CONFIGURATION		0x07
typedef struct 
{
	uint8_t		bLength;		// Number Size of descriptor
	uint8_t		bDescriptorType;	// 	Constant Other_speed_Configuration Type
	uint16_t		wTotalLength;		// Total length of data returned
	uint8_t		bNumInterfaces;	// Number Number of interfaces supported by this speed configuration
	uint8_t		bConfigurationValue;	// Number Value to use to select configuration
	uint8_t		iConfiguration;	// Index Index of string descriptor
	uint8_t		bmAttributes;		// Bitmap Same as Configuration descriptor
	uint8_t		bMaxPower;	// mA Same as Configuration descriptor
	
}Other_Speed_Config_Descriptor;

#if 0
enum USB_DescriptorTypes_t
{
    USB_DESCTYPE_DEVICE                 = 0x01, /* Device descriptor */
    USB_DESCTYPE_CONFIGURATION          = 0x02, /* Configuration descriptor */
    USB_DESCTYPE_STRING                 = 0x03, /* String descriptor */
    USB_DESCTYPE_INTERFACE              = 0x04, /* Interface descriptor */
    USB_DESCTYPE_ENDPOINT               = 0x05, /* Endpoint descriptor */
    USB_DESCTYPE_DEVICE_QUALIFIER       = 0x06, /* Device qualifier descriptor */
    USB_DESCTYPE_OTHER_SPEED            = 0x07,
    USB_DESCTYPE_INTERFACE_POWER        = 0x08, /* Interface power descriptor */
    USB_DESCTYPE_OTG                    = 0x09,
    USB_DESCTYPE_DEBUG                  = 0x0A,
    USB_DESCTYPE_INTERFACE_ASSOCIATION  = 0x0B, /* Interface association descriptor */
};
#endif
#define LIBUSB_DT_INTERFACE_ASSOCIATION		0x0B
#define LIBUSB_DT_INT_ASSOCIATION_SIZE			8

/*
4.6  Interface Association Descriptor(uac2.0)
bFunctionClass  = AUDIO (LIBUSB_CLASS_AUDIO)	uac2.0 ¾ÍÊÇÍ¨¹ýÕâ¸öÖµÀ´È·¶¨µÄ
bFunctionSubClass = FUNCTION_SUBCLASS_UNDEFINED
bFunctionProtocol = AF_VERSION_02_00
*/
/* A.1 Audio Function Class Code */
#define AUDIO_FUNCTION                                   AUDIO
/* A.2 Audio Function Subclass Codes */
#define FUNCTION_SUBCLASS_UNDEFINED                      0x00
/* A.3 Audio Function Protocol Codes */
enum USB_Audio_FuncProtocolCodes_t
{
	UAC_FUNC_PROTOCOL_UNDEFINED                          = 0x00,
	UAC_FUNC_PROTOCOL_AF_VERSION_02_00                   = 0x20
};
/* A.4 Audio Interface Class Code */
#define AUDIO                                            0x01

/*A.5  Audio Interface Subclass Codes*/
enum UAC_IntSubclassCodes_t
{
	UAC_INT_SUBCLASS_UNDEFINED			= 0x00,
	UAC_INT_SUBCLASS_AUDIOCONTROL                        = 0x01,
	UAC_INT_SUBCLASS_AUDIOSTREAMING                      = 0x02,
	UAC_INT_SUBCLASS_MIDISTREAMING                       = 0x03
};
/*A.6  Audio Interface Protocol Codes*/
enum UAC_IntProtocolCodes_t
{
    UAC_INT_PROTOCOL_UNDEFINED                           = 0x00,
    UAC_INT_PROTOCOL_IP_VERSION_02_00                    = 0x20
};
#define FUNCTION_PROTOCOL_UNDEFINED		UAC_INT_PROTOCOL_UNDEFINED
#define AF_VERSION_02_00						UAC_INT_PROTOCOL_IP_VERSION_02_00
/*A.7  Audio Function Category Codes*/
enum UAC_AudioFunctionCategory_t
{
    UAC_FUNCTION_SUBCLASS_UNDEFINED                      = 0x00,
    UAC_FUNCTION_DESKTOP_SPEAKER                         = 0x01,
    UAC_FUNCITON_HOME_THEATER                            = 0x02,
    UAC_FUNCTION_MICROPHONE                              = 0x03,
    UAC_FUNCITON_HEADSET                                 = 0x04,
    UAC_FUNCTION_TELEPHONE                               = 0x05,
    UAC_FUNCTION_CONVERTER                               = 0x06,
    UAC_FUNCTION_VOICE_SOUND_RECORDER                    = 0x07,
    UAC_FUNCTION_IO_BOX                                  = 0x08,
    UAC_FUNCTION_MUSICAL_INTRUMENT                       = 0x09,
    UAC_FUNCTION_PRO_AUDIO                               = 0x0A,
    UAC_FUNCTION_AUDIO_VIDEO                             = 0x0B,
    UAC_FUNCTION_CONTROL_PANEL                           = 0x0C,
    UAC_FUNCITON_OTHER                                   = 0xFF
};
/*A.8  Audio Class-Specific Descriptor Types(uac2.0)*/
enum UAC_CSDescriptorTypes_t
{
    UAC_CS_DESCTYPE_UNDEFINED                            = 0x20,
    UAC_CS_DESCTYPE_DEVICE                               = 0x21,
    UAC_CS_DESCTYPE_CONFIGURATION                        = 0x22,
    UAC_CS_DESCTYPE_STRING                               = 0x23,
    UAC_CS_DESCTYPE_INTERFACE                            = 0x24,
    UAC_CS_DESCTYPE_ENDPOINT                             = 0x25,
};
/*A.9  Audio Class-Specific AC Interface Descriptor Subtypes(uac2.0)  ACS:audio class specific AC:audio_control*/
enum UAC_CS_AC_InterfaceDescriptorSubtype_t
{
    UAC_CS_AC_INTERFACE_SUBTYPE_AC_DESCRIPTOR_UNDEFINED  = 0x00,
    UAC_CS_AC_INTERFACE_SUBTYPE_HEADER                   = 0x01,
    UAC_CS_AC_INTERFACE_SUBTYPE_INPUT_TERMINAL           = 0x02,
    UAC_CS_AC_INTERFACE_SUBTYPE_OUTPUT_TERMINAL          = 0x03,
    UAC_CS_AC_INTERFACE_SUBTYPE_MIXER_UNIT               = 0x04,
    UAC_CS_AC_INTERFACE_SUBTYPE_SELECTOR_UNIT            = 0x05,
    UAC_CS_AC_INTERFACE_SUBTYPE_FEATURE_UNIT             = 0x06,
    UAC_CS_AC_INTERFACE_SUBTYPE_EFFECT_UNIT              = 0x07,
    UAC_CS_AC_INTERFACE_SUBTYPE_PROCESSING_UNIT          = 0x08,
    UAC_CS_AC_INTERFACE_SUBTYPE_EXTENSION_UNIT           = 0x09,
    UAC_CS_AC_INTERFACE_SUBTYPE_CLOCK_SOURCE             = 0x0A,
    UAC_CS_AC_INTERFACE_SUBTYPE_CLOCK_SELECTOR           = 0x0B,
    UAC_CS_AC_INTERFACE_SUBSYPE_CLOCK_MULTIPLIER         = 0x0C,
    UAC_CS_AC_INTERFACE_SUBTYPE_SAMPLE_RATE_CONVERTER    = 0x0D
};
/*
A.10 Audio Class-Specific AS Interface Descriptor Subtypes(uac2.0)
AudioStreaming (AS)
*/
enum UAC_CS_AS_InterfaceDescriptorSubtype_t
{
    UAC_CS_AS_INTERFACE_SUBTYPE_UNDEFINED                = 0x00,
    UAC_CS_AS_INTERFACE_SUBTYPE_AS_GENERAL               = 0x01,
    UAC_CS_AS_INTERFACE_SUBTYPE_FORMAT_TYPE              = 0x02,
    UAC_CS_AS_INTERFACE_SUBTYPE_ENCODER                  = 0x03,
    UAC_CS_AS_INTERFACE_SUBTYPE_DECODER                  = 0x04
};

/*
A.11 Effect Unit Effect Types(uac2.0)
*/
#define EUET_EFFECT_UNDEFINED		0x00
#define EUET_PARAM_EQ_SECTION_EFFECT		0x01
#define EUET_REVERBERATION_EFFECT		0x02
#define EUET_MOD_DELAY_EFFECT		0x03
#define EUET_DYN_RANGE_COMP_EFFECT		0x04

/*
A.12 Processing Unit Process Types(uac2.0)
*/
#define PUPT_PROCESS_UNDEFINED		0x00
#define PUPT_UP_DOWNMIX_PROCESS		0x01
#define PUPT_DOLBY_PROLOGIC_PROCESS		0x02
#define PUPT_STEREO_EXTENDER_PROCESS		0x03
/*
A.13 Audio Class-Specific Endpoint Descriptor Subtypes(uac2.0)
*/
enum UAC_CS_EndpointDescriptorSubtype_t
{
    UAC_CS_ENDPOINT_SUBTYPE_UNDEFINED                   = 0x00,
    UAC_CS_ENDPOINT_SUBTYPE_EP_GENERAL                  = 0x01
};

/*
A.14 Audio Class-Specific Request Codes(uac2.0)
*/
enum UAC_ACS_RequestCode_t
{
	UAC_ACS_REQUEST_CODE_UNDEFINED	= 0x00,
	UAC_ACS_REQUEST_CUR	= 0x01,
	UAC_ACS_REQUEST_RANGE	= 0x02,
	UAC_ACS_REQUEST_MEM	= 0x03
};
/*
A.15 Encoder Type Codes(uac2.0)
*/
#define ENC_TYPE_CODE_ENCODER_UNDEFINED		0x00
#define ENC_TYPE_CODE_OTHER_ENCODER		0x01
#define ENC_TYPE_CODE_AC3_ENCODER 		0x03
#define ENC_TYPE_CODE_WMA_ENCODER 		0x04
#define ENC_TYPE_CODE_DTS_ENCODER 		0x05
/*
A.16 Decoder Type Codes(uac2.0)
*/
#define DEC_TYPE_CODE_ENCODER_UNDEFINED		0x00
#define DEC_TYPE_CODE_OTHER_ENCODER		0x01
#define DEC_TYPE_CODE_AC3_ENCODER 		0x03
#define DEC_TYPE_CODE_WMA_ENCODER 		0x04
#define DEC_TYPE_CODE_DTS_ENCODER 		0x05

/*
A.17.1  Clock Source Control Selectors(uac2.0)
*/
#define CS_CONTROL_UNDEFINED_UNDEFINED		0x00
#define CS_SAM_FREQ_CONTROL		0x01
#define CS_CLOCK_VALID_CONTROL		0x02
/*
A.17.2  Clock Selector Control Selectors(uac2.0)
*/
#define CX_CONTROL_UNDEFINED		0x00
#define CX_CLOCK_SELECTOR_CONTROL		0x01
/*
A.17.3  Clock Multiplier Control Selectors(uac2.0)
*/
#define CM_CONTROL_UNDEFINED		0x00
#define CM_NUMERATOR_CONTROL		0x01
#define CM_DENOMINATOR_CONTROL		0x02
/*
A.17.4  Terminal Control Selectors(uac2.0)
*/
#define TE_CONTROL_UNDEFINED		0x00
#define TE_COPY_PROTECT_CONTROL		0x01
#define TE_CONNECTOR_CONTROL		0x02
#define TE_OVERLOAD_CONTROL		0x03
#define TE_CLUSTER_CONTROL		0x04
#define TE_UNDERFLOW_CONTROL		0x05
#define TE_OVERFLOW_CONTROL		0x06
#define TE_LATENCY_CONTROL		0x07
/*
A.17.5  Mixer Control Selectors(uac2.0)
*/
#define MU_CONTROL_UNDEFINED		0x00
#define MU_MIXER_CONTROL		0x01
#define MU_CLUSTER_CONTROL		0x02
#define MU_UNDERFLOW_CONTROL		0x03
#define MU_OVERFLOW_CONTROL		0x04
#define MU_LATENCY_CONTROL		0x05
/*
A.17.6  Selector Control Selectors(uac2.0)
*/
#define SU_CONTROL_UNDEFINED		0x00
#define SU_SELECTOR_CONTROL		0x01
#define SU_LATENCY_CONTROL		0x02
/*
A.17.7  Feature Unit Control Selectors(uac2.0)
*/
#define FU_CONTROL_UNDEFINED		0x00
#define FU_MUTE_CONTROL		0x01
#define FU_VOLUME_CONTROL		0x02
#define FU_BASS_CONTROL		0x03
#define FU_MID_CONTROL		0x04
#define FU_TREBLE_CONTROL		0x05
#define FU_GRAPHIC_EQUALIZER_CONTROL		0x06
#define FU_AUTOMATIC_GAIN_CONTROL		0x07
#define FU_DELAY_CONTROL		0x08
#define FU_BASS_BOOST_CONTROL		0x09
#define FU_LOUDNESS_CONTROL		0x0A
#define FU_INPUT_GAIN_CONTROL		0x0B
#define FU_INPUT_GAIN_PAD_CONTROL		0x0C
#define FU_PHASE_INVERTER_CONTROL		0x0D
#define FU_UNDERFLOW_CONTROL		0x0E
#define FU_OVERFLOW_CONTROL		0x0F
#define FU_LATENCY_CONTROL		0x10

/*
A.17.8  Effect Unit Control Selectors
A.17.8.1  Parametric Equalizer Section Effect Unit Control Selectors(uac2.0)
*/
#define PE_CONTROL_UNDEFINED		0x00
#define PE_ENABLE_CONTROL		0x01
#define PE_CENTERFREQ_CONTROL		0x02
#define PE_QFACTOR_CONTROL		0x03
#define PE_GAIN_CONTROL		0x04
#define PE_UNDERFLOW_CONTROL		0x05
#define PE_OVERFLOW_CONTROL		0x06
#define PE_LATENCY_CONTROL		0x07
/*
A.17.8.2  Reverberation Effect Unit Control Selectors(uac2.0)
*/
#define RV_CONTROL_UNDEFINED		0x00
#define RV_ENABLE_CONTROL		0x01
#define RV_TYPE_CONTROL		0x02
#define RV_LEVEL_CONTROL		0x03
#define RV_TIME_CONTROL		0x04
#define RV_FEEDBACK_CONTROL		0x05
#define RV_PREDELAY_CONTROL		0x06
#define RV_DENSITY_CONTROL		0x07
#define RV_HIFREQ_ROLLOFF_CONTROL		0x08
#define RV_UNDERFLOW_CONTROL		0x09
#define RV_OVERFLOW_CONTROL		0x0A
#define RV_LATENCY_CONTROL		0x0B
/*
A.17.8.3  Modulation Delay Effect Unit Control Selectors(uac2.0)
*/
#define MD_CONTROL_UNDEFINED		0x00
#define MD_ENABLE_CONTROL		0x01
#define MD_BALANCE_CONTROL		0x02
#define MD_RATE_CONTROL		0x03
#define MD_DEPTH_CONTROL		0x04
#define MD_TIME_CONTROL		0x05
#define MD_FEEDBACK_CONTROL		0x06
#define MD_UNDERFLOW_CONTROL		0x07
#define MD_OVERFLOW_CONTROL		0x08
#define MD_LATENCY_CONTROL		0x09
/*
A.17.8.4  Dynamic Range Compressor Effect Unit Control Selectors
*/
/*
A.17.9  Processing Unit Control Selectors
A.17.9.1  Up/Down-mix Processing Unit Control Selectors
*/
/*
A.17.9.2  Dolby Prologic ™ Processing Unit Control Selectors
*/
/*
A.17.9.3  Stereo Extender Processing Unit Control Selectors
*/
/*
A.17.10 Extension Unit Control Selectors
*/
/*
A.17.11 AudioStreaming Interface Control Selectors
*/
#define AS_CONTROL_UNDEFINED		0x00
#define AS_ACT_ALT_SETTING_CONTROL		0x01
#define AS_VAL_ALT_SETTINGS_CONTROL		0x02
#define AS_AUDIO_DATA_FORMAT_CONTROL		0x03
/*
A.17.12 Encoder Control Selectors
*/
/*
A.17.13 Decoder Control Selectors
A.17.13.1  MPEG Decoder Control Selectors
*/
/*
A.17.13.2  AC-3 Decoder Control Selectors
*/
/*
A.17.13.3  WMA Decoder Control Selectors
*/
/*
A.17.13.4  DTS Decoder Control Selectors
*/
/*
A.17.14 Endpoint Control Selectors
*/

typedef struct 
{
	uint8_t	bLength;	// Number  Size of this descriptor in bytes: 8
	uint8_t	bDescriptorType;	// Constant  INTERFACE ASSOCIATION Descriptor.	
	uint8_t	bFirstInterface;	// Number  Interface number of the first interface that is associated with this function.
	uint8_t	bInterfaceCount;	// Number  Number of contiguous interfaces that are associated with this function.
	uint8_t	bFunctionClass;	// Class  AUDIO_FUNCTION Function Class code(assigned by this specification). SeeAppendix A.1, audio Function Class Code”.
	uint8_t	bFunctionSubClass; // SubClass  FUNCTION_SUBCLASS_UNDEFINED Function Subclass code. Currently not used. See Appendix A.2, audio Function Subclass Codes”.
	uint8_t	bFunctionProtocol;	 // Protocol  AF_VERSION_02_00 Function Protocol code. Indicates the current version of the specification. See Appendix A.3, audio Function Protocol Codes”
	uint8_t	iFunction; // Index  Index of a string descriptor that describes this interface.
}Interface_Association_Descriptor;

/*
4.7  AudioControl Interface Descriptors(uac2.0)
*/
/*
4.7  AudioControl Interface Descriptors
4.7.1  Standard AC Interface Descriptor(uac2.0) 
	standard ac interface is standart usb interface
*/
typedef struct
{
	uint8_t 	bLength;		// Number  Size of this descriptor, in bytes: 9
	uint8_t 	bDescriptorType;	// Constant  INTERFACE descriptor type	4
	uint8_t 	bInterfaceNumber;	// Number  Number of interface. A zero-based value identifying the index in the array of concurrent interfaces supported by this configuration.
	uint8_t 	bAlternateSetting;	// Number  Value used to select an Alternate Setting for the interface identified in the prior field. Must be set to 0.
	uint8_t 	bNumEndpoints;	// Number  Number of endpoints used by this interface (excluding endpoint 0). This number is either 0 or 1 if the optional interrupt endpoint is present.
	uint8_t 	bInterfaceClass;	// Class  AUDIO. Audio Interface Class code uac2.0 ¾ÍÊÇÍ¨¹ýÕâ¸öÖµÀ´È·¶¨µÄ
	uint8_t 	bInterfaceSubClass;	// Subclass  AUDIOCONTROL. Audio Interface Subclass code. Assigned by this specification. See Appendix A.5,audio Interface Subclass Codes.”
	uint8_t 	bInterfaceProtocol;
	uint8_t	iInterface;
}Standard_AC_Interface_Descriptor;

/*4.7.2  Class-Specific AC Interface Descriptor(uac2.0)*/
typedef struct
{
	uint8_t 	bLength;	//9	// Number  Size of this descriptor, in bytes: 9
	uint8_t 	bDescriptorType;	// Constant  CS_INTERFACE descriptor type.	0x24 36
	uint8_t 	bDescriptorSubtype;	// 1 // Constant  HEADER descriptor subtype.   1 ACS_AC_DT_HEADER
	uint16_t 	bcdADC;	//  BCD  Audio Device Class Specification Release Number in Binary-Coded Decimal.
	uint8_t 	bCategory;	// Constant  Constant, indicating the primary use of this audio function, as intended by the manufacturer. See Appendix A.7, aAudio Function Category Codes.”
	uint16_t 	wTotalLength;	// Number  Total number of bytes returned for the class-specific AudioControl interface descriptor. Includes the combined length of this descriptor header and all Clock Source, Unit and Terminal descriptors.
	uint8_t 	bmControls;	//  Bitmap  D1..0:  Latency Control D7..2:  Reserved. Must be set to 0.
}CS_AC_Interface_Header_Descriptor;
/*4.7.2.1  Clock Source Descriptor(uac2.0)*/
/*
	uint8_t 	bLength;	//8	// Number  Size of this descriptor, in bytes: 8
	uint8_t 	bDescriptorType;	// Constant  CS_INTERFACE descriptor type.	0x24 36
	uint8_t 	bDescriptorSubtype; // 10	// Constant  CLOCK_SOURCE descriptor subtype.   1 ACS_AC_DT_HEADER
	uint8_t 	bClockID;	Constant uniquely identifying the Clock
						Source Entity within the audio function.
						This value is used in all requests to
						address this Entity.
	uint8_t 	bmAttributes;	Bitmap  D1..0:  Clock Type:
									00: External Clock
									01: Internal fixed Clock
									10: Internal variable Clock
									11: Internal programmable Clock
									D2:  Clock synchronized to SOF
									D7..3:  Reserved. Must be set to 0.
	uint8_t	bmControls;	Bitmap  D1..0:  Clock Frequency Control
								D3..2:  Clock Validity Control
								D7..4:  Reserved. Must be set to 0.
	uint8_t 	bAssocTerminal;	Constant  Terminal ID of the Terminal that is
								associated with this Clock Source.
	uint8_t 	iClockSource;	  Number  Index of a string descriptor, describing the Clock Source Entity.

*/
typedef struct
{
	uint8_t 	bLength;	//8	// Number  Size of this descriptor, in bytes: 8
	uint8_t 	bDescriptorType;	// Constant  CS_INTERFACE descriptor type.	0x24 36
	uint8_t 	bDescriptorSubtype; // 10	// Constant  CLOCK_SOURCE descriptor subtype.   1 ACS_AC_DT_HEADER
	uint8_t 	bClockID;	
	uint8_t 	bmAttributes;
	uint8_t	bmControls;	
	uint8_t 	bAssocTerminal;	
	uint8_t 	iClockSource;	//  Number  Index of a string descriptor, describing the Clock Source Entity.
}CS_AC_Interface_Clock_Source_Descriptor;
/*4.7.2.2  Clock Selector Descriptor(uac2.0)*/
/*
	uint8_t 	bmAttributes;	Bitmap  D1..0:  Clock Type:
									00: External Clock
									01: Internal fixed Clock
									10: Internal variable Clock
									11: Internal programmable Clock
									D2:  Clock synchronized to SOF
									D7..3:  Reserved. Must be set to 0.

*/
#define NUMBER_INPUT_CLOCK	0x01	// 0x02 //0x03
typedef struct
{
	uint8_t 	bLength;	//7+p	// Number  Size of this descriptor, in bytes: 8
	uint8_t 	bDescriptorType;	// Constant  CS_INTERFACE descriptor type.	0x24 36
	uint8_t 	bDescriptorSubtype; // 11	// Constant  CLOCK_SELECTOR descriptor subtype.   1 ACS_AC_DT_HEADER
	uint8_t 	bClockID;	/*Constant uniquely identifying the Clock
						Source Entity within the audio function.
						This value is used in all requests to
						address this Entity.*/
	uint8_t 	bNrInPins;	// Number  Number of Input Pins of this Unit: p
	uint8_t	baCSourceID[1];	/*ID of the Clock Entity to which the first
							Clock Input Pin of this Clock Selector
							Entity is connected.*/
	uint8_t 	bmControls;	/*BitmapD1..0:  Clock Selector Control
								D7..2:  Reserved. Must be set to 0.*/
	uint8_t 	iClockSelector;	//  Index  Index of a string descriptor, describing the Clock Selector Entity..
}CS_AC_Interface_Clock_Selector_1_Descriptor;
typedef struct
{
	uint8_t 	bLength;	//7+p	// Number  Size of this descriptor, in bytes: 8
	uint8_t 	bDescriptorType;	// Constant  CS_INTERFACE descriptor type.	0x24 36
	uint8_t 	bDescriptorSubtype; // 11	// Constant  CLOCK_SELECTOR descriptor subtype.   1 ACS_AC_DT_HEADER
	uint8_t 	bClockID;	/*Constant uniquely identifying the Clock
						Source Entity within the audio function.
						This value is used in all requests to
						address this Entity.*/
	uint8_t 	bNrInPins;	// Number  Number of Input Pins of this Unit: p
	uint8_t	baCSourceID[2];	/*ID of the Clock Entity to which the first
							Clock Input Pin of this Clock Selector
							Entity is connected.*/
	uint8_t 	bmControls;	/*BitmapD1..0:  Clock Selector Control
								D7..2:  Reserved. Must be set to 0.*/
	uint8_t 	iClockSelector;	//  Index  Index of a string descriptor, describing the Clock Selector Entity..
}CS_AC_Interface_Clock_Selector_2_Descriptor;
typedef struct
{
	uint8_t 	bLength;	//7+p	// Number  Size of this descriptor, in bytes: 8
	uint8_t 	bDescriptorType;	// Constant  CS_INTERFACE descriptor type.	0x24 36
	uint8_t 	bDescriptorSubtype; // 11	// Constant  CLOCK_SELECTOR descriptor subtype.   1 ACS_AC_DT_HEADER
	uint8_t 	bClockID;	/*Constant uniquely identifying the Clock
						Source Entity within the audio function.
						This value is used in all requests to
						address this Entity.*/
	uint8_t 	bNrInPins;	// Number  Number of Input Pins of this Unit: p
	uint8_t	baCSourceID[3];	/*ID of the Clock Entity to which the first
							Clock Input Pin of this Clock Selector
							Entity is connected.*/
	uint8_t 	bmControls;	/*BitmapD1..0:  Clock Selector Control
								D7..2:  Reserved. Must be set to 0.*/
	uint8_t 	iClockSelector;	//  Index  Index of a string descriptor, describing the Clock Selector Entity..
}CS_AC_Interface_Clock_Selector_3_Descriptor;
/*
4.7.2.4  Input Terminal Descriptor(uac2.0)
0  bLength  			1  Number  Size of this descriptor, in bytes: 17
1  bDescriptorType  		1  Constant  CS_INTERFACE descriptor type.
2  bDescriptorSubtype  	1  Constant  INPUT_TERMINAL descriptor subtype.
3  bTerminalID  			1  Constant  Constant uniquely identifying the Terminal
								within the audio function. This value is
								used in all requests to address this
								Terminal.
4  wTerminalType  		2  Constant  Constant characterizing the type of Terminal. See USB Audio Terminal
								Types.
6  bAssocTerminal  		1  Constant  ID of the Output Terminal to which this Input Terminal is associated.
7  bCSourceID  			1  Constant  ID of the Clock Entity to which this Input Terminal is connected.
8  bNrChannels  		1  Number  Number of logical output channels in the Terminals output audio channel cluster.
9  bmChannelConfig  	4  Bitmap  Describes the spatial location of the logical channels.
13  iChannelNames  		1  Index  Index of a string descriptor, describing the
							name of the first logical channel.
14  bmControls  		2  Bitmap  D1..0:  Copy Protect Control
							D3..2:  Connector Control
							D5..4:  Overload Control
							D7..6:  Cluster Control
							D9..8:  Underflow Control
							D11..10: Overflow Control
							D15..12: Reserved. Must be set to 0.
16  iTerminal  			1  Index  Index of a string descriptor, describing the Input Terminal.
*/
#define USB_TERMINAL_TYPES_USB_UNDEFINED	0X100
#define USB_TERMINAL_TYPES_USB_STREAMING	0X101
#define USB_TERMINAL_TYPES_USB_VENDOR_SPECIFIC	0X1FF

typedef struct
{
	uint8_t 	bLength;	// 17
	uint8_t 	bDescriptorType;	// CS_INTERFACE 36
	uint8_t 	bDescriptorSubtype;	// INPUT_TERMINAL 2
	uint8_t 	bTerminalID;	// Constant
	uint16_t 	wTerminalType;	// Constant
	uint8_t 	bAssocTerminal;	// Constant
	uint8_t 	bCSourceID;	// Constant
	uint8_t 	bNrChannels;	// 
	uint32_t 	bmChannelConfig;	// 
	uint8_t 	iChannelNames;	// 
	uint16_t 	bmControls;	// 
	uint8_t 	iTerminal;	// 
	
}CS_AC_Interface_Input_Terminal_Descriptor;
/*
4.7.2.5  Output Terminal Descriptor(uac2.0)
*/
typedef struct
{
	uint8_t 	bLength;	// 12
	uint8_t 	bDescriptorType;	// CS_INTERFACE 36
	uint8_t 	bDescriptorSubtype;	// OUTPUT_TERMINAL 3
	uint8_t 	bTerminalID;	// Constant
	uint16_t 	wTerminalType;	// Constant
	uint8_t 	bAssocTerminal;	// Constant
	uint8_t 	bSourceID;	// Constant
	uint8_t 	bCSourceID;	// Constant
	uint16_t 	bmControls;	// 
	uint8_t 	iTerminal;	// 
	
}CS_AC_Interface_Output_Terminal_Descriptor;
/*
4.7.2.8  Feature Unit Descriptor(uac2.0)
The Feature Unit is uniquely identified by the value in the bUnitID field of the Feature Unit descriptor
(FUD). No other Unit or Terminal within the AudioControl interface may have the same ID. This value
must be passed in the Entity ID field (part of the wIndex field) of each request that is directed to the
Feature Unit.
*/
#define NUM_USB_CHAN_OUT 0x02 //0x01 // 0x03 .........
typedef struct
{
	uint8_t 	bLength;	// 6+(ch+1)*4
	uint8_t 	bDescriptorType;	// CS_INTERFACE 36
	uint8_t 	bDescriptorSubtype;	// OUTPUT_TERMINAL 3
	uint8_t 	bUnitID;	// Constant
	uint8_t 	bSourceID;	// Constant
	uint16_t 	bmaControls[NUM_USB_CHAN_OUT+1];	// bmaControls[0]:master  [1]:ch 1.................
	uint8_t 	iFeature;	// 
}CS_AC_Interface_Feature_Unit_Descriptor;

/*
4.8.1  AC Control Endpoint Descriptors
4.8.1.1  Standard AC Control Endpoint Descriptor
Because endpoint 0 is used as the AudioControl control endpoint, there is no dedicated standard control
endpoint descriptor.
4.8.1.2  Class-Specific AC Control Endpoint Descriptor
There is no dedicated class-specific control endpoint descriptor.
4.8.2  AC Interrupt Endpoint Descriptors
4.8.2.1  Standard AC Interrupt Endpoint Descriptor(uac2.0)
*/
/*
4.9  AudioStreaming Interface Descriptors
The AudioStreaming (AS) interface descriptors contain all relevant information to characterize the
AudioStreaming interface in full.
4.9.1  Standard AS Interface Descriptor(uac2.0)
*/
typedef struct
{
	uint8_t 	bLength;	// 9
	uint8_t 	bDescriptorType;	// INTERFACE 4
	uint8_t 	bInterfaceNumber;	// Number of interface. A zero-based value identifying the index in the array of concurrent interfaces supported by this configuration.
	uint8_t 	bAlternateSetting;	// Number  Value used to select an Alternate Setting for the interface identified in the prior field.
	uint8_t 	bNumEndpoints;	// Number  Number of endpoints used by this interface (excluding endpoint 0). Must be either 0 (no data endpoint), 1 (data endpoint) or 2 (data and explicit feedback endpoint).
	uint8_t 	bInterfaceClass;	// Class  AUDIO Audio Interface Class code (assigned by the USB). See Appendix A.4, Audio Interface Class Code”
	uint8_t 	bInterfaceSubClass;	// Subclass  AUDIO_STREAMING Audio Interface Subclass code. Assigned by this specification. See Appendix A.5, Audio Interface Subclass Codes.”
	uint8_t	bInterfaceProtocol;	// Protocol  IP_VERSION_02_00 Interface Protocol  code. Indicates the current version of the specification. See Appendix A.6, Audio Interface Protocol Codes”
	uint8_t	iInterface;	// Index  Index of a string descriptor that describes this interface.
}standard_AS_Interface_Descriptor;
/*
4.9.2  Class-Specific AS Interface Descriptor(uac2.0)
*/
typedef struct
{
	uint8_t 	bLength;	// 16
	uint8_t 	bDescriptorType;	// CS_INTERFACE 36
	uint8_t 	bDescriptorSubtype;	// AS_GENERAL 0x01
	uint8_t 	bTerminalLink;	// Constant  The Terminal ID of the Terminal to which this interface is connected.
	uint8_t 	bmControls;	// Bitmap  D1..0:  Active Alternate Setting Control
							//     D3..2:  Valid Alternate Settings Control
							//     D7..4:  Reserved. Must be set to 0.
	uint8_t 	bFormatType;	//  Constant  Constant identifying the Format Type the AudioStreaming interface is using.
	uint32_t 	bmFormats;	// Bitmap  The Audio Data Format(s) that can be
						//used to communicate with this interface.
						//See the USB Audio Data Formats
						//document for further details.
	uint8_t	bNrChannels;	// Number  Number of physical channels in the AS Interface audio channel cluster.
	uint32_t	bmChannelConfig;	// Bitmap  Describes the spatial location of the physical channels.
	uint8_t	iChannelNames;	// Index  Index of a string descriptor, describing the name of the first physical channel.
}CS_AS_Interface_Descriptor;
/*
4.9.3  Class-Specific AS Format Type Descriptor(uac2.0)
2.3.1.6  Type I Format Type Descriptor
*/
typedef struct
{
	uint8_t 	bLength;	// 6
	uint8_t 	bDescriptorType;	// CS_INTERFACE 36
	uint8_t 	bDescriptorSubtype;	// FORMAT_TYPE 0x02
	uint8_t 	bFormatType;	// Constant  FORMAT_TYPE_I 0x01
	uint8_t 	bSubslotSize;	// Number  The number of bytes occupied by one audio subslot. Can be 1, 2, 3 or 4.
	uint8_t 	bBitResolution;	//  umber  The number of effectively used bits from the available bits in an audio subslot.
}Type_I_Format_Type_Interface_Descriptor;

/*
4.10 AudioStreaming Endpoint Descriptors
The following sections describe all possible endpoint-related descriptors for the AudioStreaming interface.
4.10.1 AS Isochronous Audio Data Endpoint Descriptors
The standard and class-specific audio data endpoint descriptors provide pertinent information on how
audio data streams are communicated to the audio function. In addition, specific endpoint capabilities and
properties are reported.
4.10.1.1 Standard AS Isochronous Audio Data Endpoint Descriptor(uac2.0)
*/
typedef struct
{
	uint8_t 	bLength;	// 7
	uint8_t 	bDescriptorType;	// ENDPOINT 5
	uint8_t 	bEndpointAddress;	// Endpoint  The address of the endpoint on the USB device described by this descriptor. The
							//address is encoded as follows:
							//D3..0:  The endpoint number,
							//determined by the designer.
							//D6..4:  Reserved, reset to zero
							//D7:  Direction.
							//0 = OUT endpoint
							// 1 = IN endpoint
	uint8_t 	bmAttributes;	// Bit Map  D1..0:  Transfer type		01 = Isochronous
							// 	 D3..2:  Synchronization Type	01 = Asynchronous	10 = Adaptive	11 = Synchronous
							//	 D5..4:  Usage Type	00 = Data endpoint	or	10 = Implicit feedback Data	endpoint
							//All other bits are reserved.
	uint16_t 	wMaxPacketSize;	// Number  Maximum packet size this endpoint is capable of sending or receiving when this configuration is selected. This is determined by the audio bandwidth constraints of the endpoint.
	uint8_t 	bInterval;	// Number  Interval for polling endpoint for data transfers.
}Standard_AS_Iso_Audio_Data_Endpoint_Descriptor;

/*4.10.1.2 Class-Specific AS Isochronous Audio Data Endpoint Descriptor(uac2.0)*/
typedef struct
{
	uint8_t 	bLength;	// 8
	uint8_t 	bDescriptorType;	// CS_ENDPOINT 5
	uint8_t 	bDescriptorSubtype;	// Constant  EP_GENERAL descriptor subtype. 0x01
	uint8_t 	bmAttributes;	// Bit D7 indicates a requirement for
						//wMaxPacketSize packets.
						//D7:  MaxPacketsOnly
	uint8_t 	bmControls;	// Bitmap  D1..0:  Pitch Control
								//D3..2:  Data Overrun Control
								//D5..4:  Data Underrun Control
								//D7..6:  Reserved. Must be set to 0.
	uint8_t 	bLockDelayUnits;	// Number  Indicates the units used for the wLockDelay field:
							// 0:  Undefined
							// 1:  Milliseconds
							// 2:  Decoded PCM samples
							// 3..255: Reserved
	uint8_t 	wLockDelay;	// Number  Indicates the time it takes this endpoint to reliably lock its internal clock recovery circuitry. Units used depend on the value of the bLockDelayUnits field.
}CS_AS_Iso_Audio_Data_Endpoint_Descriptor;
/*
4.10.2 AS Isochronous Feedback Endpoint Descriptor
This descriptor is present only when one or more isochronous audio data endpoints of the adaptive source
type or the asynchronous sink type are implemented.
4.10.2.1 Standard AS Isochronous Feedback Endpoint Descriptor(uac2.0)
*/
typedef struct
{
	uint8_t 	bLength;	// 7
	uint8_t 	bDescriptorType;	// ENDPOINT 5
	uint8_t 	bEndpointAddress;	// Endpoint  The address of the endpoint on the USB device described by this descriptor. The
							//address is encoded as follows:
							//D3..0:  The endpoint number,
							//determined by the designer.
							//D6..4:  Reserved, reset to zero
							//D7:  Direction.
							//0 = OUT endpoint
							// 1 = IN endpoint
	uint8_t 	bmAttributes;	// Bit Map  D1..0:  Transfer type		01 = Isochronous
							// 	 D3..2:  Synchronization Type	00 = No Synchronization
							//	 D5..4:  01 = Feedback endpoint
							//All other bits are reserved.
	uint16_t 	wMaxPacketSize;	// Number  Maximum packet size this endpoint is capable of sending or receiving when this configuration is selected. This is determined by the audio bandwidth constraints of the endpoint.
	uint8_t 	bInterval;	// Number  Interval for polling endpoint for data transfers.
}Standard_AS_Iso_Audio_Feedback_Endpoint_Descriptor;

/***********************************************************************/
/** USB Device Class Definition for Audio Data Formats **/

/* A.1 Format Type Codes */
enum USB_audio_Fmt_FormatType_t
{
    UAC_FORMAT_TYPE_UNDEFINED       = 0x00,
    UAC_FORMAT_TYPE_I               = 0x01,
    UAC_FORMAT_TYPE_II              = 0x02,
    UAC_FORMAT_TYPE_III             = 0x03,
    UAC_FORMAT_TYPE_IV              = 0x04,
    UAC_EXT_FORMAT_TYPE_I           = 0x81,
    UAC_EXT_FORMAT_TYPE_II          = 0x82,
    UAC_EXT_FORMAT_TYPE_III         = 0x83
};

/* A.2 AudioData Format Bit Allocation in the bmFormats field */
/* A.2.1 Audio Data Format Type I Bit Allocations */
enum USB_Audio_Fmt_DataFormat_TypeI_t
{
	UAC_FORMAT_TYPEI_PCM               = 0x00000001,
	UAC_FORMAT_TYPEI_PCM8              = 0x00000002,
	UAC_FORMAT_TYPEI_IEEE_FLOAT        = 0x00000004,
	UAC_FORMAT_TYPEI_ALAW        = 0x00000008,
	UAC_FORMAT_TYPEI_MULAW        = 0x00000010,
	UAC_FORMAT_TYPEI_RAW_DATA          = 0x80000000,
};

/* A.2.2 Audio Data Format Type II Bit Allocations */
enum USB_Audio_Fmt_DataFormat_TypeII_t
{
    UAC_FORMAT_TYPEII_MPEG             = 0x00000001,
    UAC_FORMAT_TYPEII_AC3              = 0x00000002,
    UAC_FORMAT_TYPEII_WMA              = 0x00000004,
    UAC_FORMAT_TYPEII_DTS              = 0x00000008,
    UAC_FORMAT_TYPEII_RAW_DATA         = 0x80000000
};

/* A.3 Side Band Protocol Codes */
#define PROTOCOL_UNDEFINED             0x00
#define PRESS_TIMESTAMP_PROTOCOL       0x01

/***********************************************************************/
/* Univeral Serial Bus  Device Class Definition for Terminal Types */

/* 2.1 USB Terminal Types */
/* Terminal Types that describe Terminals that handle signals carried over USB */
#define USB_TERMTYPE_UNDEFINED         0x0100
#define USB_TERMTYPE_USB_STREAMING     0x0101
#define USB_TERMTYPE_VENDOR_SPECIFIC   0x01FF

/* 2.2 Input Terminal Types */
/* Terminal Types that describe Terminals that are designed to record sounds */
enum USB_Audio_TT_InputTermType_t
{
    UAC_TT_INPUT_TERMTYPE_INPUT_UNDEFINED               = 0x0200,
    UAC_TT_INPUT_TERMTYPE_MICROPHONE                    = 0x0201,
    UAC_TT_INPUT_TERMTYPE_DESKTOP_MICROPHONE            = 0x0202,
    UAC_TT_INPUT_TERMTYPE_PERSONAL_MICROPHONE           = 0x0203,
    UAC_TT_INPUT_TERMTYPE_OMNIDIRECTIONAL_MICROPHONE    = 0x0204,
    UAC_TT_INPUT_TERMTYPE_MICROPHONE_ARRAY              = 0x0205,
    UAC_TT_INPUT_TERMTYPE_PROCESSING_MICROPHONE_ARRAY   = 0x0206
};

/* 2.3 Output Terminal Types */
/* These Terminal Types describe Terminals that produce audible signals that are intended to
 * be heard by the user of the audio function */
enum USB_Audio_TT_OutputTermType_t
{
    UAC_TT_OUTPUT_TERMTYPE_SPEAKER                       = 0x0301,
    UAC_TT_OUTPUT_TERMTYPE_HEADPHONES                    = 0x0302,
    UAC_TT_OUTPUT_TERMTYPE_HEAD_MOUNTED_DISPLAY          = 0x0303,
    UAC_TT_OUTPUT_TERMTYPE_DESKTOP_SPEAKER               = 0x0304,
    UAC_TT_OUTPUT_TERMTYPE_ROOM_SPEAKER                  = 0x0305,
    UAC_TT_OUTPUT_TERMTYPE_COMMUNICATION_SPEAKER         = 0x0306,
    UAC_TT_OUTPUT_TERMTYPE_LOW_FREQ_EFFECTS_SPEAKER      = 0x0307
};


enum UAC_Set_Device_Type_t
{
    UAC_SET_DEVICE_VOLUME                       = 0x00,
    UAC_SET_DEVICE_CHANNEL                    = 0x01,
    UAC_SET_DEVICE_SAMPLERATE          = 0x02,
    UAC_SET_DEVICE_RESOLUTION               = 0x03,
    UAC_SET_DEVICE_MUTE               = 0x04
};

#define	INT_MAX		0x7fffffff	/* max value for an int */
#define	INT_MIN		(-0x7fffffff-1)	/* min value for an int */

#define MAX_NR_RATES	1024
#define MAX_PACKS	6		/* per URB */
#define MAX_PACKS_HS	(MAX_PACKS * 8)	/* in high speed mode */
#define MAX_URBS	12
#define SYNC_URBS	4	/* always four urbs for sync */
#define MAX_QUEUE	18	/* try not to exceed this queue length, in ms */

#define DIV_ROUND_UP(x,y) (((x) + ((y) - 1)) / (y))
#ifndef MIN
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

/*
define some string
*/
/*class code string*/
char *string_class_code[]=
{
	" ",
	"AUDIO",		// 1
	"Communications",
	"HID",
	" ",
	"Physical",		// 5
	" ",
	" ",
	" ",
	" ",
	" ",		// 0x0A
	" ",		// 0x0B
	""
};
/*A.5  Audio Interface Subclass Codes
Table A-5: Audio Interface Subclass Codes*/
char *string_Audio_Interface_Subclass_Codes[]=
{
	"undefined",
	"AUDIO CONTROL",		// 1
	"AUDIO STREAMING",
	"MIDISTREAMING",
	""
};

char *string_uac_type[]=
{
	" ",
	"uac 1.0",
	"uac 2.0",
	""
};
/*A.9  Audio Class-Specific AC Interface Descriptor Subtypes
Table A-9: Audio Class-Specific AC Interface Descriptor Subtypes*/
char *string_uac_AC_Int_Desc_Subtypes[]=
{
	"undefined",
	"HEADER",		// 0x01
	"INPUT_TERMINAL",
	"OUTPUT_TERMINAL",
	"MIXER_UNIT",
	"SELECTOR_UNIT",	//0x05
	"FEATURE_UNIT",	//0x06
	"EFFECT_UNIT",	//0x07
	"PROCESSING_UNIT",	//0x08
	"EXTENSION_UNIT",	//0x09
	"CLOCK_SOURCE",	//0x0A
	"CLOCK_SELECTOR",	//0x0B
	"CLOCK_MULTIPLIER",	//0x0C
	"SAMPLE_RATE_CONVERTER",	//0x0D
	""
};
/*
A.10 Audio Class-Specific AS Interface Descriptor Subtypes
Table A-10: Audio Class-Specific AS Interface Descriptor Subtypes
*/
char *string_uac_AS_Int_Desc_Subtypes[]=
{
	"undefined",
	"AS_GENERAL",		// 0x01
	"FORMAT_TYPE",
	"ENCODER",
	"DECODER",
	""
};

char *string_usb_Standard_Endpoint_Descriptor_Transfer_Type[]=
{
	"Control",
	"Isochronous",
	"Bulk",
	"Interrupt",
	""
	
};
char *string_usb_Standard_Endpoint_Descriptor_Synchronization_Type[]=
{
	"No Synchronization",
	"Asynchronous",
	"Adaptive",
	"Synchronous",
	""
};
char *string_usb_Standard_Endpoint_Descriptor_Usage_Type[]=
{
	"Data endpoint",
	"Feedback endpoint",
	"Implicit feedback Data endpoint",
	"Reserved",
	""
	
};
/*A.13 Audio Class-Specific Endpoint Descriptor Subtypes
Table A-13: Audio Class-Specific Endpoint Descriptor Subtypes*/
char *string_usb_ACS_Endpoint_Descriptor_Subtypes[]=
{
	"DESCRIPTOR_UNDEFINED",
	"EP_GENERAL",
	""
	
};

/* 2.2 Input Terminal Types */
/* Terminal Types that describe Terminals that are designed to record sounds */
#if 0
char *string_TerminalType[]=
{
	"Input Undefined",		//0x0200
	"Microphone",			// 0x0201
	"Desktop microphone ",			// 0x0202
	"Personal microphone ",			// 0x0203
	"Omni-directional microphone  ",			// 0x0204
	"Microphone array   ",			// 0x0205
	"Processing microphone array   ",			// 0x0206
	""
};
#endif
const char * uac_string_TerminalType(int code)
{
	switch (code) {
	case USB_TERMTYPE_UNDEFINED:
		return "USB Undefined";
	case USB_TERMTYPE_USB_STREAMING:
		return "USB streaming";
	case USB_TERMTYPE_VENDOR_SPECIFIC:
		return "USB vendor specific ";
	case UAC_TT_INPUT_TERMTYPE_INPUT_UNDEFINED:
		return "Input Undefined";
	case UAC_TT_INPUT_TERMTYPE_MICROPHONE:
		return "Microphone  ";
	case UAC_TT_INPUT_TERMTYPE_DESKTOP_MICROPHONE:
		return "Desktop microphone";
	case UAC_TT_INPUT_TERMTYPE_PERSONAL_MICROPHONE:
		return "Personal microphone";
	case UAC_TT_INPUT_TERMTYPE_OMNIDIRECTIONAL_MICROPHONE:
		return "Omni-directional microphone";
	case UAC_TT_INPUT_TERMTYPE_MICROPHONE_ARRAY:
		return "Microphone array";
	case UAC_TT_INPUT_TERMTYPE_PROCESSING_MICROPHONE_ARRAY:
		return "Processing microphone array";
	case UAC_TT_OUTPUT_TERMTYPE_SPEAKER:
		return "Speaker";
	case UAC_TT_OUTPUT_TERMTYPE_HEADPHONES:
		return "Headphones";
	case UAC_TT_OUTPUT_TERMTYPE_HEAD_MOUNTED_DISPLAY:
		return "Head Mounted Display Audio ";

	case UAC_TT_OUTPUT_TERMTYPE_DESKTOP_SPEAKER:
		return "Desktop speaker  ";
	case UAC_TT_OUTPUT_TERMTYPE_ROOM_SPEAKER:
		return "Room speaker ";
	case UAC_TT_OUTPUT_TERMTYPE_COMMUNICATION_SPEAKER:
		return "Communication speaker";
	case UAC_TT_OUTPUT_TERMTYPE_LOW_FREQ_EFFECTS_SPEAKER:
		return "Low frequency effects speaker";
	default:
		return "**UNKNOWN**";
	}
}

/*
Appendix A.  Additional Audio Device Class Codes
A.1  Format Type Codes
Table A-1: Format Type Codes
*/
const char * uac_string_FormatType(int code)
{
	switch (code) {
	case UAC_FORMAT_TYPE_UNDEFINED:
		return "FORMAT_TYPE_UNDEFINED";
	case UAC_FORMAT_TYPE_I:
		return "FORMAT_TYPE_I";
	case UAC_FORMAT_TYPE_II:
		return "FORMAT_TYPE_II";
	case UAC_FORMAT_TYPE_III:
		return "FORMAT_TYPE_III";
	case UAC_FORMAT_TYPE_IV:
		return "FORMAT_TYPE_IV";
	case UAC_EXT_FORMAT_TYPE_I:
		return "EXT_FORMAT_TYPE_I";
	case UAC_EXT_FORMAT_TYPE_II:
		return "EXT_FORMAT_TYPE_II";
	case UAC_EXT_FORMAT_TYPE_III:
		return "EXT_FORMAT_TYPE_III";
	default:
		return "**UNKNOWN**";
	}
}
/*A.2  Audio Data Format Bit Allocation in the bmFormats field
A.2.1  Audio Data Format Type I Bit Allocations
Table A-2: Audio Data Format Type I Bit Allocations*/
const char * uac_string_Audio_Data_Format_Type_I_Bit_Allocations(int code)
{
	switch (code && 0xFFFFFFFF) {
	case UAC_FORMAT_TYPEI_PCM:
		return "PCM";
	case UAC_FORMAT_TYPEI_PCM8:
		return "PCM8";
	case UAC_FORMAT_TYPEI_IEEE_FLOAT:
		return "IEEE_FLOAT";
	case UAC_FORMAT_TYPEI_ALAW:
		return "ALAW";
	case UAC_FORMAT_TYPEI_MULAW:
		return "MULAW";
	case     UAC_FORMAT_TYPEI_RAW_DATA:
		return "TYPE_I_RAW_DATA";
	default:
		return "**UNKNOWN**";
	}
}
/*A.2.2  Audio Data Format Type II Bit Allocations
Table A-3: Audio Data Format Type II Bit Allocations*/
const char * uac_string_Audio_Data_Format_Type_II_Bit_Allocations(int code)
{
	switch (code ) {
	case UAC_FORMAT_TYPEII_MPEG:
		return "MPEG";
	case UAC_FORMAT_TYPEII_AC3:
		return "AC-3";
	case UAC_FORMAT_TYPEII_WMA:
		return "WMA";
	case UAC_FORMAT_TYPEII_DTS:
		return "DTS";
	case UAC_FORMAT_TYPEII_RAW_DATA:
		return "TYPE_II_RAW_DATA";
	default:
		return "**UNKNOWN**";
	}
}
/*A.2.3  Audio Data Format Type III Bit Allocations
Table A-4: Audio Data Format Type III Bit Allocations*/
const char * uac_string_Audio_Data_Format_Type_III_Bit_Allocations(int code)
{
	switch (code ) {
	default:
		return "**UNKNOWN**";
	}
}

static struct libusb_device_handle *uac_audio_handle = NULL;
static libusb_device *uac_audio_dev = NULL;
static int nb_ifaces_first_config = 0;

static int FU_USBIN = -1;	/* Feature Unit: USB Audio device -> host */
static int FU_USBOUT = -1;	/* Feature Unit: USB Audio host -> device*/
static int ID_IT_USB = -1;	/* Input terminal: USB streaming */
static int ID_IT_AUD = -1;	/* Input terminal: Analogue input */
static int ID_OT_USB = -1;	/* Output terminal: USB streaming */
static int ID_OT_AUD = -1;	/* Output terminal: Analogue output */

static int ID_CLKSEL = -1;	/* Clock selector ID */
static int ID_CLKSRC_INT = -1;	/* Clock source ID (internal) */
static int ID_CLKSRC_SPDIF = -1;		/* Clock source ID (external) */
static int ID_CLKSRC_ADAT = -1;		/* Clock source ID (external) */

static int ID_XU_MIXSEL = -1;
static int ID_XU_OUT = -1;
static int ID_XU_IN = -1;

static int ID_MIXER_1 = -1;

static int iso_endpoint_in = -1;
static int iso_endpoint_out = -1;

unsigned char buffer[1024*1024*20]={0}; //44100*2*2
unsigned char buffer2[1024*1024*40]={0}; //44100*2*2
unsigned char fbbuffer[4] ={0};
static int ssize,stsize,posbuff;

struct itimerval value, ovalue, value2; 
static int timeId = 0;

typedef struct
{
	struct libusb_transfer	*urb[MAX_URBS];
	unsigned char 	*transfer_buffer[MAX_URBS];
	unsigned char 	*data_buffer;
	unsigned int 	data_pos;

	unsigned int nurbs;		/* # urbs */
	
	unsigned int freqn;		/* nominal sampling rate in fs/fps in Q16.16 format */
	unsigned int freqm;		/* momentary sampling rate in fs/fps in Q16.16 format */
	int	   freqshift;		/* how much to shift the feedback value to get Q16.16 */
	unsigned int freqmax;		/* maximum sampling rate, used for buffer management */
	unsigned int phase;		/* phase accumulator */
	unsigned int maxpacksize;	/* max packet size in bytes */
	unsigned int maxframesize;      /* max packet size in frames */
	unsigned int max_urb_frames;	/* max URB size in frames */
	unsigned int curpacksize;	/* current packet size in bytes (for capture) */
	unsigned int curframesize;      /* current packet size in frames (for capture) */
	unsigned int syncmaxsize;	/* sync endpoint packet size */
	unsigned int fill_max:1;	/* fill max packet size always */
	unsigned int udh01_fb_quirk:1;	/* corrupted feedback data */
	unsigned int datainterval;      /* log_2 of data packet interval */
	unsigned int syncinterval;	/* P for adaptive mode, 0 otherwise */
	unsigned char silence_value;
	unsigned int stride;

	pthread_mutex_t lock;
	pthread_mutex_t lock2urbs;

//	unsigned int urbs;	/* number of urb  in kernel maybe more than one   in app 1? */
	unsigned int urb_packets;	/* packtes in per urb */
}EP_FMT;

 EP_FMT *ep_fmt;
 EP_FMT *sync_ep;
int uac_get_interface_associate_desc(libusb_device_handle *handle, struct libusb_config_descriptor *conf_desc, Interface_Association_Descriptor *associate)
{
	Interface_Association_Descriptor _associate;
	unsigned char associate_data[LIBUSB_DT_INT_ASSOCIATION_SIZE] = {0};
	int desc_bDescriptorType = 0;
	int desc_bLength = 0;
	int r;
	int i;
	
	#if 0
	r = libusb_get_descriptor(handle, LIBUSB_DT_INTERFACE_ASSOCIATION, 0, associate_data, LIBUSB_DT_INT_ASSOCIATION_SIZE);
	if (r < 0) 
	{
		//if (r != LIBUSB_ERROR_PIPE)
		{
			//usbi_err(handle->dev->ctx, "failed to read  INTERFACE_ASSOCIATION(%d)", r);
			printf("failed to read  INTERFACE_ASSOCIATION(%s)\n", libusb_strerror((enum libusb_error)r));
		}
		return r;
	}
	
	if (r < LIBUSB_DT_INT_ASSOCIATION_SIZE) 
	{
		//usbi_err(handle->dev->ctx, "short INTERFACE_ASSOCIATION read %d/%d", r, LIBUSB_DT_INT_ASSOCIATION_SIZE);
		printf("short INTERFACE_ASSOCIATION read %d/%d", r,LIBUSB_DT_INT_ASSOCIATION_SIZE);
		return LIBUSB_ERROR_IO;
	}

	memcpy(&associate,associate_data,LIBUSB_DT_INT_ASSOCIATION_SIZE);
	for(i = 0;i<LIBUSB_DT_INT_ASSOCIATION_SIZE;i++)
		printf("%d  ",associate_data[i]);
	printf("\n");
	#else
	if(conf_desc)
	{
		if(conf_desc->extra_length && conf_desc->extra)
		{
			desc_bLength = conf_desc->extra[0];
			desc_bDescriptorType = conf_desc->extra[1];
			//printf("		type length[%d %d]\n",desc_bDescriptorType,desc_bLength);
			if(desc_bDescriptorType == LIBUSB_DT_INTERFACE_ASSOCIATION && desc_bLength ==LIBUSB_DT_INT_ASSOCIATION_SIZE)
			{
				memcpy(associate, conf_desc->extra,LIBUSB_DT_INT_ASSOCIATION_SIZE);
				//printf("		associate.bLength[%d]\n",associate.bLength);
			}
			
		}
	}
	#endif
	

	return r;
}

//AC: audio control
// header 
int uac_get_AC_interface_header_desc(libusb_device_handle *handle, char *buffer, CS_AC_Interface_Header_Descriptor *header)
{
	int r = -1;

	if(buffer && header)
	{
		if(buffer[0] == 9 && buffer[1]  == UAC_CS_DESCTYPE_INTERFACE && buffer[2] == UAC_CS_AC_INTERFACE_SUBTYPE_HEADER)
		{
			memcpy(header,buffer,9);
			r = 0;
		}
	}
	
	return r;
}

int uac_get_AC_interface_desc(libusb_device_handle *handle, struct libusb_interface_descriptor *interface_desc,
			enum UAC_CS_AC_InterfaceDescriptorSubtype_t type, char *desc)
{
	int r = 0;
	enum UAC_CS_AC_InterfaceDescriptorSubtype_t ac_int_desc_subtype;
	uint8_t *extra = NULL;
	int extra_length = 0;
	int desc_bDescriptorType = 0;
	int desc_bLength = 0;

	if(interface_desc && interface_desc->extra_length && interface_desc->extra)
	{
		extra = (uint8_t *)interface_desc->extra;
		extra_length = interface_desc->extra_length;
		while(extra_length >= 3)
		{
			
			if(extra[1] == UAC_CS_DESCTYPE_INTERFACE)
			{
				switch(extra[2])
				{
					case UAC_CS_AC_INTERFACE_SUBTYPE_HEADER:
					{
						if(type == UAC_CS_AC_INTERFACE_SUBTYPE_HEADER)
						{
							r = uac_get_AC_interface_header_desc(handle, extra ,(CS_AC_Interface_Header_Descriptor *)desc);
							return r;
						}
						extra_length -= extra[0];
						extra += extra[0];
					}
					break;

					default:
					{
						extra_length -= extra[0];
						extra += extra[0];
					}
					break;
				}
			}
			else
			{
				extra_length -= extra[0];
				extra += extra[0];
			}
		}
	}
	return r;
}

int uac_parse_AC_interface_desc(libusb_device_handle *handle, const struct libusb_interface_descriptor *interface_desc)
{
	int r = 0;
	uint8_t *extra = NULL;
	int extra_length = 0;
	int desc_bDescriptorType = 0;
	int desc_bLength = 0;
	int i = 0;
	int ch = 0;
	int wTerminalType = 0;

	if(interface_desc && interface_desc->bInterfaceSubClass == UAC_INT_SUBCLASS_AUDIOCONTROL)
	{
		if( interface_desc->extra_length && interface_desc->extra)
		{
			extra = (uint8_t *)interface_desc->extra;
			extra_length = interface_desc->extra_length;
			while(extra_length > 3)
			{
				if(extra[1] == UAC_CS_DESCTYPE_INTERFACE)
				{
					switch(extra[2])
					{
						case UAC_CS_AC_INTERFACE_SUBTYPE_HEADER:
						{
							printf("      Class-Specific AudioControl Interface Header Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bcdADC  \t\t    0x%04x\n",(extra[3] |extra[4]<<8));
							printf("        bCategory  \t\t    %d\n",extra[5]);
							printf("        wTotalLength  \t\t    0x%04x[%d]\n",(extra[6]|extra[7]<<8),(extra[6]|extra[7]<<8));
							printf("        bmControls  \t\t    %d\n",extra[8]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBTYPE_INPUT_TERMINAL:
						{
							printf("      Class-Specific AudioControl Interface Input Terminal Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bTerminalID  \t\t    %d\n",extra[3] );
							printf("        wTerminalType  \t\t    0x%04x  %s\n",extra[4]|extra[5]<<8,uac_string_TerminalType(extra[4]|extra[5]<<8));
							wTerminalType = extra[4]|extra[5]<<8;
							if(wTerminalType == USB_TERMTYPE_USB_STREAMING)
							{
								if(ID_IT_USB == -1)
									ID_IT_USB = extra[3];
							}
							else if(wTerminalType == UAC_TT_INPUT_TERMTYPE_MICROPHONE)
							{
								if(ID_IT_AUD == -1)
									ID_IT_AUD = extra[3];
							}
							printf("        bAssocTerminal  \t    %d\n", extra[6]);
							printf("        bCSourceID  \t\t    %d\n",extra[7]);
							printf("        bNrChannels  \t\t    %d\n",extra[8]);
							printf("        bmChannelConfig  \t    %d\n",extra[9] | extra[10]<<8|extra[11]<<16|extra[12]<<24);
							printf("        iChannelNames  \t\t    %d\n",extra[13]);
							printf("        bmControls  \t\t    %d\n",extra[14]|extra[15]<<8);
							printf("        iTerminal  \t\t    %d\n",extra[16]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBTYPE_OUTPUT_TERMINAL:
						{
							printf("      Class-Specific AudioControl Interface Output Terminal Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bTerminalID  \t\t    %d\n",extra[3] );
							printf("        wTerminalType  \t\t    0x%04x  %s\n",extra[4]|extra[5]<<8,uac_string_TerminalType(extra[4]|extra[5]<<8));
							wTerminalType = extra[4]|extra[5]<<8;
							if(wTerminalType == USB_TERMTYPE_USB_STREAMING)
							{
								if(ID_OT_USB == -1)
									ID_OT_USB = extra[3];
							}
							else if(wTerminalType == UAC_TT_OUTPUT_TERMTYPE_SPEAKER)
							{
								if(ID_OT_AUD == -1)
									ID_OT_AUD = extra[3];
							}
							printf("        bAssocTerminal  \t    %d\n", extra[6]);
							printf("        bSourceID  \t\t    %d\n",extra[7]);
							printf("        bCSourceID  \t\t    %d\n",extra[8]);
							printf("        bmControls  \t\t    %d\n",extra[9]|extra[10]<<8);
							printf("        iTerminal	  \t    %d\n",extra[16]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBTYPE_MIXER_UNIT:
						{
							printf("      Class-Specific AudioControl Interface Clock Selector Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bUnitID  \t\t    %d\n",extra[3]);
							printf("        bNrInPins  \t\t    %d\n",extra[4]);
							for(i = 0; i<extra[4]; i++)
							{
							printf("        baSourceID[%d]  \t    %d\n",i, extra[5+i]);
							
							}
							printf("        bNrChannels  \t\t    %d\n",extra[5+extra[4]]);
							printf("        bmChannelConfig  \t    %d\n",extra[6+extra[4]] | extra[7+extra[4]]<<8 | extra[8+extra[4]]<<16 |extra[9+extra[4]]<<24);
							printf("        iChannelNames  \t    %d\n",extra[10+extra[4]]);
							printf("        bmMixerControls  \t    %d\n",extra[11+extra[4]]);
							printf("        bmControls  \t\t    0x%02x\n",extra[extra[0]-2]);
							printf("        iMixer  \t\t    %d\n",extra[extra[0]-1]);
						}
						break;

						case UAC_CS_AC_INTERFACE_SUBTYPE_SELECTOR_UNIT:
						{
							printf("      Class-Specific AudioControl Interface Selector Unit Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bUnitID  \t\t    %d\n",extra[3]);
							printf("        bNrInPins  \t\t    %d\n",extra[4]);
							for(i = 0; i<extra[4]; i++)
							{
							printf("        baSourceID[%d]  \t    %d\n",i, extra[5+i]);
							
							}
							printf("        bmControls  \t\t    0x%02x\n",extra[extra[0]-2]);
							printf("        iSelector  \t\t    %d\n",extra[extra[0]-1]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBTYPE_FEATURE_UNIT:
						{
							printf("      Class-Specific AudioControl Interface Feature Unit Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bUnitID  \t\t    %d\n",extra[3]);
							printf("        bSourceID  \t\t    %d\n",extra[4]);
							if(extra[4] == ID_IT_USB)
							{
								if(FU_USBOUT == -1)
									FU_USBOUT = extra[3];
							}
							else if(extra[4] == ID_OT_USB)
							{
								if(FU_USBIN == -1)
									FU_USBIN = extra[3];
							}
							ch = (extra[0] -6)/4;
							for(i = 0; i<ch; i++)
							{
							printf("        bmaControls[%d]  \t    0x%02x\n",i, extra[5+i*4]|extra[5+i*4+1]<<8|extra[5+i*4+2]<<16|extra[5+i*4+3]<<24 );
							
							}
							printf("        iSelector  \t\t    %d\n",extra[extra[0]-1]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBTYPE_EFFECT_UNIT:
						{
							printf("      Class-Specific AudioControl Interface Common Part of the Effect Unit Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bUnitID  \t\t    %d\n",extra[3]);
							printf("        wEffectType  \t\t    0x%04x\n",extra[4]|extra[5]<<8);
							printf("        bSourceID  \t\t    %d\n",extra[6]);
							ch = (extra[0] -16)/4;
							for(i = 0; i<ch; i++)
							{
							printf("        bmaControls[%d]  \t    0x%02x\n",i, extra[7+i*4]|extra[7+i*4+1]<<8|extra[7+i*4+2]<<16|extra[7+i*4+3]<<24 );
							
							}
							printf("        iSelector  \t\t    %d\n",extra[extra[0]-1]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBTYPE_PROCESSING_UNIT:
						{
							printf("      Class-Specific AudioControl Interface Common Part of the Processing Unit Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bUnitID  \t\t    %d\n",extra[3]);
							printf("        wProcessType  \t\t    %d\n",extra[4]|extra[5]<<8);
							printf("        bNrInPins  \t\t    %d\n",extra[6]);
							for(i = 0; i<extra[6]; i++)
							{
							printf("        baSourceID[%d]  \t    %d\n",i, extra[7+i]);
							
							}
							printf("        bmChannelConfig  \t    %d\n",extra[8+extra[6]] | extra[9+extra[6]]<<8 | extra[10+extra[6]]<<16 |extra[11+extra[6]]<<24);
							printf("        iChannelNames  \t    %d\n",extra[12+extra[6]]);
							printf("        bmControls  \t\t    0x%04x\n",extra[13+extra[6]]|extra[14+extra[6]]<<8);
							printf("        iProcessing  \t\t    %d\n",extra[extra[0]-2]);
							printf("        Process-specific  \t    %d\n",extra[extra[0]-1]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBTYPE_EXTENSION_UNIT:
						{
							printf("      Class-Specific AudioControl Interface Common Part of the Processing Unit Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bUnitID  \t\t    %d\n",extra[3]);
							printf("        wExtensionCode  \t    %d\n",extra[4]|extra[5]<<8);
							printf("        bNrInPins  \t\t    %d\n",extra[6]);
							for(i = 0; i<extra[6]; i++)
							{
							printf("        baSourceID[%d]  \t    %d\n",i, extra[7+i]);
							
							}
							printf("        bNrChannels  \t\t    %d\n",extra[8+extra[6]]);
							printf("        bmChannelConfig  \t    %d\n",extra[9+extra[6]] | extra[10+extra[6]]<<8 | extra[11+extra[6]]<<16 |extra[12+extra[6]]<<24);
							printf("        iChannelNames  \t    %d\n",extra[13+extra[6]]);
							printf("        bmControls  \t\t    %d\n",extra[14+extra[6]]);
							printf("        iExtension  \t\t    %d\n",extra[15+extra[6]]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBTYPE_CLOCK_SOURCE:
						{
							printf("      Class-Specific AudioControl Interface Clock Source Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bClockID  \t\t    %d\n",extra[3]);
							if(ID_CLKSRC_INT == -1)
								ID_CLKSRC_INT = extra[3];
							printf("        bmAttributes  \t\t    0x%02x\n",extra[4]);
							printf("        bmControls  \t\t    0x%02x\n",extra[5]);
							printf("        bAssocTerminal  \t    %d\n",extra[6]);
							printf("        iClockSource  \t\t    %d\n",extra[7]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBTYPE_CLOCK_SELECTOR:
						{
							printf("      Class-Specific AudioControl Interface Clock Selector Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bClockID  \t\t    %d\n",extra[3]);
							if(ID_CLKSEL == -1)
								ID_CLKSEL = extra[3];
							printf("        bNrInPins  \t\t    %d\n",extra[4]);
							for(i = 0; i<extra[4]; i++)
							{
							printf("        baCSourceID[%d]  \t    %d\n",i, extra[5+i]);
							
							}
							printf("        bmControls  \t\t    0x%02x\n",extra[5+extra[4]]);
							printf("        iClockSelector  \t    %d\n",extra[6+extra[4]]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBSYPE_CLOCK_MULTIPLIER:
						{
							printf("      Class-Specific AudioControl Interface Clock Multiplier Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bClockID  \t\t    %d\n",extra[3]);
							printf("        bCSourceID  \t\t    %d\n",extra[4]);
							printf("        bmControls  \t\t    0x%02x\n",extra[5]);
							printf("        iClockMultiplier  \t    %d\n",extra[7]);
						}
						break;
						case UAC_CS_AC_INTERFACE_SUBTYPE_SAMPLE_RATE_CONVERTER:
						{
							printf("      Class-Specific AudioControl Interface  Sampling Rate Converter Unit Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AC_Int_Desc_Subtypes[extra[2]]);
							printf("        bUnitID  \t\t    %d\n",extra[3]);
							printf("        bSourceID  \t\t    %d\n",extra[4]);
							printf("        bCSourceInID  \t\t    %d\n",extra[5]);
							printf("        bCSourceOutID  \t\t    %d\n",extra[6]);
							printf("        iSRC  \t\t    %d\n",extra[7]);
						}
						break;
						default:
						{
						}
						break;
					}
					extra_length -= extra[0];
					extra += extra[0];
				}
				else
				{
					extra_length -= extra[0];
					extra += extra[0];
				}
	
			}
		}
	}

	return r;
}

int uac_parse_AS_interface_desc(libusb_device_handle *handle, const struct libusb_interface_descriptor *interface_desc)
{
	int r = 0;
	uint8_t *extra = NULL;
	int extra_length = 0;
	int desc_bDescriptorType = 0;
	int desc_bLength = 0;
	int i = 0;
	int ch = 0;
	
	if(interface_desc && interface_desc->bInterfaceSubClass == UAC_INT_SUBCLASS_AUDIOSTREAMING)
	{
		if( interface_desc->extra_length && interface_desc->extra)
		{
			extra = (uint8_t *)interface_desc->extra;
			extra_length = interface_desc->extra_length;
			while(extra_length > 3)
			{
				if(extra[1] == UAC_CS_DESCTYPE_INTERFACE)
				{
					switch(extra[2])
					{
						case UAC_CS_AS_INTERFACE_SUBTYPE_AS_GENERAL:
						{
							printf("      Class-Specific AudioStreaming Interface Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AS_Int_Desc_Subtypes[extra[2]]);
							printf("        bTerminalLink  \t\t    %d\n",extra[3]);
							printf("        bmControls  \t\t    %d\n",extra[4]);
							printf("        bFormatType  \t\t    %d\n",extra[5],string_uac_AS_Int_Desc_Subtypes[extra[5]]);
							printf("        bmFormats  \t\t    0x%08x   ",extra[6]|extra[7]<<8|extra[8]<<16|extra[9]<<24);
							switch(extra[5])
							{
								case UAC_FORMAT_TYPE_I:
									printf("(%s)\n",uac_string_Audio_Data_Format_Type_I_Bit_Allocations(extra[6]|extra[7]<<8|extra[8]<<16|extra[9]<<24));
								break;
								case UAC_FORMAT_TYPE_II:
									printf("(%s)\n",uac_string_Audio_Data_Format_Type_II_Bit_Allocations(extra[6]|extra[7]<<8|extra[8]<<16|extra[9]<<24));
								break;
								default:
								break;
							}
							printf("        bNrChannels  \t\t    %d\n",extra[10]);
							printf("        bmChannelConfig  \t    0x%08x\n",extra[11]|extra[12]<<8|extra[13]<<16|extra[14]<<24);
							printf("        iChannelNames  \t\t    %d\n",extra[15]);
						}
						break;
						case UAC_CS_AS_INTERFACE_SUBTYPE_FORMAT_TYPE:
						{
							printf("      Class-Specific AudioStreaming Interface Format Type Descriptor:\n");
							printf("        bLength  \t\t    %d\n",extra[0]);
							printf("        bDescriptorType  \t    %d\n",extra[1]);
							printf("        bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_uac_AS_Int_Desc_Subtypes[extra[2]]);
							printf("        bFormatType  \t\t    %d  (%s)\n",extra[3], uac_string_FormatType(extra[3]));
							switch(extra[3])
							{
								case UAC_FORMAT_TYPE_I:
									printf("        bSubslotSize  \t\t    %d\n",extra[4]);
									printf("        bBitResolution  \t    %d\n",extra[5]);
								break;
								case UAC_FORMAT_TYPE_II:
								break;
								default:
								break;
							}
						}
						break;
						default:
						{
						}
						break;
					}
					extra_length -= extra[0];
					extra += extra[0];
				}
				else
				{
					extra_length -= extra[0];
					extra += extra[0];
				}
	
			}
		}	
	}

	return r;
}

int uac_parse_AS_Endpoint_desc(libusb_device_handle *handle, const struct libusb_endpoint_descriptor *endpoint_desc)
{
	int r = 0;
	uint8_t *extra = NULL;
	int extra_length = 0;
	int desc_bDescriptorType = 0;
	int desc_bLength = 0;
	int i = 0;
	int ch = 0;

		if( endpoint_desc->extra_length && endpoint_desc->extra)
		{
			extra = (uint8_t *)endpoint_desc->extra;
			extra_length = endpoint_desc->extra_length;
			while(extra_length > 3)
			{
				if(extra[1] == UAC_CS_DESCTYPE_ENDPOINT && extra[2] == UAC_CS_ENDPOINT_SUBTYPE_EP_GENERAL )
				{
					printf("        AudioStreaming Endpoint Descriptor :\n");
					printf("          bLength  \t\t    %d\n",extra[0]);
					printf("          bDescriptorType  \t    %d\n",extra[1]);
					printf("          bDescriptorSubtype  \t    %d  (%s)\n",extra[2],string_usb_ACS_Endpoint_Descriptor_Subtypes[extra[2]]);
					printf("          bmAttributes  \t    0x%02x\n",extra[3]);
					printf("          bmControls  \t\t    0x%02x\n",extra[4]);
					printf("          bLockDelayUnits  \t    %d\n",extra[5]);
					printf("          wLockDelay  \t\t    %d\n",extra[6]|extra[7]);
					extra_length -= extra[0];
					extra += extra[0];
				}
				else
				{
					extra_length -= extra[0];
					extra += extra[0];
				}
	
			}
		}	
	

	return r;
}

/*
bmRequest Type  		| bRequest	| wValue		| wIndex		| wLength | Data
00100001B				CUR			CS			Entity ID
10100001B				RANGE		and			and
									CN or MCN	Interface

00100010B										Endpoint
10100010B

Set request (D7 = 0b0)	CUR=0x01	Control Selector (CS) HB
Get request (D7 = 0b1)	RANGE=0x02  Channel Number (CN) LB
class-specific request 
(D6..5 = 0b01)
 an interface 
 (AudioControl or
AudioStreaming) 
of the audio function
(D4..0 = 0b00001)
 the isochronous endpoint 
 of an
AudioStreaming 
interface 
(D4..0 = 0b00010)
*/
/*
how to enlope the control data;
1:bRequest-->CUR/RANGE
		Inspect request, NOTE: these are class specific requests
2:wIndex >> 8 --->ID_CLKSRC_INT/ID_CLKSRC_SPDIF/ID_CLKSRC_ADAT/ID_CLKSEL/FU_USBOUT/FU_USBIN/
		Extract unitID from wIndex
3:wValue >> 8 --->CS_SAM_FREQ_CONTROL/CS_CLOCK_VALID_CONTROL/.../FU_VOLUME_CONTROL/FU_MUTE_CONTROL
		
*/
static int s_volume = 0x8100;	// min 0x8100 max 0xff00
int uac_request_set_device(enum UAC_Set_Device_Type_t type, int value)
{
	int r = 0;
	unsigned char  data[0x10] = {0};

	if(FU_USBOUT < 0)
	{
		printf("FU_USBOUT  is error\n");
		return -1;
	}

	printf("uac_request_set_device\n");
	switch(type)
	{
		case UAC_SET_DEVICE_VOLUME:
		{
			//handle, bmRequest Type, bRequest, wValue , wIndex, *data, wLength, timeout
			//libusb_control_transfer(uac_audio_handle , 0x21, UAC_ACS_REQUEST_CUR, 0, );
			data[0] = value &0xFF;
			data[1] = (value>>8) & 0xFF;
			libusb_control_transfer(uac_audio_handle , (uint8_t)0x21, UAC_ACS_REQUEST_CUR,  1 | (FU_VOLUME_CONTROL<<8),
								0|(FU_USBOUT<<8), data, 0x02,1000);
			libusb_control_transfer(uac_audio_handle , (uint8_t)0x21, UAC_ACS_REQUEST_CUR,  2 | (FU_VOLUME_CONTROL<<8),
								0|(FU_USBOUT<<8), data, 0x02,1000);
		}
		break;

		case UAC_SET_DEVICE_CHANNEL:
		{
			//handle, bmRequest Type, bRequest, wValue , wIndex, *data, wLength, timeout
			//libusb_control_transfer(uac_audio_handle , 0x21, UAC_ACS_REQUEST_CUR, 0, );
			data[0] = value &0xFF;
			data[1] = (value>>8) & 0xFF;
			libusb_control_transfer(uac_audio_handle , (uint8_t)0x21, UAC_ACS_REQUEST_CUR,  1 | (FU_VOLUME_CONTROL<<8),
								0|(FU_USBOUT<<8), data, 0x02,1000);
			libusb_control_transfer(uac_audio_handle , (uint8_t)0x21, UAC_ACS_REQUEST_CUR,  2 | (FU_VOLUME_CONTROL<<8),
								0|(FU_USBOUT<<8), data, 0x02,1000);
		}
		break;
		
		case UAC_SET_DEVICE_SAMPLERATE:
		{
			//handle, bmRequest Type, bRequest, wValue , wIndex, *data, wLength, timeout
			//libusb_control_transfer(uac_audio_handle , 0x21, UAC_ACS_REQUEST_CUR, 0, );
			data[0] = value &0xFF;
			data[1] = (value>>8) & 0xFF;
			data[2] = (value>>16) & 0xFF;
			data[3] = (value>>24) & 0xFF;
			libusb_control_transfer(uac_audio_handle , (uint8_t)0x21, UAC_ACS_REQUEST_CUR,  0 | (CS_SAM_FREQ_CONTROL<<8),
								0|(ID_CLKSRC_INT<<8), data, 0x04,1000);
		}
		break;
		
		case UAC_SET_DEVICE_RESOLUTION:
		{
		}
		break;
		
		case UAC_SET_DEVICE_MUTE:
		{
			/*
			  29.0  CTL    21 01 01 01  00 0a 01 00  SET CUR                 	25.1.0        10:03:26.273  
			  29.0  OUT    01                        .                       			25.2.0        10:03:26.273  
			  29.0  CTL    21 01 02 01  00 0a 01 00  SET CUR                 	26.1.0        10:03:26.273  
			  29.0  OUT    01                        .                       			26.2.0        10:03:26.274  
			*/
			data[0] = value &0xFF;
			libusb_control_transfer(uac_audio_handle , (uint8_t)0x21, UAC_ACS_REQUEST_CUR,  1 | (FU_MUTE_CONTROL<<8),
								0|(FU_USBOUT<<8), data, 0x01,1000);
			libusb_control_transfer(uac_audio_handle , (uint8_t)0x21, UAC_ACS_REQUEST_CUR,  2 | (FU_MUTE_CONTROL<<8),
								0|(FU_USBOUT<<8), data, 0x01,1000);
		}
		break;
		
		default:
		{
		}
		break;
	}
	return r;
}

static struct libusb_transfer *uac_xfr = NULL; 
static int xfr_complete = 0;
struct timeval start, end;
static void  cb_xfr(struct libusb_transfer *xfr)
{
	unsigned int i;
//	printf("cb_xfr...\n");
	libusb_free_transfer(xfr);
	//uac_iso_transfer_feedback();
//	xfr_complete = 1;
	if(posbuff<(stsize-ssize))
	{
		uac_iso_transfer(buffer2+posbuff, ssize);
//		gettimeofday( &start, NULL );
//		printf("start : %d.%d\n", (int)start.tv_sec, (int)start.tv_usec);
		posbuff += ssize;
	}
	else
	{
		printf("end of file\n");
		value.it_value.tv_sec = 0;
		value.it_value.tv_usec = 0;	// 10ms
		value.it_interval.tv_sec = 0;
		value.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &value, &ovalue);    //(2)

	}

	return;

}

static void  cb_xfr_fb(struct libusb_transfer *xfr)
{
	unsigned int i;
//	printf("cb_xfr...\n");
//	libusb_free_transfer(xfr);
//	uac_iso_transfer_feedback();
//	xfr_complete = 1;
	if(posbuff<(stsize-ep_fmt->maxpacksize*ep_fmt->urb_packets))
	{
		uac_iso_transfer2(buffer2+posbuff, &ssize);
//		gettimeofday( &start, NULL );
//		printf("start : %d.%d\n", (int)start.tv_sec, (int)start.tv_usec);
//		posbuff += ssize;
	}
	else
	{
		system("date");
		printf("end of file\n");
		printf("posbuff: %d\n",posbuff);
		value.it_value.tv_sec = 0;
		value.it_value.tv_usec = 0;	// 10ms
		value.it_interval.tv_sec = 0;
		value.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &value, &ovalue);    //(2)

	}

	return;

}

static void  cb_xfr_fb_urbs(struct libusb_transfer *xfr)
{
	unsigned int i;
//	printf("cb_xfr...\n");
//	libusb_free_transfer(xfr);
//	uac_iso_transfer_feedback();
//	xfr_complete = 1;
//	if(posbuff<(stsize-ssize))
	{
		uac_iso_transfer_urbs(xfr,buffer2+ep_fmt->data_pos, &ssize);
//		gettimeofday( &start, NULL );
//		printf("start : %d.%d\n", (int)start.tv_sec, (int)start.tv_usec);
//		posbuff += ssize;
	}

	return;

}

static void  cb_xfr2(struct libusb_transfer *xfr)
{
	unsigned int i;
	
	unsigned int f;
//	printf("cb_xfr2...\n");

//	printf("transfer status %d\n", xfr->status);

	for (i = 0; i < xfr->num_iso_packets; i++) {
		struct libusb_iso_packet_descriptor *pack = &xfr->iso_packet_desc[i];

		if (pack->status != LIBUSB_TRANSFER_COMPLETED) {
			fprintf(stderr, "Error: pack %u status %d\n", i, pack->status);
			exit(5);
		}

//		printf("pack%u length:%u, actual_length:%u\n", i, pack->length, pack->actual_length);
	}

//	printf("length:%u, actual_length:%u\n", xfr->length, xfr->actual_length);

//	for (i = 0; i < xfr->actual_length; i++) {
//	for (i = 0; i < xfr->length; i++) {
//		printf("%02x  ", xfr->buffer[i]);
//	}
//	printf("\n");

	f= xfr->buffer[0] |xfr->buffer[1]<<8 |xfr->buffer[2]<<16 | xfr->buffer[3]<<24;
	if (xfr->iso_packet_desc[0].actual_length == 3)
		f &= 0x00ffffff;
	else
		f &= 0x0fffffff;

//	printf("f :0x%08x \n" , f);
	f <<= ep_fmt->freqshift;

	if ((f >= ep_fmt->freqn - ep_fmt->freqn / 8 && f <= ep_fmt->freqmax)) 
	{
		pthread_mutex_lock(&ep_fmt->lock);
		ep_fmt->freqm = f;
		pthread_mutex_unlock(&ep_fmt->lock);
	//	printf("ep_fmt->freqm :0x%08x \n" , ep_fmt->freqm);
	}
	
	libusb_free_transfer(xfr);

//	uac_iso_transfer_feedback(fbbuffer,4);
	return;

}
#define TRANSFER_PACKET		(392/2)
#define TRANSFER_COUNT		100
#define TRANSFER_LENGTH		(TRANSFER_PACKET*TRANSFER_COUNT)

#define DELAY_XFER	(1000*10)			// us

int uac_iso_init(void)
{
	uac_xfr = libusb_alloc_transfer(ep_fmt->urb_packets);
	if (!uac_xfr)
		return -ENOMEM;

	
	return 0;
}
int uac_iso_init_urbs(void)
{
	int i=0;
	for(i=0;i<ep_fmt->nurbs;i++)
	{
		ep_fmt->urb[i] = libusb_alloc_transfer(ep_fmt->urb_packets);
		if (!ep_fmt->urb[i])
		{
			printf("uac_iso_init_urbs [%d] failed\n",i);
			return -ENOMEM;
		}

		ep_fmt->transfer_buffer[i] = malloc(ep_fmt->maxpacksize*ep_fmt->urb_packets);
		if(!ep_fmt->transfer_buffer[i])
		{
			printf("malloc transfer_buffer [%d] failed\n",i);
			return -ENOMEM;
		}
		memset(ep_fmt->transfer_buffer[i],0,ep_fmt->maxpacksize*ep_fmt->urb_packets);
	}
	
	return 0;
}

int uac_iso_transfer(uint8_t *buffer, int size)
{
	static int num_iso_pack = 0;
	struct libusb_transfer *uac_xfr_tmp = NULL; 
	
	//num_iso_pack = 16;
	num_iso_pack = TRANSFER_LENGTH/TRANSFER_PACKET;
	num_iso_pack = size/44;

	uac_xfr_tmp = libusb_alloc_transfer(num_iso_pack);
	if (!uac_xfr_tmp)
		return -ENOMEM;

	libusb_fill_iso_transfer(uac_xfr_tmp, uac_audio_handle, 1, buffer,
					size, num_iso_pack, cb_xfr, NULL, 1000);
	libusb_set_iso_packet_lengths(uac_xfr_tmp, size/num_iso_pack);

	return libusb_submit_transfer(uac_xfr_tmp);
}

int uac_iso_transfer2(uint8_t *buffer, int *size)
{
	static int num_iso_pack = 0;
	struct libusb_transfer *uac_xfr_tmp = NULL; 
	int i = 0;
	int counts;
	int ret;

	if(uac_xfr == NULL)
	{
		printf("uac_xfr is NULL\n");
		return -1;
	}
	//num_iso_pack = 16;
//	num_iso_pack = TRANSFER_LENGTH/TRANSFER_PACKET;
//	num_iso_pack = size/44;
//	printf("[%s,%d]\n",__FUNCTION__,__LINE__);
	num_iso_pack = ep_fmt->urb_packets;
//	printf("[%s,%d] num_iso_pack : %d\n",__FUNCTION__,__LINE__,num_iso_pack);

	libusb_fill_iso_transfer(uac_xfr, uac_audio_handle, 1, buffer,
					0, num_iso_pack, cb_xfr_fb, NULL, 0);
//	printf("[%s,%d]\n",__FUNCTION__,__LINE__);
	uac_xfr->length = 0;
//	printf("[%s,%d]\n",__FUNCTION__,__LINE__);
//	libusb_set_iso_packet_lengths(uac_xfr, size/num_iso_pack);
//	for(i=0; i<ep_fmt->urb_packets;i++)
	for(i=0; i<num_iso_pack;i++)
	{
//		printf("[%s,%d]\n",__FUNCTION__,__LINE__);
		counts = uac_usb_endpoint_next_packet_size(ep_fmt);
//		printf("%d \n",counts);
//		counts=5;

		uac_xfr->iso_packet_desc[i].length = counts * ep_fmt->stride;
//		printf("[%s,%d] uac_xfr->iso_packet_desc[%d].length : %d\n",__FUNCTION__,__LINE__,i,uac_xfr->iso_packet_desc[i].length);
		uac_xfr->length += uac_xfr->iso_packet_desc[i].length;
//		printf("[%s,%d]\n",__FUNCTION__,__LINE__);
	}
	*size = uac_xfr->length;
	posbuff += uac_xfr->length;
//		printf("[%s,%d] uac_xfr->length:%d\n",__FUNCTION__,__LINE__,uac_xfr->length);

	ret = libusb_submit_transfer(uac_xfr);
//		printf("[%s,%d] ret: %d\n",__FUNCTION__,__LINE__,ret);
	return ret; 
}


int uac_iso_transfer_urbs(struct libusb_transfer *xfr, uint8_t *buffer, int *size)
{
	static int num_iso_pack = 0;
	struct libusb_transfer *uac_xfr_tmp = xfr; 
	int i = 0 ,j ;
	int counts;
	int ret;

	
	pthread_mutex_lock(&ep_fmt->lock2urbs);
	if((ep_fmt->data_pos+ep_fmt->maxpacksize*ep_fmt->urb_packets) > stsize)
	{
		pthread_mutex_unlock(&ep_fmt->lock2urbs);
		printf("end of file data_pos : %d\n",ep_fmt->data_pos);
		system("date");
		return 0;
	}

	num_iso_pack = ep_fmt->urb_packets;
	libusb_fill_iso_transfer(uac_xfr_tmp, uac_audio_handle, 1, uac_xfr_tmp->buffer,
			0, num_iso_pack, cb_xfr_fb_urbs, NULL, 1000);
	uac_xfr_tmp->length = 0;
	for(j=0; j<num_iso_pack;j++)
	{
		counts = uac_usb_endpoint_next_packet_size(ep_fmt);

		uac_xfr_tmp->iso_packet_desc[j].length = counts * ep_fmt->stride;
		uac_xfr_tmp->length += uac_xfr_tmp->iso_packet_desc[j].length;
	}
	memset(uac_xfr_tmp->buffer,0,ep_fmt->maxpacksize*ep_fmt->urb_packets);
	memcpy(uac_xfr_tmp->buffer,ep_fmt->data_buffer+ep_fmt->data_pos,uac_xfr_tmp->length);
	ep_fmt->data_pos +=  uac_xfr_tmp->length;
	pthread_mutex_unlock(&ep_fmt->lock2urbs);

	ret = libusb_submit_transfer(uac_xfr_tmp);
	return ret; 
}

int start_iso_transfer(void)
{
	int ret = 0;
	int i,j;
	int num_iso_pack = 0;
	int counts;
	struct libusb_transfer *uac_xfr_tmp = NULL; 

	ep_fmt->data_buffer = buffer2;
	ep_fmt->data_pos = 0;

	for(i=0;i<ep_fmt->nurbs;i++)
	{
		if(ep_fmt->urb[i])
		{
			uac_xfr_tmp = ep_fmt->urb[i];
			num_iso_pack = ep_fmt->urb_packets;
			libusb_fill_iso_transfer(uac_xfr_tmp, uac_audio_handle, 1, ep_fmt->transfer_buffer[i],
					0, num_iso_pack, cb_xfr_fb_urbs, NULL, 1000);
			uac_xfr_tmp->length = 0;
			for(j=0; j<num_iso_pack;j++)
			{
				counts = uac_usb_endpoint_next_packet_size(ep_fmt);

				uac_xfr_tmp->iso_packet_desc[j].length = counts * ep_fmt->stride;
				uac_xfr_tmp->length += uac_xfr_tmp->iso_packet_desc[j].length;
			}
			memset(ep_fmt->transfer_buffer[i],0,ep_fmt->maxpacksize*ep_fmt->urb_packets);
			memcpy(ep_fmt->transfer_buffer[i],ep_fmt->data_buffer+ep_fmt->data_pos,uac_xfr_tmp->length);
			ep_fmt->data_pos +=  uac_xfr_tmp->length;
				
			ret += uac_xfr_tmp->length;

			if(libusb_submit_transfer(uac_xfr_tmp) == 0)
			{
				//return ret;
			}
			else
			{
				printf("submit failed\n");
				return -1;
			}
		}
		
	}


	return ret;
}


int uac_iso_transfer_feedback(uint8_t *buffer, int size)
{
	static int num_iso_pack = 0;
	struct libusb_transfer *uac_xfr_tmp = NULL; 
	
	//num_iso_pack = 16;
//	num_iso_pack = TRANSFER_LENGTH/TRANSFER_PACKET;
	num_iso_pack = 1;
//	num_iso_pack = TRANSFER_LENGTH/392;

	uac_xfr_tmp = libusb_alloc_transfer(num_iso_pack);
	if (!uac_xfr_tmp)
		return -ENOMEM;

	libusb_fill_iso_transfer(uac_xfr_tmp, uac_audio_handle, 0x81, buffer,
					size, num_iso_pack, cb_xfr2, NULL, 1000);
	libusb_set_iso_packet_lengths(uac_xfr_tmp, size/num_iso_pack);

	return libusb_submit_transfer(uac_xfr_tmp);
}

int uac_audio_open(int vid, int pid)
{
	int rc = 0;
	struct libusb_device_descriptor dev_desc;
	struct libusb_config_descriptor *conf_desc;
	const struct libusb_endpoint_descriptor *endpoint;
	Interface_Association_Descriptor associate;
	int iface, nb_ifaces, first_iface = -1;
	int nb_iconfig;
	int m,i = 0, j ,k,r;
	uint8_t endpoint_in = 0, endpoint_out = 0;
	char string[128];
	uint8_t string_index[10]={0};	// indexes of the string descriptors

	rc = libusb_init(NULL);
	if (rc < 0) 
	{
//		fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
//		exit(1);
	}
	
//	libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_INFO);
	
	uac_audio_handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (!uac_audio_handle) {
		fprintf(stderr, "Error finding USB audio device\n");
//		goto out;
	}

	uac_audio_dev = libusb_get_device(uac_audio_handle);
	
	libusb_get_device_descriptor(uac_audio_dev, &dev_desc);
	printf("Device Descriptor:\n");
	printf("  bLength  \t\t%d\n", dev_desc.bLength);
	printf("  bDescriptorType  \t%d\n", dev_desc.bDescriptorType);
	printf("  bcdUSB  \t\t0x%04x\n", dev_desc.bcdUSB);
	printf("  bDeviceClass  \t%d\n", dev_desc.bDeviceClass);
	printf("  bDeviceSubClass  \t%d\n", dev_desc.bDeviceSubClass);
	printf("  bDeviceProtocol  \t%d\n", dev_desc.bDeviceProtocol);
	printf("  bMaxPacketSize0  \t%d\n", dev_desc.bMaxPacketSize0);
	printf("  idVendor  \t\t0x%04x\n", dev_desc.idVendor);
	printf("  idProduct  \t\t0x%04x\n", dev_desc.idProduct);
	printf("  bcdDevice  \t\t0x%04x\n", dev_desc.bcdDevice);
	printf("  iManufacturer  \t%d\n", dev_desc.iManufacturer);
	printf("  iProduct  \t\t%d\n", dev_desc.iProduct);
	printf("  iSerialNumber  \t%d\n", dev_desc.iSerialNumber);
	printf("  bNumConfigurations  \t%d\n", dev_desc.bNumConfigurations);


	for(m = 0;m<dev_desc.bNumConfigurations;m++)
	{
		libusb_get_config_descriptor(uac_audio_dev, m, &conf_desc);
		printf("  Config Descriptor(%d):\n",m);
		printf("    bLength  \t\t  %d\n", conf_desc->bLength);
		printf("    bDescriptorType  \t  %d\n", conf_desc->bDescriptorType);
		printf("    wTotalLength  \t  %d\n", conf_desc->wTotalLength);
		printf("    bNumInterfaces  \t  %d\n", conf_desc->bNumInterfaces);
		printf("    bConfigurationValue    %d\n", conf_desc->bConfigurationValue);
		printf("    iConfiguration  \t  %d\n", conf_desc->iConfiguration);
		printf("    bmAttributes  \t  0x%02x\n", conf_desc->bmAttributes);
		printf("    MaxPower  \t\t  %dmA\n", conf_desc->MaxPower);
		printf("    extra  \t\t  %p\n", conf_desc->extra);
		printf("    extra_length \t  %d\n", conf_desc->extra_length);

		//libusb_get_descriptor(uac_audio_dev, LIBUSB_DT_INTERFACE_ASSOCIATION, 0, unsigned char * data, int length);
		memset(&associate,0,LIBUSB_DT_INT_ASSOCIATION_SIZE);
		uac_get_interface_associate_desc(uac_audio_handle, conf_desc, &associate);
		printf("    Interface Association Descriptor:\n");
		printf("      bLength  \t\t    %d\n",associate.bLength);
		printf("      bDescriptorType  \t    %d\n",associate.bDescriptorType);
		printf("      bFirstInterface  \t    %d\n",associate.bFirstInterface);
		printf("      bInterfaceCount  \t    %d\n",associate.bInterfaceCount);
		printf("      bFunctionClass  \t    %d  %s\n",associate.bFunctionClass, associate.bFunctionClass==0x01?"AUDIO":"");
		printf("      bFunctionSubClass      %d\n",associate.bFunctionSubClass);
		printf("      bFunctionProtocol      %d\n",associate.bFunctionProtocol);
		printf("      iFunction  \t    %d\n",associate.iFunction);
		
		nb_ifaces = conf_desc->bNumInterfaces;
		if (nb_ifaces > 0)
			first_iface = conf_desc->usb_interface[0].altsetting[0].bInterfaceNumber;

		if(m == 0)
		{
			nb_ifaces_first_config = nb_ifaces;
		}
			
		for (i=0; i<nb_ifaces; i++) 
		{
			//printf("    interface[%d]: id = %d\n", i, conf_desc->usb_interface[i].altsetting[0].bInterfaceNumber);
			for (j=0; j<conf_desc->usb_interface[i].num_altsetting; j++) 
			{
				printf("    Interface Descriptor[%d]:\n",conf_desc->usb_interface[i].altsetting[0].bInterfaceNumber);
				printf("      bLength  \t\t    %d\n",conf_desc->usb_interface[i].altsetting[j].bLength);
				printf("      bDescriptorType  \t    %d\n",conf_desc->usb_interface[i].altsetting[j].bDescriptorType);
				printf("      bInterfaceNumber      %d\n",conf_desc->usb_interface[i].altsetting[j].bInterfaceNumber);
				printf("      bAlternateSetting      %d\n",conf_desc->usb_interface[i].altsetting[j].bAlternateSetting);
				printf("      bNumEndpoints  \t    %d\n",conf_desc->usb_interface[i].altsetting[j].bNumEndpoints);
				printf("      bInterfaceClass  \t    %d  %s\n",conf_desc->usb_interface[i].altsetting[j].bInterfaceClass, conf_desc->usb_interface[i].altsetting[j].bInterfaceClass==0x01?"AUDIO":"");
				if(conf_desc->usb_interface[i].altsetting[j].bInterfaceClass==0x01)
					printf("      bInterfaceSubClass      %d  %s\n",conf_desc->usb_interface[i].altsetting[j].bInterfaceSubClass,string_Audio_Interface_Subclass_Codes[conf_desc->usb_interface[i].altsetting[j].bInterfaceSubClass]);
				else
					printf("      bInterfaceSubClass      %d\n",conf_desc->usb_interface[i].altsetting[j].bInterfaceSubClass);
				printf("      bInterfaceProtocol      %d\n",conf_desc->usb_interface[i].altsetting[j].bInterfaceProtocol);
				printf("      iInterface  \t    %d\n",conf_desc->usb_interface[i].altsetting[j].iInterface);
				printf("      extra  \t\t    %p\n",conf_desc->usb_interface[i].altsetting[j].extra);
				printf("      extra_length  \t    %d\n",conf_desc->usb_interface[i].altsetting[j].extra_length);
				//uac_parse_AC_interface_desc(uac_audio_handle,conf_desc->usb_interface[i].altsetting[j]);
				if(conf_desc->usb_interface[i].altsetting[j].bInterfaceSubClass == UAC_INT_SUBCLASS_AUDIOCONTROL)
					uac_parse_AC_interface_desc(NULL,&conf_desc->usb_interface[i].altsetting[j]);
				else if(conf_desc->usb_interface[i].altsetting[j].bInterfaceSubClass == UAC_INT_SUBCLASS_AUDIOSTREAMING)
					uac_parse_AS_interface_desc(NULL,&conf_desc->usb_interface[i].altsetting[j]);
					

				if ( conf_desc->usb_interface[i].altsetting[j].bInterfaceClass == LIBUSB_CLASS_AUDIO ) 
				{
					// Mass storage devices that can use basic SCSI commands
					//test_mode = USE_SCSI;
					//printf("			This is Usb Audio device\n");
				}
				for (k=0; k<conf_desc->usb_interface[i].altsetting[j].bNumEndpoints; k++) 
				{
					endpoint = &conf_desc->usb_interface[i].altsetting[j].endpoint[k];
					printf("      Endpoint Descriptor:\n");
					printf("        bLength  \t\t    %d\n",endpoint->bLength);
					printf("        bDescriptorType  \t    %d\n",endpoint->bDescriptorType);
					printf("        bEndpointAddress  \t    0x%02x  EP  %d  %s\n",endpoint->bEndpointAddress, endpoint->bEndpointAddress&0x0F , (endpoint->bEndpointAddress&LIBUSB_ENDPOINT_IN)?"IN":"OUT"  );
					printf("        bmAttributes  \t\t    %d\n",endpoint->bmAttributes);
					printf("          Transfer Type  \t    %s\n",string_usb_Standard_Endpoint_Descriptor_Transfer_Type[endpoint->bmAttributes&0x03]);
					printf("          Synch Type  \t\t    %s\n",string_usb_Standard_Endpoint_Descriptor_Synchronization_Type[(endpoint->bmAttributes>>2)&0x03]);
					printf("          Usage Type  \t\t    %s\n",string_usb_Standard_Endpoint_Descriptor_Usage_Type[(endpoint->bmAttributes>>4)&0x03]);
					printf("        wMaxPacketSize  \t    %d\n",endpoint->wMaxPacketSize);
					printf("        bInterval  \t\t    %d\n",endpoint->bInterval);
					printf("        bRefresh  \t\t    %d\n",endpoint->bRefresh);
					printf("        bSynchAddress  \t\t    %d\n",endpoint->bSynchAddress);
					printf("        extra  \t\t\t    %p\n",endpoint->extra);
					printf("        extra_length  \t\t    %d\n",endpoint->extra_length);
					if(k == 0)
					{
						ep_fmt->datainterval = endpoint->bInterval - 1;
						ep_fmt->maxpacksize = endpoint->wMaxPacketSize;
						ep_fmt->fill_max = !!(endpoint->bmAttributes & 0x80);
					}
					if((endpoint->bmAttributes>>4)&0x03 == 0x01)
					{
						sync_ep->syncinterval = endpoint->bInterval - 1;
					}
					if(endpoint->extra)
					{
						uac_parse_AS_Endpoint_desc(NULL,endpoint);
					}
					

					//printf("       		endpoint[%d].address: %02X\n", k, endpoint->bEndpointAddress);
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
	}
	
	#if (AUTO_DETACH_KERNEL==0)
        if(libusb_kernel_driver_active(uac_audio_handle, 0) == 1) { //find out if kernel driver is attached  
            printf("Kernel Driver Active\n");  
            if(libusb_detach_kernel_driver(uac_audio_handle, 0) == 0) //detach it  
                printf("Kernel Driver Detached!\n");    
        }  
        if(libusb_kernel_driver_active(uac_audio_handle, 1) == 1) { //find out if kernel driver is attached  
            printf("Kernel Driver Active\n");  
            if(libusb_detach_kernel_driver(uac_audio_handle, 1) == 0) //detach it  
                printf("Kernel Driver Detached!\n");    
        }  
        if(libusb_kernel_driver_active(uac_audio_handle, 2) == 1) { //find out if kernel driver is attached  
            printf("Kernel Driver Active\n");  
            if(libusb_detach_kernel_driver(uac_audio_handle, 2) == 0) //detach it  
                printf("Kernel Driver Detached!\n");    
        }  
	#else
	libusb_set_auto_detach_kernel_driver(uac_audio_handle, 1);
	#endif


	for (iface = 0; iface < nb_ifaces_first_config; iface++)
	{
		printf("\nClaiming interface %d...\n", iface);
		r = libusb_claim_interface(uac_audio_handle, iface);
		if (r != LIBUSB_SUCCESS) {
			printf("   Failed.\n");
		}
	}

	printf("\nReading string descriptors:\n");
	for (i=0; i<20; i++) {
//		if (string_index[i] == 0) {
//			continue;
//		}
		if (libusb_get_string_descriptor_ascii(uac_audio_handle, i /*string_index[i]*/, (unsigned char*)string, 128) >= 0) {
			printf("   String (0x%02X): \"%s\"\n", i /*string_index[i]*/, string);
		}
	}


	
	return rc;
}

static void sigroutine(int signo){
    switch (signo){
        case SIGALRM:
//            printf("Catch a signal -- SIGALRM\n");
	gettimeofday( &start, NULL );
//	printf("start : %d.%d[%d]\n", (int)start.tv_sec, (int)start.tv_usec,timeId);

	uac_iso_transfer_feedback(fbbuffer,4);

            break;
        case SIGVTALRM:
            printf("Catch a signal -- SIGVTALRM\n");
            signal(SIGVTALRM, sigroutine);
            break;
    }
    return;
}

/*
 * snd_usb_endpoint is a model that abstracts everything related to an
 * USB endpoint and its streaming.
 *
 * There are functions to activate and deactivate the streaming URBs and
 * optional callbacks to let the pcm logic handle the actual content of the
 * packets for playback and record. Thus, the bus streaming and the audio
 * handlers are fully decoupled.
 *
 * There are two different types of endpoints in audio applications.
 *
 * SND_USB_ENDPOINT_TYPE_DATA handles full audio data payload for both
 * inbound and outbound traffic.
 *
 * SND_USB_ENDPOINT_TYPE_SYNC endpoints are for inbound traffic only and
 * expect the payload to carry Q10.14 / Q16.16 formatted sync information
 * (3 or 4 bytes).
 *
 * Each endpoint has to be configured prior to being used by calling
 * snd_usb_endpoint_set_params().
 *
 * The model incorporates a reference counting, so that multiple users
 * can call snd_usb_endpoint_start() and snd_usb_endpoint_stop(), and
 * only the first user will effectively start the URBs, and only the last
 * one to stop it will tear the URBs down again.
 */

/*
 * convert a sampling rate into our full speed format (fs/1000 in Q16.16)
 * this will overflow at approx 524 kHz
 */
static inline unsigned get_usb_full_speed_rate(unsigned int rate)
{
	return ((rate << 13) + 62) / 125;
}

/*
 * convert a sampling rate into USB high speed format (fs/8000 in Q16.16)
 * this will overflow at approx 4 MHz
 */
static inline unsigned get_usb_high_speed_rate(unsigned int rate)
{
	return ((rate << 10) + 62) / 125;
}

/*
 * For streaming based on information derived from sync endpoints,
 * prepare_outbound_urb_sizes() will call next_packet_size() to
 * determine the number of samples to be sent in the next packet.
 *
 * For implicit feedback, next_packet_size() is unused.
 */
int uac_usb_endpoint_next_packet_size(EP_FMT *ep)
{
	unsigned long flags;
	int ret;

	if (ep->fill_max)
		return ep->maxframesize;

	pthread_mutex_lock(&ep->lock);
	ep->phase = (ep->phase & 0xffff)
		+ (ep->freqm << ep->datainterval);
	ret = MIN(ep->phase >> 16, ep->maxframesize);
	pthread_mutex_unlock(&ep->lock);

	return ret;
}

int uac_set_params(libusb_device * dev,int rate ,int frame_bits/*32*2*/,unsigned int period_bytes,unsigned int frames_per_period,unsigned int periods_per_buffer)
{
	unsigned int maxsize, minsize, packs_per_ms, max_packs_per_urb;
	unsigned int max_packs_per_period, urbs_per_period, urb_packs;
	unsigned int max_urbs, i;
	int speed = 3/*libusb_get_device_speed(dev)*/;

	ep_fmt->maxframesize = ep_fmt->maxpacksize * 8 / frame_bits;

	ep_fmt->freqshift = 0;
	ep_fmt->datainterval = 0;
	printf("ep_fmt->datainterval : %d[0x%08x]\n",ep_fmt->datainterval,ep_fmt->datainterval);
	
	printf("speed = %s\n",speed == 3?"high":"full");
	if (speed == 2)
		ep_fmt->freqn = get_usb_full_speed_rate(rate);
	else
		ep_fmt->freqn = get_usb_high_speed_rate(rate);
	/* calculate the frequency in 16.16 format */
	ep_fmt->freqm = ep_fmt->freqn;
	//ep_fmt->freqshift = INT_MIN;
	ep_fmt->freqshift = 0;
	printf("ep_fmt->freqn : %d[0x%08x]\n",ep_fmt->freqn,ep_fmt->freqn);
	printf("ep_fmt->freqm : %d[0x%08x]\n",ep_fmt->freqm,ep_fmt->freqm);
	printf("ep_fmt->freqshift : %d[0x%08x]\n",ep_fmt->freqshift,ep_fmt->freqshift);

	ep_fmt->stride = 8; //32b*2/8
	ep_fmt->silence_value = 0;
	printf("ep_fmt->stride : %d\n",ep_fmt->stride);
	printf("ep_fmt->silence_value : %d\n",ep_fmt->silence_value);
	
	/* assume max. frequency is 25% higher than nominal */
	ep_fmt->freqmax = ep_fmt->freqn + (ep_fmt->freqn >> 2);
	printf("ep_fmt->freqmax : [0x%08x]\n",ep_fmt->freqmax);

	/* Round up freqmax to nearest integer in order to calculate maximum
	 * packet size, which must represent a whole number of frames.
	 * This is accomplished by adding 0x0.ffff before converting the
	 * Q16.16 format into integer.
	 * In order to accurately calculate the maximum packet size when
	 * the data interval is more than 1 (i.e. ep->datainterval > 0),
	 * multiply by the data interval prior to rounding. For instance,
	 * a freqmax of 41 kHz will result in a max packet size of 6 (5.125)
	 * frames with a data interval of 1, but 11 (10.25) frames with a
	 * data interval of 2.
	 * (ep->freqmax << ep->datainterval overflows at 8.192 MHz for the
	 * maximum datainterval value of 3, at USB full speed, higher for
	 * USB high speed, noting that ep->freqmax is in units of
	 * frames per packet in Q16.16 format.)
	 */
	maxsize = (((ep_fmt->freqmax << ep_fmt->datainterval) + 0xffff) >> 16) *
			 (frame_bits >> 3);
	printf("maxsize : [%d]\n",maxsize);
	/* but wMaxPacketSize might reduce this */
	if (ep_fmt->maxpacksize && ep_fmt->maxpacksize < maxsize) {
		/* whatever fits into a max. size packet */
		unsigned int data_maxsize = maxsize = ep_fmt->maxpacksize;

		ep_fmt->freqmax = (data_maxsize / (frame_bits >> 3))
				<< (16 - ep_fmt->datainterval);
	}

	ep_fmt->curpacksize = maxsize;

	if (speed != 2) {
		packs_per_ms = 8 >> ep_fmt->datainterval;
		max_packs_per_urb = MAX_PACKS_HS;
	} else {
		packs_per_ms = 1;
		max_packs_per_urb = MAX_PACKS;
	}
	printf("packs_per_ms : %d\n",packs_per_ms);
	printf("max_packs_per_urb : %d\n",max_packs_per_urb);

	printf("sync_ep : %p\n",sync_ep);
	printf("snd_usb_endpoint_implicit_feedback_sink : %d\n",0);
	if (sync_ep /*&& !snd_usb_endpoint_implicit_feedback_sink(ep)*/)
	{
		sync_ep->syncinterval = 3;
		max_packs_per_urb = MIN(max_packs_per_urb,
					1U << sync_ep->syncinterval);
		printf("sync_ep->syncinterval : %d\n",sync_ep->syncinterval);
	}
	printf("max_packs_per_urb : %d\n",max_packs_per_urb);
	max_packs_per_urb = MAX(1u, max_packs_per_urb >> ep_fmt->datainterval);
	printf("max_packs_per_urb : %d\n",max_packs_per_urb);

	//just support playback
	/* determine how small a packet can be */
	minsize = (ep_fmt->freqn >> (16 - ep_fmt->datainterval)) *
			(frame_bits >> 3);
	printf("minsize : %d\n",minsize);
	/* with sync from device, assume it can be 12% lower */
	if (sync_ep)
		minsize -= minsize >> 3;
	printf("minsize : %d\n",minsize);
	minsize = MAX(minsize, 1u);
	printf("minsize : %d\n",minsize);

	printf("period_bytes,minsize : [%d ,  %d]\n",period_bytes,minsize);
	/* how many packets will contain an entire ALSA period? */
	max_packs_per_period = DIV_ROUND_UP(period_bytes, minsize);
	printf("period_bytes,minsize,max_packs_per_period : [%d ,  %d] [%d]\n",period_bytes,minsize,max_packs_per_period);

	/* how many URBs will contain a period? */
	urbs_per_period = DIV_ROUND_UP(max_packs_per_period,
			max_packs_per_urb);
	printf("max_packs_per_period,max_packs_per_urb,urbs_per_period : [%d ,  %d] [%d]\n",max_packs_per_period,max_packs_per_urb,urbs_per_period);
	/* how many packets are needed in each URB? */
	urb_packs = DIV_ROUND_UP(max_packs_per_period, urbs_per_period);
	printf("max_packs_per_period, urbs_per_period,urb_packs : [%d ,  %d] [%d]\n",max_packs_per_period, urbs_per_period,urb_packs);

	/* limit the number of frames in a single URB */
	ep_fmt->max_urb_frames = DIV_ROUND_UP(frames_per_period,
				urbs_per_period);
	printf("frames_per_period, urbs_per_period,ep->max_urb_frames : [%d ,  %d] [%d]\n",frames_per_period, urbs_per_period,ep_fmt->max_urb_frames);

	/* try to use enough URBs to contain an entire ALSA buffer */
	max_urbs = MIN((unsigned) MAX_URBS,
			MAX_QUEUE * packs_per_ms / urb_packs);
	ep_fmt->nurbs = MIN(max_urbs, urbs_per_period * periods_per_buffer);
	printf("max_urbs,ep->nurbs : [%d , %d]\n",max_urbs,ep_fmt->nurbs);

	ep_fmt->urb_packets = urb_packs;
	#if 0
	//we set packets=ep_fmt->nurbs*urb_packs and then set ep_fmt->nurbs = 1 .
	ep_fmt->urb_packets = ep_fmt->nurbs * urb_packs*8;
//	ep_fmt->urb_packets = ep_fmt->nurbs * urb_packs*2;
	ep_fmt->nurbs = 1;
	#endif
	printf("ep_fmt->urb_packets : [%d]\n",ep_fmt->urb_packets);


	return 0;
}

static pthread_t uac_thread;
static pthread_t uac_event_thread;
static int uac_exit = 0;
static void *uac_thread_main(void *arg)
{
	int r = 0;
	int i = 0;
	printf("poll thread running\n");
	printf("%s\n",__TIME__);

	signal(SIGALRM, sigroutine);
	signal(SIGVTALRM, sigroutine);

	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = 1000*1;	// 10ms
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 1000*1;
//	setitimer(ITIMER_REAL, &value, &ovalue);    //(2)


	while (1) {
		struct timeval tv = { 1, 0 };
		#if 0
		r = libusb_handle_events_timeout(NULL, &tv);
		if (r < 0) {
			request_exit(2);
			break;
		}
		#endif
		sleep(1);

		if(access("/storage/emulated/0/Music/vu", R_OK) == 0)
		{
			system("rm /storage/emulated/0/Music/vu");
			printf("s_volume : 0x%04x\n",s_volume);
			if(s_volume<0xFF00)
			{
				s_volume += 0x1000;
				uac_request_set_device( UAC_SET_DEVICE_VOLUME, s_volume);
			}
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/vd", R_OK) == 0)
		{
			system("rm /storage/emulated/0/Music/vd");
			printf("s_volume : 0x%04x\n",s_volume);
			if(s_volume>0x8100)
			{
				s_volume -= 0x1000;
				uac_request_set_device( UAC_SET_DEVICE_VOLUME, s_volume);
			}
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/mute", R_OK) == 0)
		{
			system("rm /storage/emulated/0/Music/mute");
			uac_request_set_device( UAC_SET_DEVICE_MUTE, 0x01);
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/unmute", R_OK) == 0)
		{
			system("rm /storage/emulated/0/Music/unmute");
			uac_request_set_device( UAC_SET_DEVICE_MUTE, 0x00);
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/441", R_OK) == 0)
		{
			system("rm /storage/emulated/0/Music/441");
			uac_request_set_device( UAC_SET_DEVICE_SAMPLERATE, 44100);
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/48", R_OK) == 0)
		{
			system("rm /storage/emulated/0/Music/48");
			uac_request_set_device( UAC_SET_DEVICE_SAMPLERATE, 48000);
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/192", R_OK) == 0)
		{
			system("rm /storage/emulated/0/Music/192");
			uac_request_set_device( UAC_SET_DEVICE_SAMPLERATE, 192000);
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/play", R_OK) == 0)
		{
			int fd,size;
//			unsigned char buffer[176400]={0}; //44100*2*2
//			unsigned char buffer2[352800]={0}; //44100*2*2
			system("rm /storage/emulated/0/Music/play");

			printf("play\n");

			//fd=open("/mnt/hgfs/vmhgfs/usb/track1.pcm",O_RDONLY);
			fd=open("/storage/emulated/0/Music/11-16.pcm",O_RDONLY);

			readfile:
			size=read(fd,buffer,49392);	// 49392
			stsize = size;
			if(size > 0)
			{
//				for(i=0;i<size/8;i++)
//				{
//					printf("%02x %02x %02x %02x %02x %02x %02x %02x \n",buffer[i*8+0],buffer[i*8+1],buffer[i*8+2],buffer[i*8+3],buffer[i*8+4],buffer[i*8+5],buffer[i*8+6],buffer[i*8+7]);
//				}
				for(i=0;i<size/4;i++)
				{
					buffer2[i*8+0] = 0;
					buffer2[i*8+1] = 0;
					buffer2[i*8+2] = buffer[i*4+0];
					buffer2[i*8+3] = buffer[i*4+1];
					
					buffer2[i*8+4] = 0;
					buffer2[i*8+5] = 0;
					buffer2[i*8+6] = buffer[i*4+2];
					buffer2[i*8+7] = buffer[i*4+3];
//					printf("%02x %02x %02x %02x %02x %02x %02x %02x \n",buffer2[i*8+0],buffer2[i*8+1],buffer2[i*8+2],buffer2[i*8+3],buffer2[i*8+4],buffer2[i*8+5],buffer2[i*8+6],buffer2[i*8+7]);
				}
//				printf("size:%d\n",size);
				posbuff = 0;
//				ssize = 440*1;
				ssize = 3528/4;
				ssize = 440*10;
				posbuff += ssize;

				value.it_value.tv_sec = 0;
				value.it_value.tv_usec = 1000*3;	// 10ms
				value.it_interval.tv_sec = 0;
				value.it_interval.tv_usec = 1000*3;
//				setitimer(ITIMER_REAL, &value, &ovalue);    //(2)
				uac_iso_transfer_feedback(fbbuffer,4);

				uac_iso_transfer(buffer2,  ssize);
				//usleep(1000*500);
//				while(xfr_complete == 0)
//				{
//					usleep(DELAY_XFER);
//				}
//				xfr_complete = 0;
//				goto readfile;
			}
			else
			{
				printf("size:?end\n",size);
			}
			close(fd);
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/play1", R_OK) == 0)
		{
			int fd,size;
//			unsigned char buffer[176400]={0}; //44100*2*2
//			unsigned char buffer2[352800]={0}; //44100*2*2
			system("rm /storage/emulated/0/Music/play1");

			printf("play1\n");

			fd=open("/storage/emulated/0/Music/track5.pcm",O_RDONLY);
			//fd=open("/mnt/hgfs/vmhgfs/usb/11-16.pcm",O_RDONLY);

			readfile2:
			size=read(fd,buffer,1024*1024*10);	// 49392
			stsize  = size;
			if(size > 0)
			{
//				for(i=0;i<size/8;i++)
//				{
//					printf("%02x %02x %02x %02x %02x %02x %02x %02x \n",buffer[i*8+0],buffer[i*8+1],buffer[i*8+2],buffer[i*8+3],buffer[i*8+4],buffer[i*8+5],buffer[i*8+6],buffer[i*8+7]);
//				}
				for(i=0;i<size/4;i++)
				{
					buffer2[i*8+0] = 0;
					buffer2[i*8+1] = 0;
					buffer2[i*8+2] = buffer[i*4+0];
					buffer2[i*8+3] = buffer[i*4+1];
					
					buffer2[i*8+4] = 0;
					buffer2[i*8+5] = 0;
					buffer2[i*8+6] = buffer[i*4+2];
					buffer2[i*8+7] = buffer[i*4+3];
//					printf("%02x %02x %02x %02x %02x %02x %02x %02x \n",buffer2[i*8+0],buffer2[i*8+1],buffer2[i*8+2],buffer2[i*8+3],buffer2[i*8+4],buffer2[i*8+5],buffer2[i*8+6],buffer2[i*8+7]);
				}
				posbuff = 0;
				ssize = 440*10;
				posbuff += ssize;

				value.it_value.tv_sec = 0;
				value.it_value.tv_usec = 1000*3;	// 10ms
				value.it_interval.tv_sec = 0;
				value.it_interval.tv_usec = 1000*3;
//				setitimer(ITIMER_REAL, &value, &ovalue);    //(2)
				uac_iso_transfer_feedback(fbbuffer,4);

				uac_iso_transfer(buffer2,  ssize);
				//usleep(1000*500);
//				while(xfr_complete == 0)
//				{
//					usleep(DELAY_XFER);
//				}
//				xfr_complete = 0;
//				goto readfile2;
			}
			else
			{
				printf("size:?end\n",size);
			}
			close(fd);
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/playf", R_OK) == 0)
		{
			int fd,size;
//			unsigned char buffer[176400]={0}; //44100*2*2
//			unsigned char buffer2[352800]={0}; //44100*2*2
			system("rm /storage/emulated/0/Music/playf");

			printf("playf\n");

//			fd=open("/storage/emulated/0/Music/track5.pcm",O_RDONLY);
			fd=open("/storage/emulated/0/Music/wf01.pcm",O_RDONLY);

			readfilef:
			size=read(fd,buffer,1024*1024*20);	// 49392
			stsize  = size;
			printf("stsize : %d\n",stsize);
			if(size > 0)
			{
//				for(i=0;i<size/8;i++)
//				{
//					printf("%02x %02x %02x %02x %02x %02x %02x %02x \n",buffer[i*8+0],buffer[i*8+1],buffer[i*8+2],buffer[i*8+3],buffer[i*8+4],buffer[i*8+5],buffer[i*8+6],buffer[i*8+7]);
//				}
				for(i=0;i<size/4;i++)
				{
					buffer2[i*8+0] = 0;
					buffer2[i*8+1] = 0;
					buffer2[i*8+2] = buffer[i*4+0];
					buffer2[i*8+3] = buffer[i*4+1];
					
					buffer2[i*8+4] = 0;
					buffer2[i*8+5] = 0;
					buffer2[i*8+6] = buffer[i*4+2];
					buffer2[i*8+7] = buffer[i*4+3];
//					printf("%02x %02x %02x %02x %02x %02x %02x %02x \n",buffer2[i*8+0],buffer2[i*8+1],buffer2[i*8+2],buffer2[i*8+3],buffer2[i*8+4],buffer2[i*8+5],buffer2[i*8+6],buffer2[i*8+7]);
				}
				ssize = 0;
				posbuff = 0;
				stsize = size*2;

				value.it_value.tv_sec = 0;
				value.it_value.tv_usec = 1000*3;	// 10ms
				value.it_interval.tv_sec = 0;
				value.it_interval.tv_usec = 1000*3;
				setitimer(ITIMER_REAL, &value, &ovalue);    //(2)
//				uac_iso_transfer_feedback(fbbuffer,4);


				uac_set_params(uac_audio_dev, 44100, 64, 1920, 240, 2);

				#if 1
				//we set packets=ep_fmt->nurbs*urb_packs and then set ep_fmt->nurbs = 1 .
//				ep_fmt->urb_packets = ep_fmt->nurbs * ep_fmt->urb_packets;
				ep_fmt->urb_packets = 32;
				ep_fmt->nurbs = 1;
				#endif
				printf("ep_fmt->urb_packets : [%d]\n",ep_fmt->urb_packets);

				uac_iso_init();
				system("date");
				uac_iso_transfer2(buffer2,  &ssize);
//				posbuff += ssize;
				//usleep(1000*500);
//				while(xfr_complete == 0)
//				{
//					usleep(DELAY_XFER);
//				}
//				xfr_complete = 0;
//				goto readfile2;
			}
			else
			{
				printf("size:?end\n",size);
			}
			close(fd);
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/play192", R_OK) == 0)
		{
			int fd,size;
//			unsigned char buffer[176400]={0}; //44100*2*2
//			unsigned char buffer2[352800]={0}; //44100*2*2
			system("rm /storage/emulated/0/Music/play192");

			printf("play192\n");

//			fd=open("/storage/emulated/0/Music/test_192000_32bit.wav",O_RDONLY);
			fd=open("/storage/emulated/0/Music/192_32.wav",O_RDONLY);
			//fd=open("/mnt/hgfs/vmhgfs/usb/11-16.pcm",O_RDONLY);

			lseek(fd, 100, SEEK_CUR);
			size=read(fd,buffer2,1024*1024*40);	// 49392
			stsize  = size;
			if(size > 0)
			{
				posbuff = 0;

				value.it_value.tv_sec = 0;
				value.it_value.tv_usec = 1000*3;	// 10ms
				value.it_interval.tv_sec = 0;
				value.it_interval.tv_usec = 1000*3;
				setitimer(ITIMER_REAL, &value, &ovalue);    //(2)
//				uac_iso_transfer_feedback(fbbuffer,4);


				uac_set_params(uac_audio_dev, 192000, 64, 7680, 960, 2);

				#if 1
				//we set packets=ep_fmt->nurbs*urb_packs and then set ep_fmt->nurbs = 1 .
//				ep_fmt->urb_packets = ep_fmt->nurbs * ep_fmt->urb_packets;
				ep_fmt->urb_packets = 32;
				ep_fmt->nurbs = 8;
				#endif
				printf("ep_fmt->nurbs ep_fmt->urb_packets : [%d ,%d]\n",ep_fmt->nurbs, ep_fmt->urb_packets);

				uac_iso_init_urbs();
				system("date");
				ssize = start_iso_transfer( );
				posbuff += ssize;
			}
			else
			{
				printf("size:?end\n",size);
			}
			close(fd);
			//cdrom_exit = 1;
			//break;
		}
		if(access("/storage/emulated/0/Music/playu", R_OK) == 0)
		{
			int fd,size;
//			unsigned char buffer[176400]={0}; //44100*2*2
//			unsigned char buffer2[352800]={0}; //44100*2*2
			system("rm /storage/emulated/0/Music/playu");

			printf("playu\n");

			fd=open("/storage/emulated/0/Music/track5.pcm",O_RDONLY);
			//fd=open("/mnt/hgfs/vmhgfs/usb/11-16.pcm",O_RDONLY);

			readfileu:
			size=read(fd,buffer,1024*1024*10);	// 49392
			stsize  = size*2;
			printf("stsize : %d\n",stsize);
			if(size > 0)
			{
				for(i=0;i<size/4;i++)
				{
					buffer2[i*8+0] = 0;
					buffer2[i*8+1] = 0;
					buffer2[i*8+2] = buffer[i*4+0];
					buffer2[i*8+3] = buffer[i*4+1];
					
					buffer2[i*8+4] = 0;
					buffer2[i*8+5] = 0;
					buffer2[i*8+6] = buffer[i*4+2];
					buffer2[i*8+7] = buffer[i*4+3];
				}
				posbuff = 0;

				value.it_value.tv_sec = 0;
				value.it_value.tv_usec = 1000*3;	// 10ms
				value.it_interval.tv_sec = 0;
				value.it_interval.tv_usec = 1000*3;
				setitimer(ITIMER_REAL, &value, &ovalue);    //(2)
//				uac_iso_transfer_feedback(fbbuffer,4);


				uac_set_params(uac_audio_dev, 44100, 64, 1920, 240, 2);
				#if 1
				//we set packets=ep_fmt->nurbs*urb_packs and then set ep_fmt->nurbs = 1 .
//				ep_fmt->urb_packets = ep_fmt->nurbs * ep_fmt->urb_packets;
				ep_fmt->urb_packets = 32;
				ep_fmt->nurbs = 2;
				#endif
				printf("ep_fmt->nurbs ep_fmt->urb_packets : [%d ,%d]\n",ep_fmt->nurbs, ep_fmt->urb_packets);

				uac_iso_init_urbs();
				system("date");
				ssize = start_iso_transfer( );
				posbuff += ssize;
			}
		}
		if(access("/storage/emulated/0/Music/playu2", R_OK) == 0)
		{
			int fd,size;
//			unsigned char buffer[176400]={0}; //44100*2*2
//			unsigned char buffer2[352800]={0}; //44100*2*2
			system("rm /storage/emulated/0/Music/playu2");

			printf("playu2\n");

			fd=open("/storage/emulated/0/Music/wf01.pcm",O_RDONLY);
			//fd=open("/mnt/hgfs/vmhgfs/usb/11-16.pcm",O_RDONLY);

			size=read(fd,buffer,1024*1024*20);	// 49392
			stsize  = size*2;
			printf("stsize : %d\n",stsize);
			if(size > 0)
			{
				for(i=0;i<size/4;i++)
				{
					buffer2[i*8+0] = 0;
					buffer2[i*8+1] = 0;
					buffer2[i*8+2] = buffer[i*4+0];
					buffer2[i*8+3] = buffer[i*4+1];
					
					buffer2[i*8+4] = 0;
					buffer2[i*8+5] = 0;
					buffer2[i*8+6] = buffer[i*4+2];
					buffer2[i*8+7] = buffer[i*4+3];
				}
				posbuff = 0;

				value.it_value.tv_sec = 0;
				value.it_value.tv_usec = 1000*3;	// 10ms
				value.it_interval.tv_sec = 0;
				value.it_interval.tv_usec = 1000*3;
				setitimer(ITIMER_REAL, &value, &ovalue);    //(2)
//				uac_iso_transfer_feedback(fbbuffer,4);


				uac_set_params(uac_audio_dev, 44100, 64, 1920, 240, 2);
				#if 1
				//we set packets=ep_fmt->nurbs*urb_packs and then set ep_fmt->nurbs = 1 .
//				ep_fmt->urb_packets = ep_fmt->nurbs * ep_fmt->urb_packets;
				ep_fmt->urb_packets = 32;
				ep_fmt->nurbs = 2;
				#endif
				printf("ep_fmt->nurbs ep_fmt->urb_packets : [%d ,%d]\n",ep_fmt->nurbs, ep_fmt->urb_packets);

				uac_iso_init_urbs();
				system("date");
				ssize = start_iso_transfer( );
				posbuff += ssize;
			}
			else
			{
				printf("size:?end\n",size);
			}
			close(fd);
		}
		if(access("/storage/emulated/0/Music/exit", R_OK) == 0)
		{
			system("rm /storage/emulated/0/Music/exit");
			uac_exit = 1;
			break;
		}

	}

	printf("poll thread shutting down\n");
	return NULL;
}

void *_uac_handle_events(void *args)
{
    libusb_context *handle_ctx = (libusb_context *)args;
    printf("%s start handle_ctx: %p\n", __func__,handle_ctx);
    while(uac_exit == 0)
    {
        if (libusb_handle_events(handle_ctx) != LIBUSB_SUCCESS) {
            printf("libusb_handle_events err\n");
            break;
        }
//		usleep(20);
    }
    printf("libusb_handle_events exit\n");

	return NULL;
}

void uac_printf_tt(void)
{
	printf("FU_USBIN\t\t%d\n",FU_USBIN);
	printf("FU_USBOUT\t\t%d\n",FU_USBOUT);
	printf("ID_IT_USB\t\t%d\n",ID_IT_USB);
	printf("ID_IT_AUD\t\t%d\n",ID_IT_AUD);
	printf("ID_OT_USB\t\t%d\n",ID_OT_USB);
	printf("ID_OT_AUD\t\t%d\n",ID_OT_AUD);

	printf("\n");
	printf("ID_CLKSEL\t\t%d\n",ID_CLKSEL);
	printf("ID_CLKSRC_INT\t\t%d\n",ID_CLKSRC_INT);
	printf("ID_CLKSRC_SPDIF\t\t%d\n",ID_CLKSRC_SPDIF);
	printf("ID_CLKSRC_ADAT\t\t%d\n",ID_CLKSRC_ADAT);
	
	printf("\n");
	printf("ID_XU_MIXSEL\t\t%d\n",ID_XU_MIXSEL);
	printf("ID_XU_OUT\t\t%d\n",ID_XU_OUT);
	printf("ID_XU_IN\t\t%d\n",ID_XU_IN);
}
void main(int argc, char *argv[])
{
	int r;
	int iface, nb_ifaces, first_iface = -1;

	ep_fmt = malloc(sizeof(EP_FMT));
	if(!ep_fmt)
		return ;
		

	sync_ep = malloc(sizeof(EP_FMT));
	if(!sync_ep)
		return ;


	if(argc >= 3)
		uac_audio_open(strtol(argv[1],NULL, 0), strtol(argv[2],NULL, 0));
	else
		uac_audio_open(0x20b1, 0x000a);

	libusb_set_configuration(uac_audio_handle, 1);
	libusb_set_interface_alt_setting(uac_audio_handle,1,1);
	libusb_set_interface_alt_setting(uac_audio_handle,2,1);

	uac_printf_tt();

	if(pthread_mutex_init(&ep_fmt->lock, NULL) < 0)  
		printf("pthread_mutex_init  ep_fmt->lock error\n"); 

	if(pthread_mutex_init(&ep_fmt->lock2urbs, NULL) < 0)  
		printf("pthread_mutex_init  ep_fmt->lock2urbs error\n"); 
	
	if(pthread_mutex_init(&sync_ep->lock, NULL) < 0)  
		printf("pthread_mutex_init  sync_ep->lock error\n"); 

	r = pthread_create(&uac_thread, NULL, uac_thread_main, NULL);
	if (r)
	{
		goto out_deinit;
	}

	r = pthread_create(&uac_event_thread, NULL, _uac_handle_events, NULL);
	if (r)
	{
		goto out_deinit;
	}

	
	pthread_join(uac_thread, NULL);
	pthread_join(uac_event_thread, NULL);


	while(uac_exit == 0)
	{
		sleep(1);
	}

out_deinit:
	for (iface = 0; iface<nb_ifaces_first_config; iface++) 
	{
		printf("Releasing interface %d...\n", iface);
		libusb_release_interface(uac_audio_handle, iface);
	}

	#if (AUTO_DETACH_KERNEL==0)
	libusb_attach_kernel_driver(uac_audio_handle, 0);  
	libusb_attach_kernel_driver(uac_audio_handle, 1);  
	libusb_attach_kernel_driver(uac_audio_handle, 2);  
	#endif

	printf("Closing device...\n");
	libusb_close(uac_audio_handle);

	libusb_exit(NULL);

}
