/* Descriptor Types */

#define AEM_ENTITY_TYPE                 0x0000
#define AEM_CONFIGURATION_TYPE          0x0001
#define AEM_AUDIO_UNIT_TYPE             0x0002
#define AEM_VIDEO_UNIT_TYPE             0x0003
#define AEM_SENDOR_UNIT_TYPE            0x0004
#define AEM_STREAM_INPUT_TYPE           0x0005
#define AEM_STREAM_OUTPUT_TYPE          0x0006
#define AEM_JACK_INPUT_TYPE             0x0007
#define AEM_JACK_OUTPUT_TYPE            0x0008
#define AEM_AUDIO_PORT_INPUT_TYPE       0x0009
#define AEM_AUDIO_PORT_OUTPUT_TYPE      0x000a
#define AEM_VIDEO_PORT_INPUT_TYPE       0x000b
#define AEM_VIDEO_PORT_OUTPUT_TYPE      0x000c
#define AEM_EXTERNAL_PORT_INPUT_TYPE    0x000d
#define AEM_EXTERNAL_PORT_OUTPUT_TYPE   0x000e
#define AEM_SENSOR_PORT_INPUT_TYPE      0x000f
#define AEM_SENSOR_PORT_OUTPUT_TYPE     0x0010
#define AEM_INTERNAL_PORT_INPUT_TYPE    0x0011
#define AEM_INTERNAL_PORT_OUTPUT_TYPE   0x0012
#define AEM_AVB_INTERFACE_TYPE          0x0013
#define AEM_CLOCK_SOURCE_TYPE           0x0014
#define AEM_AUDIO_MAP_TYPE              0x0015
#define AEM_AUDIO_CLUSTER_TYPE          0x0016
#define AEM_CONTROL_TYPE                0x0017
#define AEM_SIGNAL_SELECTOR_TYPE        0x0018
#define AEM_MIXER_TYPE                  0x0019
#define AEM_MATRIX_TYPE                 0x001a
#define AEM_LOCALE_TYPE                 0x001b
#define AEM_STRINGS_TYPE                0x001c
#define AEM_MATRIX_SIGNAL_TYPE          0x001d
#define AEM_MEMORY_OBJECT_TYPE          0x001e
#define AEM_INVALID_TYPE                0xffff

/* 7.2.8.1 Port Flags */
#define AEM_AUDIO_PORT_FLAG_CLOCK_SYNC_SOURCE       0x00000001
#define AEM_AUDIO_PORT_FLAG_ASYNC_SAMPLE_RATE_CONV  0x00000002
#define AEM_AUDIO_PORT_FLAG_SYNC_SAMPLE_RATE_CONV   0x00000004

/*** Control Descriptors *******************************/

/* 7.2.17.1 control_value_type */
#define AEM_CONTROL_VALUE_TYPE_READ_ONLY_FLAG     0x4000
#define AEM_CONTROL_VALUE_TYPE_UNKNOWN_FLAG       0x2000

/* 7.2.17.1.3 value_type */
#define AEM_CONTROL_LINEAR_INT8         0x0000
#define AEM_CONTROL_LINEAR_UINT8        0x0001
#define AEM_CONTROL_LINEAR_INT16        0x0002
#define AEM_CONTROL_LINEAR_UINT16       0x0003
#define AEM_CONTROL_LINEAR_INT32        0x0004
#define AEM_CONTROL_LINEAR_UINT32       0x0005
#define AEM_CONTROL_LINEAR_INT64        0x0006
#define AEM_CONTROL_LINEAR_UINT64       0x0007
#define AEM_CONTROL_LINEAR_FLOAT        0x0008
#define AEM_CONTROL_LINEAR_DOUBLE       0x0009
#define AEM_CONTROL_SELECTOR_INT8       0x000a
#define AEM_CONTROL_SELECTOR_UINT8      0x000b
#define AEM_CONTROL_SELECTOR_INT16      0x000c
#define AEM_CONTROL_SELECTOR_UINT16     0x000d
#define AEM_CONTROL_SELECTOR_INT32      0x000e
#define AEM_CONTROL_SELECTOR_UINT32     0x000f
#define AEM_CONTROL_SELECTOR_INT64      0x0010
#define AEM_CONTROL_SELECTOR_UINT64     0x0011
#define AEM_CONTROL_SELECTOR_FLOAT      0x0012
#define AEM_CONTROL_SELECTOR_DOUBLE     0x0013
#define AEM_CONTROL_UTF8                0x0014
#define AEM_CONTROL_BODE_PLOT           0x0015
#define AEM_CONTROL_ARRAY_INT8          0x0016
#define AEM_CONTROL_ARRAY_UINT8         0x0017
#define AEM_CONTROL_ARRAY_INT16         0x0018
#define AEM_CONTROL_ARRAY_UINT16        0x0019
#define AEM_CONTROL_ARRAY_INT32         0x001a
#define AEM_CONTROL_ARRAY_UINT32        0x001b
#define AEM_CONTROL_ARRAY_INT64         0x001c
#define AEM_CONTROL_ARRAY_UINT64        0x001d
#define AEM_CONTROL_ARRAY_FLOAT         0x001e
#define AEM_CONTROL_ARRAY_DOUBLE        0x001f
#define AEM_CONTROL_SELECTOR_STRING     0x0020
#define AEM_CONTROL_SMPTE_TIME          0x0021
#define AEM_CONTROL_SAMPLE_RATE         0x0022
#define AEM_CONTROL_GPTP_TIME           0x0023
#define AEM_CONTROL_VENDOR              0x3fff

/* 7.3.4. Control Types */
#define AEM_CONTROL_TYPE_MUTE           0x90e0f00000010000
#define AEM_CONTROL_TYPE_VOLUME         0x90e0f00000010001

/* 7.3.3. Control Value Units */
#define AEM_CONTROL_UNITS_UNITLESS      0x0000

/*** Stream Descriptors *******************************/
#define AEM_STREAM_FLAGS_CLOCK_SYNC_SOURCE  0x00000001
#define AEM_STREAM_FLAGS_CLASS_A            0x00000002
#define AEM_STREAM_FLAGS_CLASS_B            0x00000004

/* 7.3.1. Stream Formats */
#define AEM_STREAM_FORMAT_VERSION_0           0x0000000000000000
#define AEM_STREAM_FORMAT_VERSION_1           0x4000000000000000 

#define AEM_STREAM_FORMAT_61883_IIDC_SUBTYPE    0x0000000000000000
#define AEM_STREAM_FORMAT_MMA_SUBTYPE           0
#define AEM_STREAM_FORMAT_EXPERIMENTAL_SUBTYPE  0

/* Subtype 0 Stream Format */
#define AEM_SUBTYPE_0_SF_IIDC                     0x0000000000000000
#define AEM_SUBTYPE_0_SF_61183                    0x0040000000000000

/*** Jack Descriptors *********************************/

/* Flags */
#define AEM_JACK_FLAGS_CLOCK_SYNC_SOURCE        0x00000001
#define AEM_JACK_FLAGS_CAPTIVE                  0x00000002

/* Types */
#define AEM_JACK_TYPE_UNBALANCED_ANALOG         0x0007
#define AEM_JACK_TYPE_DIGITAL                   0x0009

/*** Clock Source Descriptors *********************************/

/* Types */

#define AEM_CLOCK_SOURCE_LOCAL_OSCILLATOR       0x0000
#define AEM_CLOCK_SOURCE_INPUT_STREAM           0x0001
#define AEM_CLOCK_SOURCE_8021_AS                0x0005

/*** Locale/String Descriptors *********************************/

#define AEM_NO_STRING                           0x0007