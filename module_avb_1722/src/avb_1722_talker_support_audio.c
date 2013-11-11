/**
 * \file avb_1722_talker_support_audio.c
 * \brief 1722 Talker support C functions
 */
 #include <print.h>
 #include <xscope.h>
#include "avb_conf.h"

#if AVB_NUM_SOURCES > 0 && (defined(AVB_1722_FORMAT_61883_6) || defined(AVB_1722_FORMAT_SAF))

#define streaming
#include <xccompat.h>
#include <string.h>

#include "avb_1722_talker.h"
#include "gptp.h"
#include "media_input_fifo.h"

// default audio sample type 24bits.
unsigned int AVB1722_audioSampleType = MBLA_24BIT;


/** This configure AVB Talker buffer for a given stream configuration.
 *  It updates the static portion of Ehternet/AVB transport layer headers.
 */
void AVB1722_Talker_bufInit(unsigned char Buf0[],
        avb1722_Talker_StreamConfig_t *pStreamConfig,
        int vlanid)
{
    int i;
    unsigned char *Buf = &Buf0[2];
    AVB_Frame_t *pEtherHdr = (AVB_Frame_t *) &(Buf[0]);
    AVB_DataHeader_t *p1722Hdr = (AVB_DataHeader_t *) &(Buf[AVB_ETHERNET_HDR_SIZE]);
    AVB_AVB1722_CIP_Header_t *p61883Hdr = (AVB_AVB1722_CIP_Header_t *) &(Buf[AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE]);

    unsigned data_block_size;

    // store the sample type
    switch (pStreamConfig->sampleType)
    {
    case MBLA_20BIT:
        AVB1722_audioSampleType = MBLA_20BIT;
        data_block_size = pStreamConfig->num_channels * 1;
        break;
    case MBLA_16BIT:
        AVB1722_audioSampleType = MBLA_16BIT;
        data_block_size = pStreamConfig->num_channels / 2;
        break;
    case MBLA_24BIT:
        AVB1722_audioSampleType = MBLA_24BIT;
        data_block_size = pStreamConfig->num_channels * 1;
        break;
    default:
        //printstr("ERROR: AVB1722_Talker_bufInit : Unsupported audio MBLA type.\n");
        AVB1722_audioSampleType = MBLA_24BIT;
        data_block_size = pStreamConfig->num_channels * 1;
        break;
    }

    // clear all the bytes in header.
    memset( (void *) Buf, 0, (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE));

    //--------------------------------------------------------------------------
    // 1. Initialaise the ethernet layer.
    // copy both Src/Dest MAC address
    for (i = 0; i < MAC_ADRS_BYTE_COUNT; i++) {
        pEtherHdr->DA[i] = pStreamConfig->destMACAdrs[i];
        pEtherHdr->SA[i] = pStreamConfig->srcMACAdrs[i];
    }
    SET_AVBTP_TPID(pEtherHdr, AVB_TPID);
    SET_AVBTP_PCP(pEtherHdr, AVB_DEFAULT_PCP);
    SET_AVBTP_CFI(pEtherHdr, AVB_DEFAULT_CFI);
    SET_AVBTP_VID(pEtherHdr, vlanid);
    SET_AVBTP_ETYPE(pEtherHdr, AVB_1722_ETHERTYPE);

    //--------------------------------------------------------------------------
    // 2. Initialaise the AVB TP layer.
    // NOTE: Since the data structure is cleared before we only set the requird bits.
    SET_AVBTP_SV(p1722Hdr, 1); // set stream ID to valid.
    SET_AVBTP_STREAM_ID0(p1722Hdr, pStreamConfig->streamId[0]);
    SET_AVBTP_STREAM_ID1(p1722Hdr, pStreamConfig->streamId[1]);

    //--------------------------------------------------------------------------
    // 3. Initialise the Simple Audio Format protocol specific part
#ifdef AVB_1722_FORMAT_SAF
    //TODO://This is hardcoded for 48k 32 bit 32 bit samples 2 channels
    SET_AVBTP_PROTOCOL_SPECIFIC(p1722Hdr, 2);
    SET_AVBTP_GATEWAY_INFO(p1722Hdr, 0x02000920);
    SET_AVBTP_SUBTYPE(p1722Hdr, 2);
#else
    //--------------------------------------------------------------------------
    // 3. Initialaise the 61883 CIP protocol specific part
    SET_AVB1722_CIP_TAG(p1722Hdr, AVB1722_DEFAULT_TAG);
    SET_AVB1722_CIP_CHANNEL(p1722Hdr, AVB1722_DEFAULT_CHANNEL);
    SET_AVB1722_CIP_TCODE(p1722Hdr, AVB1722_DEFAULT_TCODE);
    SET_AVB1722_CIP_SY(p1722Hdr, AVB1722_DEFAULT_SY);

    SET_AVB1722_CIP_EOH1(p61883Hdr, AVB1722_DEFAULT_EOH1);
    SET_AVB1722_CIP_SID(p61883Hdr, AVB1722_DEFAULT_SID);
    SET_AVB1722_CIP_DBS(p61883Hdr, data_block_size);

    SET_AVB1722_CIP_FN(p61883Hdr, AVB1722_DEFAULT_FN);
    SET_AVB1722_CIP_QPC(p61883Hdr, AVB1722_DEFAULT_QPC);
    SET_AVB1722_CIP_SPH(p61883Hdr, AVB1722_DEFAULT_SPH);
    SET_AVB1722_CIP_DBC(p61883Hdr, AVB1722_DEFAULT_DBC);

    SET_AVB1722_CIP_EOH2(p61883Hdr, AVB1722_DEFAULT_EOH2);
    SET_AVB1722_CIP_FMT(p61883Hdr, AVB1722_DEFAULT_FMT);
    SET_AVB1722_CIP_FDF(p61883Hdr, AVB1722_DEFAULT_FDF);
    SET_AVB1722_CIP_SYT(p61883Hdr, AVB1722_DEFAULT_SYT);
#endif

}

static void sample_copy_strided(int *src, unsigned int *dest, int stride, int n) {
    int i;
    for (i = 0; i < n; i++) {
#ifdef AVB_1722_FORMAT_SAF
        unsigned sample = *src << 8;
#else
        unsigned sample = (*src & 0xffffff) | AVB1722_audioSampleType;
#endif
        sample = __builtin_bswap32(sample);
        *dest = sample;
        src += 1;
        dest += stride;
    }
}

/** This receives user defined audio samples from local out stream and packetize
 *  them into specified AVB1722 transport packet.
 */
int avb1722_create_packet(unsigned char Buf0[],
        avb1722_Talker_StreamConfig_t *stream_info,
        ptp_time_info_mod64 *timeInfo,
        int time)
{
    unsigned int presentationTime = 0;
    int timerValid = 0;
    int i;
    int num_channels = stream_info->num_channels;
    int stream_id0 = stream_info->streamId[0];
    media_input_fifo_t *map = stream_info->map;
    int samples_per_fifo_packet = stream_info->samples_per_fifo_packet;
    int num_audio_samples;
    int samples_in_packet;

    // align packet 2 chars into the buffer so that samples are
    // word align for fast copying.
    unsigned char *Buf = &Buf0[2];

#ifdef AVB_1722_FORMAT_SAF
    unsigned int *dest = (unsigned int *) &Buf[(AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE)];
#else
    unsigned int *dest = (unsigned int *) &Buf[(AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE)];
#endif

    int stride = num_channels;
    unsigned ptp_ts = 0;
    int dbc;
    int pkt_data_length;

    if (stream_info->initial)
    {
        if (media_input_fifo_fill_level(map[0]) < MEDIA_INPUT_FIFO_SAMPLE_FIFO_SIZE/2)
            return 0;
    }

    // Figure out the number of samples in the 1722 packet
    samples_in_packet = stream_info->samples_per_packet_base;

    stream_info->rem += stream_info->samples_per_packet_fractional;

    if (stream_info->rem & 0xffff0000) {
        samples_in_packet += 1;
        stream_info->rem &= 0xffff;
    }

    if (stream_info->initial)
    {
        if (media_input_fifo_fill_level(map[0]) < MEDIA_INPUT_FIFO_SAMPLE_FIFO_SIZE/2) return 0;
    }

    // Check to see if there is something that can be transmitted.  If there is not, then we give up
    // transmitting this packet, because there may be other streams serviced by this thread which
    // can be serviced.  Since a packet on the wire is always shorter than a packet in the fifo,
    // we know that having a packet in the fifo is enough to transmit one on the wire.
    //
    // There is a slight issue here, that because wire packets are potentially shorter than fifo
    // packets, that we will occasionally not transmit when we could do. The period of this is
    // 1/(ceil(rate/8000)-(rate/8000))
    for (i = 0; i < num_channels; i++) {
        // If more data is required from the media_fifo then check there are
        // enough samples present so that this thread won't wait for data
        //
        // The worst-case is that it will read twice from the media_input_fifo
        const int need_more_data = stream_info->initial != 0 ||
            stream_info->samples_left_in_fifo_packet < samples_in_packet;
        if (need_more_data)
        {
            const int not_enough_data = media_input_fifo_fill_level(map[i]) < (2 * samples_per_fifo_packet);
            if (not_enough_data)
                return 0;
        }
    }

    // If the FIFOs are not being filled then also do not process the packet
    if ((media_input_fifo_enable_ind_state() & stream_info->fifo_mask) == 0)
        return 0;

    // Figure out if it is time to transmit a packet
    if (!stream_info->initial && !stream_info->transmit_ok) {
        int elapsed = time - stream_info->last_transmit_time;
        if (elapsed < AVB1722_PACKET_PERIOD_TIMER_TICKS)
            return 0;

        stream_info->transmit_ok = 1;
    }

    AVB_Frame_t *pEtherHdr = (AVB_Frame_t *) &(Buf[0]);
    for (i = 0; i < MAC_ADRS_BYTE_COUNT; i++) {
        pEtherHdr->DA[i] = stream_info->destMACAdrs[i];
    }

    num_audio_samples = samples_in_packet * num_channels;

#ifdef AVB_1722_FORMAT_SAF
    pkt_data_length = (num_audio_samples << 2);
#else
    pkt_data_length = AVB_CIP_HDR_SIZE + (num_audio_samples << 2);
#endif

    // Find the DBC for the current stream
    dbc = stream_info->dbc_at_start_of_last_fifo_packet;
    dbc += samples_per_fifo_packet - stream_info->samples_left_in_fifo_packet;

    // Get the audio data packet information
    // Note: see 61883-6 section 6.2 which explains that the timestamps coming out
    // of the FIFO should refer to the data blocks which are equal to zero mod
    // the SYT_INTERVAL.  this is implicit in our implementation, because each
    // fifo packet is a group of data blocks, and the first sample, which carries
    // the timestamp is index 0
    if (stream_info->initial != 0) {
        for (i = 0; i < num_channels; i++) {
            int *src = (int *)media_input_fifo_get_packet(map[i],
                    &presentationTime,
                    &(stream_info->dbc_at_start_of_last_fifo_packet));

            media_input_fifo_set_ptr(map[i], src);
        }
        timerValid = 1;
        stream_info->initial = 0;
        dbc = stream_info->dbc_at_start_of_last_fifo_packet;
        stream_info->samples_left_in_fifo_packet = samples_per_fifo_packet;
    } else {
        if (stream_info->samples_left_in_fifo_packet < samples_in_packet) {
            // Not enough samples left in fifo packet to fill the 1722 packet
            // therefore pull out remaining samples and get the next packet
            for (i = 0; i < num_channels; i++) {
                int *src = media_input_fifo_get_ptr(map[i]);
                sample_copy_strided(src, dest, stride, stream_info->samples_left_in_fifo_packet);
                media_input_fifo_release_packet(map[i]);
                src = (int *)media_input_fifo_get_packet(map[i],
                        &presentationTime,
                        &(stream_info->dbc_at_start_of_last_fifo_packet));
                media_input_fifo_set_ptr(map[i], src);
                dest += 1;
                timerValid = 1;
            }
            dest += (stream_info->samples_left_in_fifo_packet - 1) * num_channels;
            samples_in_packet -= stream_info->samples_left_in_fifo_packet;
            stream_info->samples_left_in_fifo_packet = samples_per_fifo_packet;
        }
    }

    for (i = 0; i < num_channels; i++) {
        int *src = media_input_fifo_get_ptr(map[i]);
        sample_copy_strided(src, dest, stride, samples_in_packet);
        dest += 1;
        media_input_fifo_set_ptr(map[i], src + samples_in_packet);
    }

    stream_info->samples_left_in_fifo_packet -= samples_in_packet;

    dbc &= 0xff;

#ifndef AVB_1722_FORMAT_SAF
    AVB1722_CIP_HeaderGen(Buf, dbc);
#endif
    // perform required updates to header
    if (timerValid) {
        ptp_ts = local_timestamp_to_ptp_mod32(presentationTime, timeInfo);
        ptp_ts = ptp_ts + stream_info->presentation_delay;
    }

    // Update timestamp value and valid flag.
    AVB1722_AVBTP_HeaderGen(Buf, timerValid, ptp_ts, pkt_data_length, stream_info->sequence_number, stream_id0);

    stream_info->transmit_ok = 0;
    stream_info->sequence_number++;
    return (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + pkt_data_length);
}

#endif
