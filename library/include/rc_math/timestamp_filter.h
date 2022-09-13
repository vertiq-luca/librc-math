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

#ifndef RC_TS_FILTER_H
#define RC_TS_FILTER_H

#include <stdint.h>

typedef struct rc_ts_filter_t{
	// User configurable fields
	int en_debug_prints;	///< set to 1 to enable API calls to print debug info
	int64_t error_tol_ns;	///< guesses that deviate from the prediction by more than this will be flagged as a bad read
	double expected_odr;	///< expected output data rate, set by rc_ts_filter_init
	double phase_constant;	///< default 20, lower converges faster
	double scale_constant;	///< defualt 50, lower converges faster

	// state fields, read only
	int initialized;		///< set to 1 by rc_ts_filter_init()
	double clock_ratio;		///< starts at 1, converges on the estimated ratio of expected_odr to estimated_odr
	int64_t last_ts_ns;		///< last estimated timestamp returned by a _calc() function
	double last_diff;		///< previous step's difference between guessed and estimated timestamp
	int bad_read_flag;		///< flag indicating a timestamp guess was wrong or samples were dropped
} rc_ts_filter_t;


#define RC_TS_FILTER_INITIALIZER {\
	.en_debug_prints = 0,\
	.error_tol_ns = 100000000,\
	.expected_odr = 0,\
	.phase_constant = 50.0,\
	.scale_constant = 50.0,\
	.initialized = 0,\
	.clock_ratio = 1.0,\
	.last_ts_ns = 0,\
	.last_diff = 0.0,\
	.bad_read_flag = 0\
}



int rc_ts_filter_init(rc_ts_filter_t* f, double odr);

int64_t rc_ts_filter_calc(rc_ts_filter_t* f, int64_t best_guess);

int64_t rc_ts_filter_calc_multi(rc_ts_filter_t* f, int64_t best_guess, int samples);

int rc_ts_filter_report_bad_read(rc_ts_filter_t* f);


#endif // RC_TS_FILTER_H
