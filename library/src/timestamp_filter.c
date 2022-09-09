/*******************************************************************************
 * Copyright 2022 ModalAI Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * 4. The Software is used solely in conjunction with devices provided by
 *    ModalAI Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/


#include <stdio.h>
#include <unistd.h> // for usleep
#include <string.h> // for memset
#include <math.h>
#include <stdlib.h>

#include "algebra_common.h"
#include <rc_math/timestamp_filter.h>




int rc_ts_filter_init(rc_ts_filter_t* f, double odr)
{
	rc_ts_filter_t new = RC_TS_FILTER_INITIALIZER;
	new.expected_odr = odr;
	new.initialized = 1;
	*f = new;
	return 0;
}


int64_t rc_ts_filter_calc(rc_ts_filter_t* f, int64_t best_guess)
{
	return rc_ts_filter_calc_multi(f, best_guess, 1);

}


int64_t rc_ts_filter_calc_multi(rc_ts_filter_t* f, int64_t best_guess, int samples)
{
	if(unlikely(f == NULL)){
		fprintf(stderr,"ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(!f->initialized)){
		fprintf(stderr,"ERROR in %s, f not initialized yet\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(samples<1)){
		fprintf(stderr,"ERROR in %s, samples must be >=1\n", __FUNCTION__);
		return -1;
	}

	// first sample
	if(f->last_ts_ns<=0){
		f->last_ts_ns = best_guess;
		f->bad_read_flag = 0;
		return best_guess;
	}

	// if there was some sort of bad read due to serial bus error, dropped packet,
	// etc then just reset back to the best guess read
	if(f->bad_read_flag){
		if(f->en_debug_prints){
			printf("using monotonic time, lost packets since last read\n");
		}
		f->last_ts_ns = best_guess;
		f->bad_read_flag = 0;
		return best_guess;
	}

	// if the last read was good, (and assuming this read was good), we can guess
	// the next timestamp by adding the known dt to the last timestamp
	int64_t filtered_ts_ns = best_guess;

	// start by guessing and see how far off we are
	int64_t forward_prediction = f->last_ts_ns \
				+ (int64_t)((double)samples*(f->clock_ratio)*1000000000.0/f->expected_odr);

	int64_t diff = best_guess - forward_prediction;

	// if we exceeded the tolerance
	if(llabs(diff) > f->error_check_tol_ns){
		f->last_ts_ns = best_guess;
		if(f->en_debug_prints){
			printf("using monotonic time, diff too big: %6.1fms\n", diff/1000000.0);
		}
		return best_guess;
	}

	// this filter seeks to converge on the static offset between our noisy
	// monotonic clock reading and the predicted timestamp
	// this should converge quite quickly to catch up after missed packets
	filtered_ts_ns = forward_prediction + (diff/f->phase_constant);
	f->last_ts_ns = filtered_ts_ns;

	// diff should hover around 0 and just represent noise from the irregularity
	// in when the apps proc wakes up to service the data. If it trends up or
	// down then that indicates a difference in clock speed between apps and imu.
	// try to converge on that clock ratio.
	f->clock_ratio += ((double)diff/1000000000.0)/f->clock_ratio_constant;

	if(f->en_debug_prints){
		int64_t new_dt = 1000000000.0/(f->expected_odr/(f->clock_ratio));
		printf("scale: %f diff_ms: %6.1f dt_ms %7.3f\n", f->clock_ratio, diff/1000000.0, new_dt/1000000.0);
	}

	return filtered_ts_ns;
}



int rc_ts_filter_report_bad_read(rc_ts_filter_t* f)
{
	if(unlikely(f==NULL)){
		fprintf(stderr,"ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(!f->initialized)){
		fprintf(stderr,"ERROR in %s, f not initialized yet\n", __FUNCTION__);
		return -1;
	}
	f->bad_read_flag = 1;
	return 0;
}

