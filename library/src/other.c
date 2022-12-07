/**
 * @file       math/other.c
 *
 * @brief      general low-level math functions that don't fit elsewhere
 *
 * @author     James Strawson
 * @date       2016
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <rc_math/other.h>
#include "algebra_common.h"


typedef union {
    uint32_t i;
    float f;
}rc_int_float_t;

float rc_get_random_float(void)
{
    rc_int_float_t new;
    // get random 32-bit int, mask out all but the mantissa (right 23 bits)
    // with the & operator, then set the leftmost exponent bit to scale it
    new.i = (rand()&0x007fffff) | 0x40000000;
    return new.f - 3.0f; // convert to float and shift to range from -1 to 1
}


typedef union {
    uint64_t i;
    double d;
}rc_int_double_t;

double rc_get_random_double(void)
{
    rc_int_double_t new;
    // get random 64-bit int, mask out all but the mantissa (right 52 bits)
    // with the & operator, then set the leftmost exponent bit to scale it
    new.i = ((((uint64_t)rand()<<32)|rand()) & 0x000fffffffffffff) | 0x4000000000000000;
    return new.d - 3.0; // convert to double and shift to range from -1 to 1
}

int rc_saturate_float(float* val, float min, float max)
{
    // sanity checks
    if(unlikely(min>max)){
        fprintf(stderr,"ERROR: in rc_saturate_float, min must be less than max\n");
        return -1;
    }
    // bound val
    if(*val>max){
        *val = max;
        return 1;
    }else if(*val<min){
        *val = min;
        return 1;
    }
    return 0;
}

int rc_saturate_double(double* val, double min, double max)
{
    // sanity checks
    if(unlikely(min>max)){
        fprintf(stderr,"ERROR: in rc_saturate_double, min must be less than max\n");
        return -1;
    }
    // bound val
    if(*val>max){
        *val = max;
        return 1;
    }else if(*val<min){
        *val = min;
        return 1;
    }
    return 0;
}

int64_t rc_time_monotonic_ns()
{
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts)){
        fprintf(stderr,"ERROR calling clock_gettime\n");
        return -1;
    }
    return (int64_t)ts.tv_sec*1000000000 + (int64_t)ts.tv_nsec;
}


int64_t rc_time_realtime_ns()
{
    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME, &ts)){
        fprintf(stderr,"ERROR calling clock_gettime\n");
        return -1;
    }
    return (int64_t)ts.tv_sec*1000000000 + (int64_t)ts.tv_nsec;
}



void rc_nanosleep(int64_t ns){
    if(ns<=0) return;
    struct timespec req,rem;
    req.tv_sec = ns/1000000000;
    req.tv_nsec = ns%1000000000;
    // loop untill nanosleep sets an error or finishes successfully
    errno=0; // reset errno to avoid false detection
    while(nanosleep(&req, &rem) && errno==EINTR){
        req.tv_sec = rem.tv_sec;
        req.tv_nsec = rem.tv_nsec;
    }
    return;
}


int rc_loop_sleep(double rate_hz, int64_t* next_time)
{
    int64_t current_time = rc_time_monotonic_ns();

    // static variable so we remember when we last woke up
    if(*next_time<=0){
        *next_time = current_time;
    }

    // try to maintain output data rate
    *next_time += (1000000000.0/rate_hz);

    // uh oh, we fell behind, warn and get back on track
    if(*next_time<=current_time){
        //fprintf(stderr, "WARNING my_loop_sleep fell behind\n");
        return -1;
    }

    rc_nanosleep(*next_time-current_time);
    return 0;
}
