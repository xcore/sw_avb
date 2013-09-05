/**
 * \file media_clock_support.c
 * \brief C functions for the media clock
 *
 * The copyrights, all other intellectual and industrial
 * property rights are retained by XMOS and/or its licensors.
 * Terms and conditions covering the use of this code can
 * be found in the Xmos End User License Agreement.
 *
 * Copyright XMOS Ltd 2010
 *
 * In the case where this code is a modification of existing code
 * under a separate license, the separate license terms are shown
 * below. The modifications to the code are still covered by the
 * copyright notice above.
 *
 **/
#include <xccompat.h>
#include "gptp.h"
#include "avb_1722_def.h"
#include "print.h"
#include "media_clock_internal.h"
#include "media_clock_client.h"
#include "misc_timer.h"

#define NANO_SECOND 1000000000

// The clock recovery internal representation of the worldlen.  More precision and range than the external
// worldlen representation.  The max percision is 26 bits before the PTP clock recovery multiplcation overflows
#define WORDLEN_FRACTIONAL_BITS 24

/**
 *  \brief Records the state of the media stream
 *
 *  It is used by the stream based clock recovery to store stream state, and
 *  therefore work out deltas between the last clock recovery state and the
 *  current one
 */
typedef struct stream_info_t {
	int valid;
	unsigned int local_ts;
	unsigned int outgoing_ptp_ts;
	unsigned int presentation_ts;
	int locked;
	int fill;
} stream_info_t;

/**
 * \brief Records the state of the clock recovery for one media clock
 */
typedef struct clock_info_t {
	unsigned long long wordlen;
	long long ierror;
	unsigned int rate;
	int first;
	stream_info_t stream_info1;
	stream_info_t stream_info2;
} clock_info_t;

/// The array of media clock state structures
static clock_info_t clock_states[AVB_NUM_MEDIA_CLOCKS];

/**
 * \brief Converts the internal 64 bit wordlen into an external 32 bit wordlen
 */
static unsigned int local_wordlen_to_external_wordlen(unsigned long long w) {
	return (w >> (WORDLEN_FRACTIONAL_BITS - WC_FRACTIONAL_BITS));
}

void init_media_clock_recovery(chanend ptp_svr,
							   int clock_num,
							   unsigned int clk_time,
							   unsigned int rate) {
	clock_info_t *clock_info = &clock_states[clock_num];

	clock_info->first = 1;
	clock_info->rate = rate;
	clock_info->ierror = 0;
	if (rate != 0) {
		clock_info->wordlen = ((100000000LL << WORDLEN_FRACTIONAL_BITS) / clock_info->rate);
	} else {
		clock_info->wordlen = 0;
	}

	clock_info->stream_info1.valid = 0;
	clock_info->stream_info2.valid = 0;
}

void update_media_clock_stream_info(int clock_index,
                                    unsigned int local_ts,
                                    unsigned int outgoing_ptp_ts,
                                    unsigned int presentation_ts,
                                    int locked,
                                    int fill) {
	clock_info_t *clock_info = &clock_states[clock_index];

	clock_info->stream_info2.local_ts = local_ts;
	clock_info->stream_info2.outgoing_ptp_ts = outgoing_ptp_ts;
	clock_info->stream_info2.presentation_ts = presentation_ts;
	clock_info->stream_info2.valid = 1;
	clock_info->stream_info2.locked = locked;
	clock_info->stream_info2.fill = fill;
}

void inform_media_clock_of_lock(int clock_index) {
	clock_info_t *clock_info = &clock_states[clock_index];
	clock_info->stream_info2.valid = 0;
}

#define MAX_ERROR_TOLERANCE 100

unsigned int update_media_clock(chanend ptp_svr,
								int clock_index,
								const media_clock_t *mclock,
								unsigned int t2,
								int period0) {
	clock_info_t *clock_info = &clock_states[clock_index];
	long long diff_local;
	int clock_type = mclock->clock_type;

	switch (clock_type) {
	case LOCAL_CLOCK:
		return local_wordlen_to_external_wordlen(clock_info->wordlen);
		break;

#ifndef MEDIA_CLOCK_EXCLUDE_STREAM_DERIVED
	case INPUT_STREAM_DERIVED: {
		long long ierror, perror;

		// If the stream info isn't valid at all, then return the default clock rate
		if (!clock_info->stream_info2.valid)
			return local_wordlen_to_external_wordlen(clock_info->wordlen);

		// If there are not two stream infos to compare, then return default clock rate
		if (!clock_info->stream_info1.valid) {
			clock_info->stream_info1 = clock_info->stream_info2;
			clock_info->stream_info2.valid = 0;
			return local_wordlen_to_external_wordlen(clock_info->wordlen);
		}

		// If the stream is unlocked, return the default clock rate
		if (!clock_info->stream_info2.locked) {
			clock_info->wordlen = ((100000000LL << WORDLEN_FRACTIONAL_BITS)
					/ clock_info->rate);
			clock_info->stream_info1 = clock_info->stream_info2;
			clock_info->stream_info2.valid = 0;
			clock_info->first = 1;
			clock_info->ierror = 0;

		// We have all the info we need to perform clock recovery
		} else {
			diff_local = clock_info->stream_info2.local_ts
					- clock_info->stream_info1.local_ts;

			ierror = (signed) clock_info->stream_info2.outgoing_ptp_ts -
					 (signed) clock_info->stream_info2.presentation_ts;

			ierror = ierror << WORDLEN_FRACTIONAL_BITS;

			if (clock_info->first) {
				perror = 0;
				clock_info->first = 0;
			} else
				perror = ierror - clock_info->ierror;

			clock_info->ierror = ierror;

#if PLL_TYPE_CS2300
			clock_info->wordlen = clock_info->wordlen - ((perror / diff_local) * 32) - ((ierror / diff_local) / 4);
#else
			clock_info->wordlen = clock_info->wordlen - ((perror / diff_local) * 80)/11 - ((ierror / diff_local) * 1) / 5;
#endif

			clock_info->stream_info1 = clock_info->stream_info2;
			clock_info->stream_info2.valid = 0;
		}
		break;
	}
#endif

		break;
	}

	return local_wordlen_to_external_wordlen(clock_info->wordlen);
}

