

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <rc_math/alt_filter.h>
#include <math.h>
#include <float.h>
#include <string.h>

#define SAMPLE_RATE 10
#define DT (1.0/SAMPLE_RATE)
#define ASCENT_RATE 2.0


static char csv_path[256];


static void _print_usage()
{
	printf("TODO HELP TEXT\n");
	return;
}

static int parse_opts(int argc, char* argv[])
{
	static struct option long_options[] =
	{
		{"help",			no_argument,		0,	'h'},
		{0, 0, 0, 0}
	};

	while(1){
		int option_index = 0;
		int c = getopt_long(argc, argv, "h", long_options, &option_index);

		if(c == -1) break; // Detect the end of the options.

		switch(c){
		case 0:
			// for long args without short equivalent that just set a flag
			// nothing left to do so just break.
			if (long_options[option_index].flag != 0) break;
			break;
		case 'h':
			_print_usage();
			exit(0);

		default:
			_print_usage();
			return -1;
		}
	}

	// scan through the non-flagged arguments for the desired path
	for(int i=optind; i<argc; i++){
		if(csv_path[0]!=0){
			fprintf(stderr, "ERROR: Please specify only one path\n");
			_print_usage();
			exit(-1);
		}
		strcpy(csv_path,argv[i]);
	}

	return 0;
}


static void sim_march(rc_alt_filter_t* f, double current_height, double gnd_height)
{
	static int64_t timestamp = 0;
	static double last_baro_height = 0;
	static double last_gnd_height = 0;

	if(timestamp==0) last_baro_height = current_height;
	timestamp += DT*1000000000;

	// calc velocity
	double vel = (current_height - last_baro_height)/DT;
	last_baro_height = current_height;

	// check for sudden jumps in ground height, assume no features tracked
	// as we just went over a building
	int is_scale_good = 1;
	if(last_gnd_height != gnd_height){
		is_scale_good = 0;
		last_gnd_height = gnd_height;
	}

	// put current height straight in as barometer data
	rc_alt_filter_add_baro(f, current_height, timestamp);
	rc_alt_filter_add_vel(f, vel, timestamp);

	// calculate scale
	static double last_range = 0;
	double current_range = current_height - gnd_height;
	double scale;

	if(is_scale_good) scale = last_range/current_range;
	else scale = 1.0;
	last_range = current_range;

	printf("truth: %4.1f ", current_range);

	// add scale to filter
	rc_alt_filter_add_flow(f, scale, timestamp);

	double error = f->last_output - current_range;

	printf(" err: %5.2f", error);
	printf("\n");
	return;
}


// so a fake sim with no noise
static void sim(void)
{

	int i;
	rc_alt_filter_t f = RC_ALT_FILTER_INITIALIZER;
	f.en_debug_prints = 1;
	rc_alt_filter_init(&f, SAMPLE_RATE);

	double gnd_height = 0;
	double current_height = 0.0;

	// chill
	for(int i=0;i<0.5*SAMPLE_RATE;i++){
		sim_march(&f, current_height, gnd_height);
	}

	// ascend
	printf("---- START ASCENDING ------\n");
	for(i=0;i<10*SAMPLE_RATE;i++){
		current_height += ASCENT_RATE * DT;
		sim_march(&f, current_height, gnd_height);
	}

	// chill
	printf("---- WAIT ------\n");
	for(i=0;i<1*SAMPLE_RATE;i++){
		sim_march(&f, current_height, gnd_height);
	}

	// step & chill
	printf("---- 5m Step ------\n");
	gnd_height += 5;
	for(i=0;i<1*SAMPLE_RATE;i++){
		sim_march(&f, current_height, gnd_height);
	}

	printf("---- DOWN ------\n");
	for(i=0;i<5*SAMPLE_RATE;i++){
		current_height -= ASCENT_RATE * DT;
		sim_march(&f, current_height, gnd_height);
	}
	printf("---- UP ------\n");
	for(i=0;i<5*SAMPLE_RATE;i++){
		current_height += ASCENT_RATE * DT;
		sim_march(&f, current_height, gnd_height);
	}
	printf("---- DOWN ------\n");
	for(i=0;i<5*SAMPLE_RATE;i++){
		current_height -= ASCENT_RATE * DT;
		sim_march(&f, current_height, gnd_height);
	}

	// step & chill
	printf("---- -15m Step ------\n");
	gnd_height -= 15;
	for(i=0;i<1*SAMPLE_RATE;i++){
		sim_march(&f, current_height, gnd_height);
	}

	printf("---- DOWN ------\n");
	for(i=0;i<9*SAMPLE_RATE;i++){
		current_height -= ASCENT_RATE * DT;
		sim_march(&f, current_height, gnd_height);
	}
	printf("---- UP ------\n");
	for(i=0;i<5*SAMPLE_RATE;i++){
		current_height += ASCENT_RATE * DT;
		sim_march(&f, current_height, gnd_height);
	}

	return;
}

typedef struct pose_vel_6dof_t{
    uint32_t magic_number;         ///< Unique 32-bit number used to signal the beginning of a packet while parsing a data stream.
    int64_t timestamp_ns;          ///< Timestamp in clock_monotonic system time of the provided pose.
    float T_child_wrt_parent[3];   ///< Translation of the body with respect to parent frame in meters.
    float R_child_to_parent[3][3]; ///< Rotation matrix from body to parent frame.
    float v_child_wrt_parent[3];   ///< Velocity of the body with respect to the parent frame.
    float w_child_wrt_child[3];    ///< Angular velocity of the body about its X Y and Z axes respectively. Essentially filtered gyro values with internal biases applied.
} __attribute__((packed)) pose_vel_6dof_t;


static int _isfinite(double d)
{
	if(d<=DBL_MAX && d>=-DBL_MAX){
		return 1;
	}
	return 0;

}


int main(int argc, char* argv[])
{
	// check for options
	if(parse_opts(argc, argv)) return -1;

	// if no csv path was given, just run the fake data sim
	if(csv_path[0]==0){
		printf("no path given, running noise-free sim\n");
		sim();
		return 0;
	}

	printf("trying to open csv file: %s\n", csv_path);
	FILE* csv_fd = fopen(csv_path, "r");
	if(csv_fd==NULL){
		fprintf(stderr,"failed to open csv file: %s\n", csv_path);
		perror("error");
		return -1;
	}

	rc_alt_filter_t f = RC_ALT_FILTER_INITIALIZER;
	f.en_debug_prints = 1;
	rc_alt_filter_init(&f, 8.7);

	pose_vel_6dof_t p;
	int ret;
	int index;
	char* line;
	size_t buflen = 512;
	ssize_t line_len;
	int64_t ts_start = 0;
	int64_t ts = 0;
	float rpy[3];

	line = (char*)malloc(buflen);

	// drop the first line, that's the header
	line_len = getline(&line, &buflen, csv_fd);

	while(1){
		line_len = getline(&line, &buflen, csv_fd);
		if(line_len<0){
			printf("reached end of file\n");
			exit(0);
		}

		// scan data out of the line
		ret = sscanf(line, "%d,%ld,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",\
		&index, &ts,\
		&p.T_child_wrt_parent[0], &p.T_child_wrt_parent[1], &p.T_child_wrt_parent[2],\
		&rpy[0],&rpy[1],&rpy[2],\
		&p.v_child_wrt_parent[0], &p.v_child_wrt_parent[1], &p.v_child_wrt_parent[2],\
		&p.w_child_wrt_child[0], &p.w_child_wrt_child[1], &p.w_child_wrt_child[2]);

		// make sure all fields were populated
		if(ret!=14){
			fprintf(stderr, "failed to parse csv line read %d values\n", ret);
			perror("error:");
			return -1;
		}

		if(ts_start==0) ts_start = ts;

		// extract the bits we are interested in;
		//double ts_s = (double)(ts-ts_start)/1000000000.0;
		double scale = p.T_child_wrt_parent[0];
		double z_deriv = p.T_child_wrt_parent[1];
		double baro_hgt = p.T_child_wrt_parent[2];

		if(_isfinite(scale)){
			rc_alt_filter_add_flow(&f, scale, ts);
			printf("\n");
		}

		if(_isfinite(z_deriv)){
			rc_alt_filter_add_vel(&f, z_deriv, ts);
		}

		if(_isfinite(baro_hgt)){
			rc_alt_filter_add_baro(&f, baro_hgt, ts);
		}



	}




	return 0;
}
