/**
 * @example    rc_test_filters.c
 *
 * @brief      Demonstrates the use of the discrete time SISO filters.
 *
 *             It sets up three filters, a complentary low & high pass filter
 *             along with an integrator. It varies a common input u from 0 to 1
 *             through time and show the output of each filter. It also displays
 *             the sum of the complementary high and low pass filters to
 *             demonstrate how they sum to 1
 *
 * @author     James Strawson
 * @date       1/29/2018
 */

#define __USE_POSIX199309
#define _POSIX_C_SOURCE 199309L

#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <rc_math.h>

#define IMU_ODR			1000
#define CAM_ODR			30
#define PHASE_NOISE		0.2		// +- 50% error per sample
#define RATE_ERR		1.02	// 2 percent slow


static int running = 0;

// interrupt handler to catch ctrl-c
static void __signal_handler(__attribute__ ((unused)) int dummy)
{
	running=0;
	return;
}



int main()
{
	// IMU test
	double odr_expected = CAM_ODR;

	// correct for intentional rate error
	double odr_real = odr_expected / RATE_ERR;
	int64_t dt = 1e9 / odr_real;

	// set up filter with the ODR we expect from the sensor
	rc_ts_filter_t f = RC_TS_FILTER_INITIALIZER;
	rc_ts_filter_init(&f, odr_expected);

	// turn on debug printing so we can monitor the filter
	f.en_debug_prints = 1;

	// set signal handler so the loop can exit cleanly
	signal(SIGINT, __signal_handler);
	running = 1;

	int64_t t_next = rc_time_monotonic_ns();

	// Keep Running until program state changes to 0
	while(running){

		int64_t t_current = rc_time_monotonic_ns();


		// guess the timestamp. Here we just grab current time. In practice you
		// should subtract things like serial port latency and camera exposure.
		int64_t best_guess = t_current;

		// estimate timestamp from best guess
		int64_t estimated_timestamp = rc_ts_filter_calc(&f, best_guess);



		// now
		int64_t induced_error_ns = dt * PHASE_NOISE * rc_get_random_double();
		t_next = t_next + dt + induced_error_ns;
		//printf("sleeping for %0.1fms\n", (t_next - t_current)/1000000.0);
		rc_nanosleep(t_next-t_current);
	}

	printf("\nDONE\n");
	return 0;
}
