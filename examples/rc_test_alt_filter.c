

#include <stdio.h>
#include <rc_math/alt_filter.h>



#define SAMPLE_RATE 10
#define DT (1.0/SAMPLE_RATE)
#define ASCENT_RATE 2.0


static void march(rc_alt_filter_t* f, double current_height, double gnd_height)
{
	static int64_t timestamp = 0;
	timestamp += DT*1000000000;

	// put current height straight in as barometer data
	rc_alt_filter_add_baro(f, current_height, timestamp);

	// calculate scale
	static double last_range = 0;
	double current_range = current_height - gnd_height;
	double scale = last_range/current_range;
	printf("scale: %5.2f ", scale);
	last_range = current_range;

	printf("truth: %5.1f ", current_range);

	// add scale to filter
	rc_alt_filter_add_flow(f, scale, timestamp);

	double error = f->last_output - current_range;


	printf("out: %7.2f ", f->last_output);
	printf("err: %7.2f ", error);
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
	f.en_debug_prints = 0;
	rc_alt_filter_init(&f, SAMPLE_RATE);

	double gnd_height = 0;
	double current_height = 0.0;

	print_header();

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

	print_header();

	return 0;
}
