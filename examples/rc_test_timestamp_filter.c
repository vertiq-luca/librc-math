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
static double odr_expected = CAM_ODR;
static int64_t samples = 1;
static double phase_noise = DEFAULT_PHASE_NOISE;
static double scale_err = DEFAULT_SCALE_ERR;
static int bad_samples = 0;
static int en_debug = 0;
static int en_response = 0;

// interrupt handler to catch ctrl-c
static void __signal_handler(__attribute__ ((unused)) int dummy)
{
	running=0;
	return;
}


static void _print_usage(void)
{
	printf("\n");
	printf("rc_test_timestamp_filter\n");
	printf("\n");
	printf("Tool for evalulating the timestamp filter using simulated\n");
	printf("noisy timestamp data.\n");
	printf("\n");
	printf("\n");
	printf("-b --bad {n}       trigger a bad read every n wakeups\n");
	printf("-c --cam           preset for 30fps camera sim\n");
	printf("-d --debug         enable the API's build in debug mode\n");
	printf("-h --help          print this help message\n");
	printf("-i --imu           preset for 1khz imu reading 10 samples each time\n");
	printf("-n --noise {val}   noise level coefficient. Multiplied by dt to find\n");
	printf("                     the max deviation from idea timestamp. For the\n");
	printf("                     default value of 0.5 and a sample rate of 100hz,\n");
	printf("                     this would give a dt of 10ms and timestamp guess\n");
	printf("                     error would be +-5ms\n");
	printf("-o --odr {val}     simulated output data rate in hz(default 30)\n");
	printf("-s --scale {val}   simluate an error in the ODR, for example when\n");
	printf("-r --response      print only the error in ms for evalulating response.\n");
	printf("                     sampling an IMU with an inaccurate internal clock.\n");
	printf("                     Default value of 1.02 means an actual dt between\n");
	printf("                     samples in 1.02 (2 percent) more than expected.\n");
	printf("-t --samples {val} specify number of samples read per wakeup. Defaults\n");
	printf("                     to 1 which is most common. IMU preset mode sets this\n");
	printf("                     to 10 to simulate reading 10 samples from an IMU\n");
	printf("                     FIFO buffer each time the bus is serviced.\n");
	printf("\n");

}


static int _parse_opts(int argc, char* argv[])
{
	static struct option long_options[] =
	{
		{"bad",             required_argument, 0, 'b'},
		{"cam",             no_argument,       0, 'c'},
		{"debug",           no_argument,       0, 'd'},
		{"help",            no_argument,       0, 'h'},
		{"imu",             no_argument,       0, 'i'},
		{"noise",           required_argument, 0, 'n'},
		{"odr",             required_argument, 0, 'o'},
		{"response",        no_argument,       0, 'r'},
		{"scale",           required_argument, 0, 's'},
		{"samples",         required_argument, 0, 't'},
		{0, 0, 0, 0}
	};

	while(1){
		int option_index = 0;
		int c = getopt_long(argc, argv, "b:cdhin:o:rs:t:", long_options, &option_index);

		if(c == -1) break; // Detect the end of the options.

		switch(c){
		case 0:
			// for long args without short equivalent that just set a flag
			// nothing left to do so just break.
			if (long_options[option_index].flag != 0) break;
			break;
		case 'b':
			bad_samples = atoi(optarg);
			if(bad_samples<0){
				fprintf(stderr, "ERROR: bad_samples must be >=0\n");
				exit(-1);
			}
			break;
		case 'c':
			odr_expected = CAM_ODR;
			samples = CAM_SAMPLES;
			break;
		case 'd':
			en_debug = 1;
			break;
		case 'h':
			_print_usage();
			exit(0);
		case 'i':
			odr_expected = IMU_ODR;
			samples = IMU_SAMPLES;
			break;
		case 'n':
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
		case 'r':
			en_response = 1;
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
			_print_usage();
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
	if(bad_samples>0) printf("triggering a bad read every %d wakeups\n", bad_samples);
	printf("\n");

	// set up filter with the ODR we expect from the sensor
	rc_ts_filter_t f = RC_TS_FILTER_INITIALIZER;
	rc_ts_filter_init(&f, odr_expected);

	// turn on debug printing so we can monitor the filter
	if(en_debug) f.en_debug_prints = 1;

	// set signal handler so the loop can exit cleanly
	signal(SIGINT, __signal_handler);
	running = 1;

	int64_t t_start = rc_time_monotonic_ns();
	int64_t t_next_ideal = t_start;
	int64_t t_actual_wakeup_last = t_start;
	int ctr = 0;


	// Keep Running until program state changes to 0
	while(running){

		int64_t t_current = rc_time_monotonic_ns();

		// guess the timestamp. Here we just grab current time. In practice you
		// should subtract things like serial port latency and camera exposure.
		int64_t best_guess = t_current;

		// estimate timestamp from best guess
		// we could technically call rc_ts_filter_calc_multi with samples set to
		// 1 as this happens internally, but we use both single and multi functions
		// here to complete the API testing
		int64_t estimated_ts;
		if(samples==1){
			estimated_ts = rc_ts_filter_calc(&f, best_guess);
		}else{
			estimated_ts = rc_ts_filter_calc_multi(&f, best_guess, samples);
		}



		// now calculate and print some stats
		int64_t ns_since_start = t_current - t_start;
		int64_t measured_dt = t_current - t_actual_wakeup_last;
		int64_t error_ns = estimated_ts - t_next_ideal;
		if(!en_debug && !en_response){
			printf("i:%5d t_s:%6.2f  scale: %5.3f  measured_dt_ms:%6.2f error_ms:%6.2f\n",\
								ctr,\
								(double)ns_since_start/1000000000.0,\
								f.clock_ratio,\
								(double)measured_dt/1000000.0,\
								(double)error_ns/1000000.0);
		}

		// print just the ts error to help plot impulse response
		if(en_response){
			printf("%6.2f\n", (double)error_ns/1000000.0);
		}

		// simulate skipping a read cycle such as a bus read error or dropped frame
		if(bad_samples>0 && ctr%bad_samples == 0){
			if(!en_debug) printf("simulating a bad reading\n");
			rc_ts_filter_report_bad_read(&f);

			// skip a whole read cycle
			t_next_ideal = t_next_ideal + samples*dt;
		}


		// this is the next time we should wake up assuming perfect sampling
		t_next_ideal = t_next_ideal + samples*dt;

		// calculate new random noise to add to this ideal time
		int64_t induced_error_ns = dt * phase_noise * rc_get_random_double();

		// sleep until next random wakeup time and bump loop counter
		rc_nanosleep(t_next_ideal-t_current + induced_error_ns);
		ctr++;
		t_actual_wakeup_last = t_current;
	}

	printf("\nDONE\n");
	return 0;
}
