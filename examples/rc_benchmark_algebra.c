/**
 * @file rc_benchmark_algebra.c
 * @example    rc_benchmark_algebra
 *
 * @brief      benchmarks the linear algebra functions to test floating point
 *             speed
 *
 *             This example prints the time to execute all functions and reports
 *             the speed of basic matrix multiplication in MFLOPS. It can be
 *             used as a test to see if the compiled math library is using the
 *             CPU hardware vectorized floating point units.
 *
 *
 * @author     James Strawson
 * @date       1/29/2018
 */

#define __USE_POSIX199309
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <time.h>
#include <stdlib.h> // for atoi
#include <rc_math.h>

#define DEFAULT_DIM 140
#define MIN_DIM     1
#define MAX_DIM     500

#define TIMER __nanos_thread_time()

// ns consumed just by reading the thread time
#define TIMER_DELAY 2100


static void __print_usage(void)
{
    printf("\n");
    printf("-d         use default matrix size (%dx%d)\n",DEFAULT_DIM,DEFAULT_DIM);
    printf("-s {size}  use custom matrix size\n");
    printf("-h         print this help message\n");
    printf("\n");
}

static uint64_t __nanos_thread_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return ((uint64_t)ts.tv_sec*1000000000)+ts.tv_nsec;
}

int main(int argc, char *argv[])
{
    int dim = 0;
    int c, diff;
    uint64_t t1, t2;
    rc_vector_t b = RC_VECTOR_INITIALIZER;
    rc_vector_t x = RC_VECTOR_INITIALIZER;
    rc_matrix_t A =  RC_MATRIX_INITIALIZER;
    rc_matrix_t AA =  RC_MATRIX_INITIALIZER;
    rc_matrix_t B =  RC_MATRIX_INITIALIZER;
    rc_matrix_t L =  RC_MATRIX_INITIALIZER;
    rc_matrix_t U =  RC_MATRIX_INITIALIZER;
    rc_matrix_t P =  RC_MATRIX_INITIALIZER;
    rc_matrix_t Q =  RC_MATRIX_INITIALIZER;
    rc_matrix_t R =  RC_MATRIX_INITIALIZER;

    // make sure user gave an argument
    if(argc>3){
        printf("Too many arguments given.\n");
        __print_usage();
        return -1;
    }
    if(argc<2){
        printf("Not enough arguments given.\n");
        __print_usage();
        return -1;
    }
    // parse arguments
    opterr = 0;
    while ((c = getopt(argc, argv, "ds:h")) != -1){
        switch (c){
        case 'd': // default size option
            if(dim!=0){
                printf("invalid combination of arguments\n");
                __print_usage();
                return -1;
            }
            dim = DEFAULT_DIM;
            break;
        case 's': // custom size option
            if(dim!=0){
                printf("invalid combination of arguments\n");
                __print_usage();
                return -1;
            }
            dim = atoi(optarg);
            if(dim>MAX_DIM || dim<MIN_DIM){
                printf("requested size out of bounds\n");
                __print_usage();
                return -1;
            }
            break;
        case 'h':
            __print_usage();
            return 0;
        default:
            printf("inavlid argument\n");
            __print_usage();
            return -1;
        }
    }

    printf("Starting single-threaded test\n");

    // create a random nxn matrix for later use
    t1 = TIMER;
    rc_matrix_random(&A,dim,dim);
    rc_vector_zeros(&b,dim);
    t2 = TIMER;
    diff = (int)((t2-t1-TIMER_DELAY)/(uint64_t)1000);
    printf("%10dus Time to make random matrix & vector\n", diff);

    // duplicate matrix
    t1 = TIMER;
    rc_matrix_duplicate(A,&AA);
    t2 = TIMER;
    diff = (int)((t2-t1-TIMER_DELAY)/(uint64_t)1000);
    printf("%10dus Time to duplicate matrix\n", diff);

    // Multiply matrices
    rc_matrix_alloc(&B,dim,dim);
    t1 = TIMER;
    rc_matrix_multiply(A, AA, &B);
    t2 = TIMER;
    diff = (int)((t2-t1-TIMER_DELAY)/(uint64_t)1000);
    printf("%10dus Time to multiply matrices\n", diff);

    // find determinant
    t1 = TIMER;
    rc_matrix_determinant(A);
    t2 = TIMER;
    diff = (int)((t2-t1-TIMER_DELAY)/(uint64_t)1000);
    printf("%10dus Time to find matrix determinant\n", diff);

    // find inverse
    t1 = TIMER;
    rc_algebra_invert_matrix(A, &AA);
    t2 = TIMER;
    diff = (int)((t2-t1-TIMER_DELAY)/(uint64_t)1000);
    printf("%10dus Time to invert matrix\n", diff);

    // LUP
    rc_matrix_alloc(&L,dim,dim);
    rc_matrix_alloc(&U,dim,dim);
    rc_matrix_alloc(&P,dim,dim);
    t1 = TIMER;
    rc_algebra_lup_decomp(A,&L,&U,&P);
    t2 = TIMER;
    diff = (int)((t2-t1-TIMER_DELAY)/(uint64_t)1000);
    printf("%10dus Time to do LUP decomposition\n", diff);

    // do a QR decomposition on A
    rc_matrix_alloc(&Q,dim,dim);
    rc_matrix_alloc(&R,dim,dim);
    t1 = TIMER;
    rc_algebra_qr_decomp(A,&Q,&R);
    t2 = TIMER;
    diff = (int)((t2-t1-TIMER_DELAY)/(uint64_t)1000);
    printf("%10dus Time to do QR decomposition\n", diff);

    // do a QR decomposition on A
    rc_vector_alloc(&x,dim);
    t1 = TIMER;
    rc_algebra_lin_system_solve(A,b,&x);
    t2 = TIMER;
    diff = (int)((t2-t1-TIMER_DELAY)/(uint64_t)1000);
    printf("%10dus Time to solve linear system\n", diff);

    // Multiply matrices 1000 times
    rc_matrix_alloc(&B,dim,dim);
    rc_matrix_random(&A,dim,dim);
    rc_matrix_random(&AA,dim,dim);
    t1 = TIMER;
    for (int i=0;i<1000;i++){
        rc_matrix_multiply(A, AA, &B);
    }
    t2 = TIMER;
    diff = (int)((t2-t1-TIMER_DELAY)/(uint64_t)1000);
    printf("%10dus Time to multiply matrices 1000 times\n", diff);

    // calculate floating pointer operations per second, both multiplication
    // and addition count as operations, hence multiply by 2
    uint64_t flops = ((uint64_t)2*dim*dim*dim*1000000000)/(diff);
    double gflops = (double)flops/1000000000.0;
    printf("     %7.3f GFLOPS multiplying matrices 1000 times\n", gflops);

    printf("DONE\n");
    //rc_set_cpu_freq(FREQ_ONDEMAND);
    return 0;
}
