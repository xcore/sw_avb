#ifndef AVB_1722_1_ADPDU_H_
#define AVB_1722_1_ADPDU_H_

#include "avb_1722_1_protocol.h"
#include "avb_1722_1_default_conf.h"

#define AVB_1722_1_ADP_CD_LENGTH        56

#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_DFU_MODE                     (0x00000001)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_ADDRESS_ACCESS_SUPPORTED     (0x00000002)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_GATEWAY_ENTITY               (0x00000004)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_AEM_SUPPORTED                (0x00000008)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_LEGACY_AVC                   (0x00000010)
#define AVB_1722_1_ADP_ENTITY_CAPIBILITIES_ASSOCIATION_ID_SUPPORTED     (0x00000020)
#define AVB_1722_1_ADP_ENTITY_CAPIBILITIES_ASSOCIATION_ID_VALID         (0x00000040)
#define AVB_1722_1_ADP_ENTITY_CAPIBILITIES_VENDOR_UNIQUE_SUPPORTED      (0x00000080)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_CLASS_A_SUPPORTED            (0x00000100)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_CLASS_B_SUPPORTED            (0x00000200)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_AS_SUPPORTED                 (0x00000400)

#define AVB_1722_1_ADP_TALKER_CAPABILITIES_IMPLEMENTED                      (0x0001)
#define AVB_1722_1_ADP_TALKER_CAPABILITIES_OTHER_SOURCE                     (0x0200)
#define AVB_1722_1_ADP_TALKER_CAPABILITIES_CONTROL_SOURCE                   (0x0400)
#define AVB_1722_1_ADP_TALKER_CAPABILITIES_MEDIA_CLOCK_SOURCE               (0x0800)
#define AVB_1722_1_ADP_TALKER_CAPABILITIES_SMPTE_SOURCE                     (0x1000)
#define AVB_1722_1_ADP_TALKER_CAPABILITIES_MIDI_SOURCE                      (0x2000)
#define AVB_1722_1_ADP_TALKER_CAPABILITIES_AUDIO_SOURCE                     (0x4000)
#define AVB_1722_1_ADP_TALKER_CAPABILITIES_VIDEO_SOURCE                     (0x8000)

#define AVB_1722_1_ADP_LISTENER_CAPABILITIES_IMPLEMENTED                    (0x0001)
#define AVB_1722_1_ADP_LISTENER_CAPABILITIES_OTHER_SINK                     (0x0200)
#define AVB_1722_1_ADP_LISTENER_CAPABILITIES_CONTROL_SINK                   (0x0400)
#define AVB_1722_1_ADP_LISTENER_CAPABILITIES_MEDIA_CLOCK_SINK               (0x0800)
#define AVB_1722_1_ADP_LISTENER_CAPABILITIES_SMPTE_SINK                     (0x1000)
#define AVB_1722_1_ADP_LISTENER_CAPABILITIES_MIDI_SINK                      (0x2000)
#define AVB_1722_1_ADP_LISTENER_CAPABILITIES_AUDIO_SINK                     (0x4000)
#define AVB_1722_1_ADP_LISTENER_CAPABILITIES_VIDEO_SINK                     (0x8000)

#define AVB_1722_1_ADP_CONTROLLER_CAPABILITIES_IMPLEMENTED                  (0x0001)
#define AVB_1722_1_ADP_CONTROLLER_CAPABILITIES_LAYER3_PROXY                 (0x0002)

/* Default audio formats fields */
#define AVB_1722_1_ADP_DEFAULT_AUDIO_SAMPLE_RATES_MASK                  (0xFC000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_MAX_CHANS_MASK                     (0x03FC0000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_SAF_MASK                           (0x00020000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FLOAT_MASK                         (0x00010000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_CHAN_FORMATS_MASK                  (0x0000FFFF)

#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_MONO                        (0x00000001)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_2_CH                        (0x00000002)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_3_CH                        (0x00000004)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_4_CH                        (0x00000008)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_5_CH                        (0x00000010)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_6_CH                        (0x00000020)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_7_CH                        (0x00000040)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_8_CH                        (0x00000080)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_10_CH                       (0x00000100)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_12_CH                       (0x00000200)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_14_CH                       (0x00000400)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_16_CH                       (0x00000800)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_18_CH                       (0x00001000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_20_CH                       (0x00002000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_22_CH                       (0x00004000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_24_CH                       (0x00008000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_FLOAT                       (0x00010000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_SAF                         (0x00020000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_MAX_STREAMS_4               (0x00100000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_MAX_STREAMS_8               (0x00200000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_MAX_STREAMS_16              (0x00400000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_44K1                        (0x04000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_48K                         (0x08000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_88K2                        (0x10000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_96K                         (0x20000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_176K4                       (0x40000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_192K                        (0x80000000)

#define AVB_1722_1_ADP_ENTITY_TYPE_OTHER                                (0x00000001)
#define AVB_1722_1_ADP_ENTITY_TYPE_MULTIFUNCTION                        (0x00000002)
#define AVB_1722_1_ADP_ENTITY_TYPE_LOUDSPEAKER                          (0x00000004)
#define AVB_1722_1_ADP_ENTITY_TYPE_MICROPHONE                           (0x00000008)
#define AVB_1722_1_ADP_ENTITY_TYPE_AUDIO_AMPLIFIER                      (0x00000010)
#define AVB_1722_1_ADP_ENTITY_TYPE_AUDIO_SOURCE                         (0x00000020)
#define AVB_1722_1_ADP_ENTITY_TYPE_AUDIO_PROCESSOR                      (0x00000040)
#define AVB_1722_1_ADP_ENTITY_TYPE_AUDIO_MIXER                          (0x00000080)
#define AVB_1722_1_ADP_ENTITY_TYPE_HEADSET                              (0x00000100)
#define AVB_1722_1_ADP_ENTITY_TYPE_COMPUTER                             (0x00000200)
#define AVB_1722_1_ADP_ENTITY_TYPE_MUSICAL_INSTRUMENT                   (0x00000400)
#define AVB_1722_1_ADP_ENTITY_TYPE_MIDI_DEVICE                          (0x00000800)
#define AVB_1722_1_ADP_ENTITY_TYPE_MEDIA_SERVER                         (0x00001000)
#define AVB_1722_1_ADP_ENTITY_TYPE_MEDIA_RECORDER                       (0x00002000)
#define AVB_1722_1_ADP_ENTITY_TYPE_VIDEO_SOURCE                         (0x00004000)
#define AVB_1722_1_ADP_ENTITY_TYPE_VIDEO_DISPLAY                        (0x00008000)
#define AVB_1722_1_ADP_ENTITY_TYPE_VIDEO_PROCESSOR                      (0x00010000)
#define AVB_1722_1_ADP_ENTITY_TYPE_VIDEO_MIXER                          (0x00020000)
#define AVB_1722_1_ADP_ENTITY_TYPE_TIMING_DEVICE                        (0x00040000)

/**
 *  A 1722.1 AVDECC Discovery Protocol packet
 *
 *  \note all elements 16 bit aligned
 */
typedef struct {
    avb_1722_1_packet_header_t header;
    unsigned char entity_guid[8];
    unsigned char vendor_id[4];
    unsigned char entity_model_id[4];
    unsigned char entity_capabilities[4];
    unsigned char talker_stream_sources[2];
    unsigned char talker_capabilities[2];
    unsigned char listener_stream_sinks[2];
    unsigned char listener_capabilites[2];
    unsigned char controller_capabilities[4];
    unsigned char available_index[4];
    unsigned char as_grandmaster_id[8];
    unsigned char reserved0[4];
    unsigned char reserved1[4];
    unsigned char association_id[8];
    unsigned char reserved2[4];
} avb_1722_1_adp_packet_t;

#define AVB_1722_1_ADP_PACKET_SIZE (sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_adp_packet_t))

typedef struct {
    guid_t guid;
    unsigned int vendor_id;
    unsigned int entity_model_id;
    unsigned int capabilities;
    unsigned short talker_stream_sources;
    unsigned short talker_capabilities;
    unsigned short listener_stream_sinks;
    unsigned short listener_capabilites;
    unsigned int controller_capabilities;
    unsigned int available_index;
    gmid_t as_grandmaster_id;
    unsigned int association_id;
    unsigned timeout;
} avb_1722_1_entity_record;

typedef enum {
    ENTITY_AVAILABLE = 0,
    ENTITY_DEPARTING = 1,
    ENTITY_DISCOVER = 2
} avb_1722_1_adp_message_type;


#endif /* AVB_1722_1_ADPDU_H_ */
