//-*-c++-*------------------------------------------------------------
// NAME: Message Tags used in IO service to send and receive functions
// AUTH: Qi Yang
// FILE: MessageTags.h
// DATE: Mon Nov  6 07:10:08 1995
//--------------------------------------------------------------------

#ifndef MESSAGETAGS_HEADER
#define MESSAGETAGS_HEADER

const double NO_MSG_WAITING_TIME = 0.1;
const unsigned int MSG_TAG_MC	 = 0;

const unsigned int MSG_TAG_MITSIM= 1;

const unsigned int MSG_TAG_TMS	 = 4;
const unsigned int MSG_TAG_MESO	 = 5;
const unsigned int MSG_TAG_OD	 = 6;
const unsigned int MSG_TAG_MDI	 = 15;
const unsigned int MSG_TAG_MOE	 = 16;
const unsigned int MSG_TAG_TMCA	 = 17;
const unsigned int MSG_TAG_GUI	 = 32;

const unsigned int MSG_TAG_ZOOM_MITSIM	 = 2;
const unsigned int MSG_TAG_ZOOM_MEZZO = 3;
//const unsigned int MSG_TAG_ZOOM	 = 42;

// When a message is received from another process, it first check the
// message type from the message header, which is packed in the left
// most 2 hexadecimal digits.  If the message type is non-zero, it
// interpret the remaining digits (usually the id/index of a
// sensors/signals) of the message header and following data based on
// the message type.  If the message type is zero, it is interpreted
// as one of the following messages define here by MSG_DATA_CODE.

const unsigned int MSG_TYPE             = 0xFF000000;

const unsigned int MSG_TYPE_SENSOR_A    = 0x01000000;
const unsigned int MSG_TYPE_SENSOR_I    = 0x02000000;
const unsigned int MSG_TYPE_SENSOR_S    = 0x03000000;
const unsigned int MSG_TYPE_INCI        = 0x04000000; // enumeration
const unsigned int MSG_TYPE_INCI_CALLINS= 0x05000000; // enumeration

const unsigned int MSG_TYPE_SIGNAL      = 0x01000000;

const unsigned int MSG_DATA_CODE        = 0x00FFFFFF; // mask
const unsigned int MSG_PROCESS_IDS      = 0x00000000; // enumeration
const unsigned int MSG_TIMING_DATA	    = 0x00000001; // enumeration
const unsigned int MSG_END_CYCLE	    = 0x00000002; // enumeration
const unsigned int MSG_CURRENT_TIME	    = 0x00000003; // enumeration
const unsigned int MSG_PAUSE	    = 0x00000004; // enumeration
const unsigned int MSG_RESUME	    = 0x00000005; // enumeration
const unsigned int MSG_SENSOR_RESET    = 0x00000006; // enumeration
const unsigned int MSG_SENSOR_READY	    = 0x00000007; // enumeration
					    					        
const unsigned int MSG_MESO_START	= 0x0000000A; // enumeration
const unsigned int MSG_MESO_DONE	= 0x0000000B; // enumeration
const unsigned int MSG_STATE_START	= 0x0000000C; // enumeration
const unsigned int MSG_STATE_DONE	= 0x0000000D; // enumeration
const unsigned int MSG_MDI_START	= 0x0000000E; // enumeration
const unsigned int MSG_MDI_DONE	= 0x0000000F; // enumeration
const unsigned int MSG_DUMP_SURVEILLANCE = 0x00000100; // enumeration
const unsigned int MSG_NEW_SURVEILLANCE_READY  = 0x00000200; // enumeration
				        				        
const unsigned int MSG_QUIT		= 0x00000010; // enumeration
const unsigned int MSG_DONE		= 0x00000020; // enumeration
					    					        
const unsigned int MSG_WINDOW_SHOW	= 0x00000030; // enumeration
const unsigned int MSG_WINDOW_HIDE	= 0x00000040; // enumeration
					    					        
const unsigned int MSG_END_FLAG		= 0x000000FF; // enumeration

// Running mode

const unsigned int MODE_HOLD		= 0x0001;
const unsigned int MODE_DEMO		= 0x0002;
const unsigned int MODE_DEMO_PAUSE	= 0x0004;


// Some useful constants

enum { WARNING_ERROR, FATAL_ERROR };
enum { WAIT_MSG = -1, DO_NOT_WAIT_MSG = 0 }; 
enum { SEND_BY_PACKAGE = 0, SEND_END = 1, SEND_IMMEDIATELY = 2 };

//extern double NO_MSG_WAITING_TIME;
//extern double DEFAULT_NO_MSG_WAITING_TIME;

// These tell MITSIM and MesoTS what type of routing should be used

const int INFO_FLAG_DYNAMIC     = 0x0001; // time variant
const int INFO_FLAG_UPDATE      = 0x0006; // sum
const int INFO_FLAG_UPDATE_TREES= 0x0002; // calculate shortest path trees
const int INFO_FLAG_UPDATE_PATHS= 0x0004; // prespecified path only
const int INFO_FLAG_USE_EXISTING_TABLES = 0x0008;// do not update initial SP
const int INFO_FLAG_AVAILABILITY= 0x0100; // updated TT available at pretrip
const int INFO_FLAG_VMS_BASED   = 0x0200; // updated TT received at beacons

#endif
