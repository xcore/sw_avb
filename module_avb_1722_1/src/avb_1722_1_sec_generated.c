// AVB 1722.1 SEC Parse Table
//
// This file was autogrenerated by the make_table.pl script.


unsigned int avb_1722_1_sec_parse_tree[] = {
  0x0040001b, // compare section/subsection to range (0x0, 0x0)
  0xb0700003, // compare item to range (0x30, 0x30)
  0xc0000000, // Time to actuate messages  (Function at 0x0)
  0xb0f00006, // compare item to range (0x70, 0x71)
  0x01000000, // Request identifier for request and related response (Data at 0x0)
  0x01000004, // Status response (Data at 0x4)
  0xb0d00008, // compare item to range (0x90, 0x90)
  0x02000008, // Report values of address patterns matching mask and (Data at 0x8)
  0xf880001b, // compare item to range (0x400, 0x411)
  0x01000010, // AVDECC Protocol Version (Data at 0x10)
  0x83000001, // Device's GUID (Function at 0x1)
  0x04000014, // Vendor's human readable name (Data at 0x14)
  0x05000019, // Vendor's OUI (Data at 0x19)
  0x86000002, // Boot identifier (Function at 0x2)
  0xc4000003, // user settable device name (Function at 0x3)
  0x0400001f, // Human readable product description (Data at 0x1f)
  0x06000031, // Model identifier (Data at 0x31)
  0x04000039, // Human readable device model name (Data at 0x39)
  0x01000043, // Enumerated device types (Data at 0x43)
  0x04000047, // Human readable device type (Data at 0x47)
  0xc1000004, // Device notification wink request (Function at 0x4)
  0x01000050, // Device notification signalled (Data at 0x50)
  0x87000005, // mac-address on this device for each network port (Function at 0x5)
  0x87000006, // mac-addr of current gPTP grand master ID for each network port (Function at 0x6)
  0x01000054, // Talker Capabilities (Data at 0x54)
  0x01000058, // Listener Capabilities (Data at 0x58)
  0x0100005c, // Controller Capabilities (Data at 0x5c)
  0x00450037, // compare section/subsection to range (0x5, 0x5)
  0x10410025, // compare subsubsection to range (0x1, 0x1)
  0xa041001f, // compare subaddress to range (0x1, 0x1)
  0x08000060, // Count of talker media sources (Data at 0x60)
  0xa0440021, // compare subaddress to range (0x4, 0x4)
  0x01000062, // Enumeration of media source type (Data at 0x62)
  0xa0460023, // compare subaddress to range (0x6, 0x6)
  0x01000066, // Media format item of this talker media source (Data at 0x66)
  0xe0480025, // compare subaddress to range (0x8, 0x8)
  0xc4000007, // The user settable name for this media source (Function at 0x7)
  0x1042002a, // compare subsubsection to range (0x2, 0x2)
  0xa0410028, // compare subaddress to range (0x1, 0x1)
  0x0800006a, // Count of the number of different talker media formats supported (Data at 0x6a)
  0xe045002a, // compare subaddress to range (0x5, 0x5)
  0x0300006c, // Media format properties (Data at 0x6c)
  0x10430032, // compare subsubsection to range (0x3, 0x3)
  0xa041002d, // compare subaddress to range (0x1, 0x1)
  0x08000074, // Count of the number of talker streams supported (Data at 0x74)
  0xa0850030, // compare subaddress to range (0x5, 0x6)
  0x04000076, // The stream properties of one talker stream (Data at 0x76)
  0x08000078, // The format item for one talker stream (Data at 0x78)
  0xe0480032, // compare subaddress to range (0x8, 0x8)
  0x84000008, // The user settable name for this talker stream (Function at 0x8)
  0x50440037, // compare subsubsection to range (0x4, 0x4)
  0xa0410035, // compare subaddress to range (0x1, 0x1)
  0x0800007a, // Count of the number of talker stream formats supported (Data at 0x7a)
  0xe0450037, // compare subaddress to range (0x5, 0x5)
  0x0300007c, // The properties of one talker stream format (Data at 0x7c)
  0x00460053, // compare section/subsection to range (0x6, 0x6)
  0x10410041, // compare subsubsection to range (0x1, 0x1)
  0xa041003b, // compare subaddress to range (0x1, 0x1)
  0x08000084, // Count of listener media sinks (Data at 0x84)
  0xa044003d, // compare subaddress to range (0x4, 0x4)
  0x01000086, // Enumeration of media sink type (Data at 0x86)
  0xa046003f, // compare subaddress to range (0x6, 0x6)
  0x0100008a, // Media format item of this listener media sink (Data at 0x8a)
  0xe0480041, // compare subaddress to range (0x8, 0x8)
  0x84000009, // The user settable name for this media sink (Function at 0x9)
  0x10420046, // compare subsubsection to range (0x2, 0x2)
  0xa0410044, // compare subaddress to range (0x1, 0x1)
  0x0800008e, // Count of the number of listener media formats supported (Data at 0x8e)
  0xe0450046, // compare subaddress to range (0x5, 0x5)
  0x03000090, // The properties of one listener media format (Data at 0x90)
  0x1043004e, // compare subsubsection to range (0x3, 0x3)
  0xa0410049, // compare subaddress to range (0x1, 0x1)
  0x08000098, // Count of the number of listener streams supported (Data at 0x98)
  0xa085004c, // compare subaddress to range (0x5, 0x6)
  0x0400009a, // The stream properties of one listener stream (Data at 0x9a)
  0x0800009c, // The format item of one listener stream (Data at 0x9c)
  0xe048004e, // compare subaddress to range (0x8, 0x8)
  0x8400000a, // The user settable name for this listener stream (Function at 0xa)
  0x50440053, // compare subsubsection to range (0x4, 0x4)
  0xa0410051, // compare subaddress to range (0x1, 0x1)
  0x0800009e, // Count of the number of talker stream formats supported (Data at 0x9e)
  0xe0450053, // compare subaddress to range (0x5, 0x5)
  0x030000a0, // The properties of one talker stream format (Data at 0xa0)
  0x00490057, // compare section/subsection to range (0x9, 0x9)
  0x50430057, // compare subsubsection to range (0x3, 0x3)
  0xe0470057, // compare subaddress to range (0x7, 0x7)
  0x010000a8, // Map of talker media sources to talker stream source (Data at 0xa8)
  0x004a005b, // compare section/subsection to range (0xa, 0xa)
  0x5043005b, // compare subsubsection to range (0x3, 0x3)
  0xe047005b, // compare subaddress to range (0x7, 0x7)
  0x010000ac, // Map of listener media sinks to listener stream sinks (Data at 0xac)
  0x004d0069, // compare section/subsection to range (0xd, 0xd)
  0x10410065, // compare subsubsection to range (0x1, 0x1)
  0xa04a005f, // compare subaddress to range (0xa, 0xa)
  0x0b0000b0, // Set media source level in db (Data at 0xb0)
  0xa04d0061, // compare subaddress to range (0xd, 0xd)
  0x090000b4, // Set media source mute (Data at 0xb4)
  0xe0d80065, // compare subaddress to range (0x18, 0x1a)
  0x0a0000b5, // Talker media source meter slot values (Data at 0xb5)
  0x010000b9, // Meter format code for each meter slot (Data at 0xb9)
  0x010000bd, // Mapping of talker media sources to meter slots (Data at 0xbd)
  0x50430069, // compare subsubsection to range (0x3, 0x3)
  0xe0980069, // compare subaddress to range (0x18, 0x19)
  0x0a0000c1, // Meter values for a specific talker stream (Data at 0xc1)
  0x010000c5, // Meter format code for each meter slot in a specific talker stream (Data at 0xc5)
  0x404e0077, // compare section/subsection to range (0xe, 0xe)
  0x10410073, // compare subsubsection to range (0x1, 0x1)
  0xa04a006d, // compare subaddress to range (0xa, 0xa)
  0x0b0000c9, // Set media sink level in db (Data at 0xc9)
  0xa04d006f, // compare subaddress to range (0xd, 0xd)
  0x090000cd, // Set media sink mute (Data at 0xcd)
  0xe0d80073, // compare subaddress to range (0x18, 0x1a)
  0x0a0000ce, // Talker media source meter slot values (Data at 0xce)
  0x010000d2, // Meter format code for each meter slot (Data at 0xd2)
  0x010000d6, // Mapping of listener media sinks to meter slots (Data at 0xd6)
  0x50430077, // compare subsubsection to range (0x3, 0x3)
  0xe0980077, // compare subaddress to range (0x18, 0x19)
  0x010000da, // Meter values for a specific listener stream (Data at 0xda)
  0x010000de, // Meter format code for each meter slot in a specific listener stream (Data at 0xde)
};

extern unsigned int avb_1722_1_getset_actuation_time(unsigned item_number, unsigned set, char *data);
extern unsigned int avb_1722_1_getset_device_guid(unsigned item_number, unsigned set, char *data);
extern unsigned int avb_1722_1_getset_boot_id(unsigned item_number, unsigned set, char *data);
extern unsigned int avb_1722_1_getset_device_name(unsigned item_number, unsigned set, char *data);
extern unsigned int avb_1722_1_getset_device_wink(unsigned item_number, unsigned set, char *data);
extern unsigned int avb_1722_1_getset_device_mac_addr(unsigned item_number, unsigned set, char *data);
extern unsigned int avb_1722_1_getset_device_ptp_gm(unsigned item_number, unsigned set, char *data);
extern unsigned int avb_1722_1_getset_source_name(unsigned item_number, unsigned set, char *data);
extern unsigned int avb_1722_1_getset_source_stream_name(unsigned item_number, unsigned set, char *data);
extern unsigned int avb_1722_1_getset_sink_name(unsigned item_number, unsigned set, char *data);
extern unsigned int avb_1722_1_getset_sink_stream_name(unsigned item_number, unsigned set, char *data);


unsigned avb_1722_1_sec_dispatch(unsigned func_num, unsigned item_number, unsigned set, char* data)
{
  switch (func_num) {
  case 0:
    return avb_1722_1_getset_actuation_time(item_number, set, data);
  case 1:
    return avb_1722_1_getset_device_guid(item_number, set, data);
  case 2:
    return avb_1722_1_getset_boot_id(item_number, set, data);
  case 3:
    return avb_1722_1_getset_device_name(item_number, set, data);
  case 4:
    return avb_1722_1_getset_device_wink(item_number, set, data);
  case 5:
    return avb_1722_1_getset_device_mac_addr(item_number, set, data);
  case 6:
    return avb_1722_1_getset_device_ptp_gm(item_number, set, data);
  case 7:
    return avb_1722_1_getset_source_name(item_number, set, data);
  case 8:
    return avb_1722_1_getset_source_stream_name(item_number, set, data);
  case 9:
    return avb_1722_1_getset_sink_name(item_number, set, data);
  case 10:
    return avb_1722_1_getset_sink_stream_name(item_number, set, data);
  }  return 0;
}

typedef enum {
  AVB_1722_1_SEC_DATA_TYPE_BOOL = 9,
  AVB_1722_1_SEC_DATA_TYPE_MACADDR = 7,
  AVB_1722_1_SEC_DATA_TYPE_GAINDB = 11,
  AVB_1722_1_SEC_DATA_TYPE_MASKCOMPARE = 2,
  AVB_1722_1_SEC_DATA_TYPE_STRING = 4,
  AVB_1722_1_SEC_DATA_TYPE_INT32 = 1,
  AVB_1722_1_SEC_DATA_TYPE_OUI = 5,
  AVB_1722_1_SEC_DATA_TYPE_EUI64 = 3,
  AVB_1722_1_SEC_DATA_TYPE_GTPTIME = 0,
  AVB_1722_1_SEC_DATA_TYPE_INT16 = 8,
  AVB_1722_1_SEC_DATA_TYPE_INT64 = 6,
  AVB_1722_1_SEC_DATA_TYPE_4METER = 10,
} avb_1722_1_sec_data_type_t;

unsigned char avb_1722_1_sec_constant_data[] = {
  0x00,0x00,0x00,0x00,	// Request identifier for request and related response "0"
  0x00,0x00,0x00,0x00,	// Status response "0"
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// Report values of address patterns matching mask and "0"
  0x00,0x00,0x00,0x00,	// AVDECC Protocol Version "0"
  0x58,0x4d,0x4f,0x53,0x00,	// Vendor's human readable name "XMOS"
  0x04,0x03,0x02,0x00,0x00,0x00,	// Vendor's OUI "0x020304"
  0x58,0x4d,0x4f,0x53,0x20,0x41,0x56,0x42,0x20,0x65,0x6e,0x64,0x70,0x6f,0x69,0x6e,0x74,0x00,	// Human readable product description "XMOS AVB endpoint"
  0xf0,0xde,0xbc,0x9a,0x78,0x56,0x34,0x12,	// Model identifier "0x123456789abcdef0"
  0x56,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x31,0x00,	// Human readable device model name "Version 1"
  0x00,0x00,0x00,0x00,	// Enumerated device types "0"
  0x45,0x6e,0x64,0x70,0x6f,0x69,0x6e,0x74,0x00,	// Human readable device type "Endpoint"
  0x00,0x00,0x00,0x00,	// Device notification signalled "0"
  0x01,0x00,0x00,0x00,	// Talker Capabilities "0x1"
  0x01,0x00,0x00,0x00,	// Listener Capabilities "0x1"
  0x00,0x00,0x00,0x00,	// Controller Capabilities "0x0"
  0x00,0x00,	// Count of talker media sources "0"
  0x00,0x00,0x00,0x00,	// Enumeration of media source type "0"
  0x00,0x00,0x00,0x00,	// Media format item of this talker media source "0"
  0x00,0x00,	// Count of the number of different talker media formats supported "0"
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// Media format properties "0"
  0x00,0x00,	// Count of the number of talker streams supported "0"
  0x30,0x00,	// The stream properties of one talker stream "0"
  0x00,0x00,	// The format item for one talker stream "0"
  0x00,0x00,	// Count of the number of talker stream formats supported "0"
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// The properties of one talker stream format "0"
  0x00,0x00,	// Count of listener media sinks "0"
  0x00,0x00,0x00,0x00,	// Enumeration of media sink type "0"
  0x00,0x00,0x00,0x00,	// Media format item of this listener media sink "0"
  0x00,0x00,	// Count of the number of listener media formats supported "0"
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// The properties of one listener media format "0"
  0x00,0x00,	// Count of the number of listener streams supported "0"
  0x30,0x00,	// The stream properties of one listener stream "0"
  0x00,0x00,	// The format item of one listener stream "0"
  0x00,0x00,	// Count of the number of talker stream formats supported "0"
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// The properties of one talker stream format "0"
  0x00,0x00,0x00,0x00,	// Map of talker media sources to talker stream source "0"
  0x00,0x00,0x00,0x00,	// Map of listener media sinks to listener stream sinks "0"
  0x00,0x00,0x00,0x00,	// Set media source level in db "0"
  0x00,	// Set media source mute "0"
  0x00,0x00,0x00,0x00,	// Talker media source meter slot values "0"
  0x00,0x00,0x00,0x00,	// Meter format code for each meter slot "0"
  0x00,0x00,0x00,0x00,	// Mapping of talker media sources to meter slots "0"
  0x00,0x00,0x00,0x00,	// Meter values for a specific talker stream "0"
  0x00,0x00,0x00,0x00,	// Meter format code for each meter slot in a specific talker stream "0"
  0x00,0x00,0x00,0x00,	// Set media sink level in db "0"
  0x00,	// Set media sink mute "0"
  0x00,0x00,0x00,0x00,	// Talker media source meter slot values "0"
  0x00,0x00,0x00,0x00,	// Meter format code for each meter slot "0"
  0x00,0x00,0x00,0x00,	// Mapping of listener media sinks to meter slots "0"
  0x00,0x00,0x00,0x00,	// Meter values for a specific listener stream "0"
  0x00,0x00,0x00,0x00,	// Meter format code for each meter slot in a specific listener stream "0"
};

unsigned int avb_1722_1_sec_data_type_length_t[] = {
  1, //bool
  6, //macaddr
  4, //gaindb
  8, //maskcompare
  0, //string
  4, //int32
  6, //oui
  8, //eui64
  8, //gtptime
  2, //int16
  8, //int64
  4, //4meter
};

