

#include <stdio.h>
#include <rc_math/timed_ringbuf.h>

#define SIZE	5			// short buffer so we can test wrapping easily
#define DT_NS	100000000	// 100ms


static void _print_array(int n, double* d){
	for(int i=0;i<n;i++){
		printf("%2d %4.1f\n",i,d[i]);
	}
	printf("\n");
	return;
}


int main()
{
	int i, ret;
	double inval = 0.0;
	int64_t intime = 0;
	double outval;
	int64_t outtime;

	rc_timed_ringbuf_t b = RC_TIMED_RINGBUF_INITIALIZER;
	rc_timed_ringbuf_alloc(&b, SIZE);

	// start with a partial fill and test stuff
	for(i=0;i<2;i++){
		inval += 1.0;
		intime += DT_NS;
		rc_timed_ringbuf_insert(&b,intime, inval);
	}

	// read out those 2 values and try to get an older one to check the error
	for(i=0;i<3;i++){
		ret = rc_timed_ringbuf_get_ts_at_pos(&b, i, &outtime);
		printf("ret: %3d pos: %2d, ts_s: %4.1f\n", ret, i, (double)outtime/1000000000.0);
		ret = rc_timed_ringbuf_get_val_at_pos(&b, i, &outval);
		printf("ret: %3d pos: %2d, val: %4.1f\n", ret, i, outval);
	}

	printf("test mean\n");
	for(i=0;i<4;i++){
		ret = rc_timed_ringbuf_mean(&b, i, &outval);
		printf("ret: %3d n: %2d, val: %4.1f\n", ret, i, outval);
	}


	// fill the rest and test the looparound
	for(i=0;i<SIZE;i++){
		inval += 1.0;
		intime += DT_NS;
		rc_timed_ringbuf_insert(&b,intime, inval);
	}

	// read out those 2 values and try to get an older one to check the error
	for(i=0;i<SIZE+1;i++){
		ret = rc_timed_ringbuf_get_ts_at_pos(&b, i, &outtime);
		printf("ret: %3d pos: %2d, ts_s: %4.1f\n", ret, i, (double)outtime/1000000000.0);
		ret = rc_timed_ringbuf_get_val_at_pos(&b, i, &outval);
		printf("ret: %3d pos: %2d, val: %4.1f\n", ret, i, outval);
	}

	printf("test interpolation\n");
	ret = rc_timed_ringbuf_get_val_at_time(&b, 650000000, &outval);
	printf("ret: %3d val: %4.1f\n", ret, outval);
	ret = rc_timed_ringbuf_get_val_at_time(&b, 700000000, &outval);
	printf("ret: %3d val: %4.1f\n", ret, outval);
	ret = rc_timed_ringbuf_get_val_at_time(&b, 750000000, &outval);
	printf("ret: %3d val: %4.1f\n", ret, outval);

	printf("test integration\n");
	rc_timed_ringbuf_integrate_over_time(&b, 400000000, 700000000, &outval);
	printf("ret: %3d val: %4.1f\n", ret, outval);

	printf("copy out everything into contiguous memory\n");
	double all[SIZE];
	rc_timed_ringbuf_copy_out_n_newest(&b, SIZE-1, all);
	_print_array(SIZE-1, all);
	rc_timed_ringbuf_copy_out_n_newest(&b, SIZE, all);
	_print_array(SIZE, all);

	printf("test mean\n");
	for(i=1;i<=SIZE;i++){
		ret = rc_timed_ringbuf_mean(&b, i, &outval);
		printf("ret: %3d n: %2d, val: %4.1f\n", ret, i, outval);
	}

	printf("test std dev\n");
	for(i=1;i<=SIZE;i++){
		ret = rc_timed_ringbuf_std_dev(&b, i, &outval);
		printf("ret: %3d n: %2d, val: %4.1f\n", ret, i, outval);
	}


	return 0;
}
