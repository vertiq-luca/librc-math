/**
 * @example    rc_test_filters.c
 *
 * @brief      Demonstrates the use of the timestamp filter for Camera and IMU
 *
 *
 *
 * @author     James Strawson
 * @date       9/9/2022
 */


#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <rc_math.h>

#define IMU_ODR			1000
#define IMU_SAMPLES		10
#define CAM_ODR			30
#define CAM_SAMPLES		1

#define DEFAULT_PHASE_NOISE		0.5		// +- 50% error per sample
#define DEFAULT_SCALE_ERR		1.02	// 2 percent slow


static int running = 0;

// IMU test
static double odr_expected = CAM_ODR;
static int64_t samples = 1;
static double phase_noise = DEFAULT_PHASE_NOISE;
static double scale_err = DEFAULT_SCALE_ERR;


// interrupt handler to catch ctrl-c
static void __signal_handler(__attribute__ ((unused)) int dummy)
{
	running=0;
	return;
}


static int _parse_opts(int argc, char* argv[])
{
	static struct option long_options[] =
	{
		{"cam",             no_argument,       0, 'c'},
		{"help",            no_argument,       0, 'h'},
		{"imu",             no_argument,       0, 'i'},
		{"phase_noise",     required_argument, 0, 'p'},
		{"odr",             required_argument, 0, 'o'},
		{"scale_err",       required_argument, 0, 's'},
		{"samples",         required_argument, 0, 't'},
		{0, 0, 0, 0}
	};

	while(1){
		int option_index = 0;
		int c = getopt_long(argc, argv, "chip:o:s:t:", long_options, &option_index);

		if(c == -1) break; // Detect the end of the options.

		switch(c){
		case 0:
			// for long args without short equivalent that just set a flag
			// nothing left to do so just break.
			if (long_options[option_index].flag != 0) break;
			break;
		case 'c':
			odr_expected = CAM_ODR;
			samples = CAM_SAMPLES;
			break;
		case 'h':
			//_print_usage();
			exit(0);
		case 'i':
			odr_expected = IMU_ODR;
			samples = IMU_SAMPLES;
			break;
		case 'p':
			phase_noise = atof(optarg);
			if(phase_noise<0.0){
				fprintf(stderr, "ERROR: phase_noise must be >= 0.0\n");
				exit(-1);
			}
			break;
		case 'o':
			odr_expected = atof(optarg);
			if(odr_expected<0.0){
				fprintf(stderr, "ERROR: odr must be >= 0.0\n");
				exit(-1);
			}
			break;
		case 's':
			scale_err = atof(optarg);
			if(scale_err<0.8 || scale_err>1.2){
				fprintf(stderr, "ERROR: scale must be between 0.8 and 1.2\n");
				exit(-1);
			}
			break;
		case 't':
			samples = atoi(optarg);
			if(samples<1){
				fprintf(stderr, "ERROR: samples must be >=1\n");
				exit(-1);
			}
			break;
		default:
			//_print_usage();
			exit(-1);
		}
	}


	return 0;
}


int main(int argc, char* argv[])
{
	// check for options
	if(_parse_opts(argc, argv)) return -1;

	// correct for intentional rate error
	double odr_real = odr_expected / scale_err;
	int64_t dt = 1e9 / odr_real;


	printf("\nSettings in use:\n");
	printf("applied scale error:      %0.2f\n", scale_err);
	printf("expected ODR:             %0.1f\n", odr_expected);
	printf("ODR with scale error:     %0.1f\n", odr_real);
	printf("dt (ms) with scale error: %0.2f\n", dt/1000000.0);
	printf("\n");

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
		int64_t estimated_timestamp;
		if(samples==1){
			estimated_timestamp = rc_ts_filter_calc(&f, best_guess);
		}else{
			estimated_timestamp = rc_ts_filter_calc_multi(&f, best_guess, samples);
		}


		// now
		int64_t induced_error_ns = dt * phase_noise * rc_get_random_double();
		t_next = t_next + samples*dt;
		//printf("sleeping for %0.1fms\n", (t_next - t_current)/1000000.0);
		rc_nanosleep(t_next-t_current+induced_error_ns);
	}

	printf("\nDONE\n");
	return 0;
}
