

#include <stdio.h>
#include <math.h>
#include <rc_math/alt_filter.h>


rc_alt_filter_t rc_alt_filter_empty(void)
{
	rc_alt_filter_t new = RC_ALT_FILTER_INITIALIZER;
	return new;
}


int rc_alt_filter_init(rc_alt_filter_t* f, double odr_hz)
{

	// sanity checks
	if(f==NULL){
		fprintf(stderr,"ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(odr_hz<=0){
		fprintf(stderr,"ERROR in %s, odr_hz must be >=0\n", __FUNCTION__);
		return -1;
	}
	if(f->baro_buf_len<5){
		fprintf(stderr,"ERROR in %s, baro_buf_len must be >=5\n", __FUNCTION__);
		return -1;
	}
	if(f->baro_buf_len<5){
		fprintf(stderr,"ERROR in %s, baro_buf_len must be >=5\n", __FUNCTION__);
		return -1;
	}
	if(f->initialized){
		fprintf(stderr,"ERROR in %s, filter already initialized\n", __FUNCTION__);
		return -1;
	}

	f->odr_hz = odr_hz;
	f->dt = 1.0/odr_hz;

	rc_filter_first_order_lowpass(&f->lpf, f->dt, f->crossover_filter_constant);
	rc_filter_first_order_highpass(&f->hpf, f->dt, f->crossover_filter_constant);

	f->current_ground_alt = 0.0;
	rc_timed_ringbuf_alloc(&f->baro_buf, f->baro_buf_len);
	rc_timed_ringbuf_alloc(&f->baro_v_buf, f->baro_buf_len);

	f->is_valid = 0;

	f->initialized = 1;

	return 0;
}


int rc_alt_filter_add_baro(rc_alt_filter_t* f, double alt_m, int64_t ts_ns)
{
	if(!f->initialized){
		fprintf(stderr,"ERROR in %s, filter not initialized\n", __FUNCTION__);
		return -1;
	}

	// insert into altitude buffer
	rc_timed_ringbuf_insert(&f->baro_buf, ts_ns, alt_m);

	return 0;
}


int rc_alt_filter_add_vel(rc_alt_filter_t* f, double v_up, int64_t ts_ns)
{
	if(!f->initialized){
		fprintf(stderr,"ERROR in %s, filter not initialized\n", __FUNCTION__);
		return -1;
	}
	rc_timed_ringbuf_insert(&f->baro_v_buf, ts_ns, v_up);
	return 0;
}


int rc_alt_filter_add_flow(rc_alt_filter_t* f, double scale, int64_t ts_ns)
{
	if(!f->initialized){
		fprintf(stderr,"ERROR in %s, filter not initialized\n", __FUNCTION__);
		return -1;
	}

	// baro height at timestamp
	double baro_at_ts;
	if(rc_timed_ringbuf_get_val_at_time(&f->baro_buf, ts_ns, &baro_at_ts)){
		if(f->en_debug_prints){
			printf("failed to get baro height\n");
		}
		f->is_valid = 0;
		return -1;
	}
	// baro velocity at time between last two frames
	double baro_v_at_ts;
	int64_t v_ts = ts_ns - (f->dt * 500000000);
	if(rc_timed_ringbuf_get_val_at_time(&f->baro_v_buf, v_ts, &baro_v_at_ts)){
		if(f->en_debug_prints){
			printf("failed to get baro velocity\n");
		}
		f->is_valid = 0;
		return -1;
	}

	if(f->counter == 0){
		f->current_ground_alt = baro_at_ts;
	}


	// height above ground as predicted by optic scale to go into HPF
	// image shrinking means we are ascending.
	double cam_hgt = f->last_output / scale;
	int should_run_feedback = 0;
	int should_run_passthrough = 0;


	// Reasons not to run feedback:
	// - scale is too close to 1 (e.g. not moving)
	// - baro reports too low a velocity (e.g. not moving)
	// - velocity and scale contradict (opposite directions)
	double dist_from_one = fabs(scale - 1.0);
	if(dist_from_one  > f->scale_outer_limit){

		should_run_feedback = 0;
		should_run_passthrough = 1;
	}

	if(dist_from_one  < f->scale_inner_limit){
		should_run_feedback = 0;
	}

	if(	fabs(baro_v_at_ts) < f->vel_lower_limit	||\
		(baro_v_at_ts>0 && scale>1.0)			||\
		(baro_v_at_ts<0 && scale<1.0))
	{
		should_run_feedback = 0;
		should_run_passthrough = 1;
	}



	if(f->counter == 0){
		rc_filter_prefill_inputs( &f->lpf, 0.0);
		rc_filter_prefill_outputs(&f->lpf, 0.0);
		rc_filter_prefill_inputs( &f->hpf, baro_at_ts);
		rc_filter_prefill_outputs(&f->hpf, 0.0);
	}


	double h_eq = 0.0;
	double h_error = 0.0;
	double feedback = 0.0;

	if(should_run_feedback){

		// height that would match the baro velocity at that scale.
		h_eq = (baro_v_at_ts * f->dt)/(1.0-scale);
		h_error = cam_hgt - h_eq;

		// integrator not currently used, but there in case it's useful in the future
		f->err_integrator += (h_error + f->last_error) * f->dt / 2;
		f->last_error = h_error;

		double gain = f->dt / f->feedback_constant;
		feedback = h_error * gain;

		// PI feedback, not used right now
		//double feedback = (f->err_integrator * gain) + (h_error * gain * 1.1);

		// ADD feedback TO LPF
		cam_hgt -= feedback;
	}

	if(should_run_passthrough){
		cam_hgt = baro_at_ts - f->current_ground_alt;
	}


	// never let the camera height drop below min
	if(cam_hgt < f->min_hgt_to_estimate){
		cam_hgt = f->min_hgt_to_estimate;
	}

	rc_filter_march(&f->hpf, baro_at_ts);
	rc_filter_march(&f->lpf, cam_hgt);

	// sum complementary filter
	f->last_output = f->lpf.newest_output + f->hpf.newest_output;

	// when estimating altitude with camera, keep track of the baro alt
	if(should_run_feedback){
		f->current_ground_alt = baro_at_ts - f->last_output;
	}

	// lower bound on output
	if(f->last_output < f->min_hgt_to_estimate){
		f->last_output = 0.0;
	}

	if(f->en_debug_prints){
		fprintf(stderr, "scl:%5.2f", scale);
		fprintf(stderr, " fb/pt: %d%d", should_run_feedback, should_run_passthrough);
		fprintf(stderr, " v:%5.2f", baro_v_at_ts);
		fprintf(stderr, "  h_eq:%5.2f", h_eq);
		//printf(" h_err: %5.2f", h_error);
		fprintf(stderr, "  fb:%5.2f", feedback);
		fprintf(stderr, "  hcam:%5.1f", cam_hgt);
		fprintf(stderr, "  clpf:%5.1f", f->lpf.newest_output);
		fprintf(stderr, "  hbar:%5.1f", baro_at_ts);
		fprintf(stderr, "  bhpf:%5.1f", f->hpf.newest_output);
		fprintf(stderr, "  out:%5.1f", f->last_output);
		fprintf(stderr, "\n");
	}


	f->counter++;
	f->is_valid = 1;

	return 0;
}


