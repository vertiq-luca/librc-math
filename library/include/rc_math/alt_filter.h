


#ifndef RC_ALT_FILTER_H
#define RC_ALT_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rc_math/timed_ringbuf.h>
#include <rc_math/filter.h>
#include <stdint.h>


typedef struct rc_alt_filter_t{
	// User configurable fields
	int en_debug_prints;	///< set to 1 to enable API calls to print debug info

	double crossover_filter_constant;
	double feedback_constant;
	int baro_buf_len;

	// state fields, read only
	double odr_hz;			///< framerate of the optic flow camera
	double dt;				///< seconds
	int initialized;		///< set to 1 by rc_alt_filter_init()
	rc_filter_t lpf;
	rc_filter_t hpf;

	rc_timed_ringbuf_t baro_buf;
	rc_timed_ringbuf_t baro_v_buf;
	int counter;
	double last_error;
	double err_integrator;

	// final output of filter
	int is_valid;			///< last output and last_ts are only valid if this is non-zero
	int64_t last_ts;
	double last_output;			///< final output of our predictor
	double current_ground_alt; // altitude relative to barometer of the ground
} rc_alt_filter_t;


#define RC_ALT_FILTER_INITIALIZER {\
	.en_debug_prints = 0,\
	.odr_hz = 0.0,\
	.dt = 0.0,\
	.crossover_filter_constant = 1.0,\
	.feedback_constant = 0.2,\
	.baro_buf_len = 100,\
	.initialized = 0,\
	.lpf = RC_FILTER_INITIALIZER,\
	.hpf = RC_FILTER_INITIALIZER,\
	.baro_buf = RC_TIMED_RINGBUF_INITIALIZER,\
	.baro_v_buf = RC_TIMED_RINGBUF_INITIALIZER,\
	.counter = 0,\
	.last_error = 0,\
	.err_integrator = 0,\
	.is_valid = 0,\
	.last_ts = 0,\
	.last_output = 0.0,\
	.current_ground_alt = 0.0\
}



int rc_alt_filter_init(rc_alt_filter_t* f, double odr_hz);

// set scale to 0 if it's unknown for one frame
// scale smaller than 1 means ascending
int rc_alt_filter_add_flow(rc_alt_filter_t* f, double scale, int64_t ts_ns);

int rc_alt_filter_add_baro(rc_alt_filter_t* f, double alt_m, int64_t ts_ns);


#ifdef __cplusplus
}
#endif

#endif // RC_ALT_FILTER_H
