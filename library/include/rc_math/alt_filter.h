


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
	double min_hgt_to_estimate;
	double scale_inner_limit;	///< not not estimate alt when cam scale is inside 1+-inner_lim
	double scale_outer_limit;	///< not not estimate alt when cam scale is outside 1+-outer_lim
	double vel_lower_limit;		///< do not estimate altitude when velocity is below this

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
	.min_hgt_to_estimate = 0.5,\
	.scale_inner_limit = 0.01,\
	.scale_outer_limit = 0.1,\
	.vel_lower_limit = 0.3,\
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


// returns a clean default uninitialized rc_alt_filter struct for cases where
// you can't or don't want to use the initializer macro above (looking at you C++ -_-)
rc_alt_filter_t rc_alt_filter_empty(void);

int rc_alt_filter_init(rc_alt_filter_t* f, double odr_hz);

// set scale to 0 if it's unknown for one frame
// scale smaller than 1 means ascending
int rc_alt_filter_add_flow(rc_alt_filter_t* f, double scale, int64_t ts_ns);

// add a barometer height estimate, such as relative or monotonic fields in
// the mavlink altitude message
int rc_alt_filter_add_baro(rc_alt_filter_t* f, double alt_m, int64_t ts_ns);

// velocity should be sourced from ekf2 through something like the
// local_position_ned vs field. NOTE that you should reverse the sign of vz if
// using local_position_ned since local position has Z pointing down whereas
// the barometer altitude increases with altitude!!!
int rc_alt_filter_add_vel(rc_alt_filter_t* f, double v_up, int64_t ts_ns);


#ifdef __cplusplus
}
#endif

#endif // RC_ALT_FILTER_H
