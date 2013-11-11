#ifndef _PTP_MESSAGE_FORMAT_H_
#define _PTP_MESSAGE_FORMAT_H_   1

#include "nettypes.h"

// Bit mask for PTP Message Type
#define PTP_MESSAGE_TYPE_MASK             (0xF)

// PTP Message type definations
#define PTP_SYNC_MESG                     (0x0)
#define PTP_DELAY_REQ_MESG                (0x1)
#define PTP_PDELAY_REQ_MESG               (0x2)
#define PTP_PDELAY_RESP_MESG              (0x3)
#define PTP_FOLLOW_UP_MESG                (0x8)
#define PTP_DELAY_RESP_MESG               (0x9)
#define PTP_PDELAY_RESP_FOLLOW_UP_MESG    (0xA)
#define PTP_ANNOUNCE_MESG                 (0xB)
#define PTP_SIGNALING_MESG                (0xC)
#define PTP_MANAGEMENT_MESG               (0xD)

// PTP Control Field ENUM
#define PTP_CTL_FIELD_SYNC                (0x0)
#define PTP_CTL_FIELD_DELAY_REQ           (0x1)
#define PTP_CTL_FIELD_FOLLOW_UP           (0x2)
#define PTP_CTL_FIELD_DELAY_RESP          (0x3)
#define PTP_CTL_FIELD_MANAGEMENT          (0x4)
#define PTP_CTL_FIELD_OTHERS              (0x5)

// PTP common message header length
#define PTP_COMMON_MESG_HDR_LENGTH        (34)

// PTP Common Message Header format
typedef struct
{
  n8_t transportSpecific_messageType;
  n8_t versionPTP;
  n16_t messageLength;
  n8_t domainNumber;
  n8_t Resv0;
  n8_t flagField[2];
  n64_t correctionField;
  n32_t Resv1;
  n80_t sourcePortIdentity;
  n16_t sequenceId;
  n8_t controlField;
  n8_t logMessageInterval;
} ComMessageHdr;

// PTP Announce message
typedef struct
{
  n80_t originTimestamp;
  n16_t currentUtcOffset;
  n8_t  Resv1;
  n8_t  grandmasterPriority1;
  n8_t clockClass;
  n8_t clockAccuracy;
  n16_t clockOffsetScaledLogVariance;
  n8_t  grandmasterPriority2;
  n64_t grandmasterIdentity;
  n16_t stepsRemoved;
  n8_t  timeSource;
  n16_t tlvType;
  n16_t tlvLength;
  n64_t pathSequence[PTP_MAXIMUM_PATH_TRACE_TLV];
} AnnounceMessage;

// PTP Sync & Delay_Req message
typedef struct
{
  n80_t originTimestamp;
} SyncMessage;

// PTP Follow_Up message
typedef struct
{
  n80_t preciseOriginTimestamp;
  n16_t tlvType;
  n16_t lengthField;
  n8_t  organizationId[3];
  n8_t  organizationSubType[3];
  n32_t cumulativeScaledRateOffset;
  n16_t gmTimeBaseIndicator;
  n96_t lastGmPhaseChange;
  n32_t scaledLastGmFreqChange;
} FollowUpMessage;

// PTP Peer delay request message
typedef struct
{
  n80_t originTimestamp;
  n80_t Resv0;
} PdelayReqMessage;

// PTP Peer delay response message
typedef struct
{
  n80_t requestReceiptTimestamp;
  n80_t requestingPortIdentity;
} PdelayRespMessage;

// PTP Peer delay response follow up message
typedef struct
{
  n80_t responseOriginTimestamp;
  n80_t requestingPortIdentity;
} PdelayRespFollowUpMessage;

// Macro to evaluate flagField(s) in PTP message
#define ALTERNATE_MASTER_FLAG(msgHdr)        (msgHdr->flagField[0] & 0x1)

#define TWO_STEP_FLAG(msgHdr)                (msgHdr->flagField[0] & 0x2)

#define UNICAST_FLAG(msgHdr)                 (msgHdr->flagField[0] & 0x4)

#define PTP_PROFILE_SPECIFIC1_FLAG(msgHdr)   (msgHdr->flagField[0] & 0x8)

#define PTP_PROFILE_SPECIFIC2_FLAG(msgHdr)   (msgHdr->flagField[0] & 0x10)

#define LEAP61_FLAG(msgHdr)                  (msgHdr->flagField[1] & 0x1)

#define LEAP59_FLAG(msgHdr)                  (msgHdr->flagField[1] & 0x2)

#define CURRENT_UTC_OFFSET_VALID_FLAG(msgHdr)(msgHdr->flagField[1] & 0x4)

#define PTP_TIMESCALE_FLAG(msgHdr)           (msgHdr->flagField[1] & 0x8)

#define TIME_TRACEABLE_FLAG(msgHdr)          (msgHdr->flagField[1] & 0x10)

#define FREQUENCY_TRACEABLE_FLAG(msgHdr)     (msgHdr->flagField[1] & 0x20)


#endif

