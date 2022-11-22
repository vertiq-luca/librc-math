

#include <stdio.h>
#include <rc_math/alt_filter.h>



#define SAMPLE_RATE 10
#define DT (1.0/SAMPLE_RATE)
#define ASCENT_RATE 2.0


static void march(rc_alt_filter_t* f, double current_height, double gnd_height)
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

	printf("scale: %4.2f ", scale);
	last_range = current_range;

	printf("truth: %4.1f ", current_range);

	// add scale to filter
	rc_alt_filter_add_flow(f, scale, timestamp);

	double error = f->last_output - current_range;

	printf(" err: %5.2f ", error);
	printf("\n");
	return;
}

static void print_header(void){

	printf("ctr,ok,   baro,  scale,    gnd, output,   err\n");

}


int main()
{
	int i;
	rc_alt_filter_t f = RC_ALT_FILTER_INITIALIZER;
	f.en_debug_prints = 1;
	rc_alt_filter_init(&f, SAMPLE_RATE);

	double gnd_height = 0;
	double current_height = 0.0;

	//print_header();

	// chill
	for(int i=0;i<0.5*SAMPLE_RATE;i++){
		march(&f, current_height, gnd_height);
	}

	// ascend
	printf("---- START ASCENDING ------\n");
	for(i=0;i<10*SAMPLE_RATE;i++){
		current_height += ASCENT_RATE * DT;
		march(&f, current_height, gnd_height);
	}

	// chill
	printf("---- WAIT ------\n");
	for(i=0;i<1*SAMPLE_RATE;i++){
		march(&f, current_height, gnd_height);
	}

	// step & chill
	printf("---- 5m Step ------\n");
	gnd_height += 5;
	for(i=0;i<1*SAMPLE_RATE;i++){
		march(&f, current_height, gnd_height);
	}

	printf("---- DOWN ------\n");
	for(i=0;i<5*SAMPLE_RATE;i++){
		current_height -= ASCENT_RATE * DT;
		march(&f, current_height, gnd_height);
	}
	printf("---- UP ------\n");
	for(i=0;i<5*SAMPLE_RATE;i++){
		current_height += ASCENT_RATE * DT;
		march(&f, current_height, gnd_height);
	}
	printf("---- DOWN ------\n");
	for(i=0;i<5*SAMPLE_RATE;i++){
		current_height -= ASCENT_RATE * DT;
		march(&f, current_height, gnd_height);
	}

	// step & chill
	printf("---- -15m Step ------\n");
	gnd_height -= 15;
	for(i=0;i<1*SAMPLE_RATE;i++){
		march(&f, current_height, gnd_height);
	}

	printf("---- DOWN ------\n");
	for(i=0;i<9*SAMPLE_RATE;i++){
		current_height -= ASCENT_RATE * DT;
		march(&f, current_height, gnd_height);
	}
	printf("---- UP ------\n");
	for(i=0;i<5*SAMPLE_RATE;i++){
		current_height += ASCENT_RATE * DT;
		march(&f, current_height, gnd_height);
	}

	//print_header();

	return 0;
}
