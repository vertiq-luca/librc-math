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

	int initialized;
	float expected_odr;
	double clock_ratio;
	int64_t last_ts_ns;
	double phase_constant;
	double clock_ratio_constant;
	int bad_read_flag;
	int en_debug_prints;
	int64_t error_check_tol_ns;
	double estimated_dt_ns;

} rc_ts_filter_t;


#define RC_TS_FILTER_INITIALIZER {\
	.initialized = 0,\
	.expected_odr = 0,\
	.clock_ratio = 1.0,\
	.last_ts_ns = -1,\
	.phase_constant = 50.0,\
	.clock_ratio_constant = 200.0,\
	.bad_read_flag = 0,\
	.en_debug_prints = 0,\
	.error_check_tol_ns = 100000000,\
	.estimated_dt_ns = 0.0\
}



int rc_ts_filter_init(rc_ts_filter_t* f, double odr);

int64_t rc_ts_filter_calc(rc_ts_filter_t* f, int64_t best_guess);

int64_t rc_ts_filter_calc_multi(rc_ts_filter_t* f, int64_t best_guess, int samples);

int rc_ts_filter_report_bad_read(rc_ts_filter_t* f);


#endif // RC_TS_FILTER_H
