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
#include <math.h>   // for M_PI
#include <rc_math.h>

#define SAMPLE_RATE 50
#define TIME_CONSTANT   2.0

static int running = 0;

// interrupt handler to catch ctrl-c
static void __signal_handler(__attribute__ ((unused)) int dummy)
{
    running=0;
    return;
}

static void __nanosleep(uint64_t ns){
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

int main()
{
    rc_filter_t low_pass    = RC_FILTER_INITIALIZER; // structs that hold buffers and transfer function coeffs
    rc_filter_t high_pass   = RC_FILTER_INITIALIZER;
    rc_filter_t integrator  = RC_FILTER_INITIALIZER;
    rc_filter_t lp_butter   = RC_FILTER_INITIALIZER;
    rc_filter_t hp_butter   = RC_FILTER_INITIALIZER;
    rc_filter_t notch   = RC_FILTER_INITIALIZER;

    const double dt = 1.0/SAMPLE_RATE;
    double lp,hp,i,u,lpb,hpb,nch;
    double time = 0;
    int step = 0;
    double sin_freq = M_PI * 2;

    printf("\nSample Rate: %dhz\n", SAMPLE_RATE);
    printf("Time Constant: %5.2f\n", TIME_CONSTANT);

    rc_filter_first_order_lowpass(&low_pass, dt, TIME_CONSTANT);
    rc_filter_first_order_highpass(&high_pass, dt, TIME_CONSTANT);
    rc_filter_integrator(&integrator, dt);
    rc_filter_butterworth_lowpass(&lp_butter, 2, dt, 2.0*M_PI/TIME_CONSTANT);
    rc_filter_butterworth_highpass(&hp_butter, 2, dt, 2.0*M_PI/TIME_CONSTANT);
    int worked = rc_filter_bandstop(&notch, 3 ,dt, 2 * M_PI, 3, 40);
    printf("notch worked?: %i", worked);

    printf("\nLow Pass:\n");
    rc_filter_print(low_pass);
    printf("\nHigh Pass:\n");
    rc_filter_print(high_pass);
    printf("\nIntegrator:\n");
    rc_filter_print(integrator);
    printf("\nLow Pass Butterworth:\n");
    rc_filter_print(lp_butter);
    printf("\nHigh Pass Butterworth:\n");
    rc_filter_print(hp_butter);
    printf("\nNotch:\n");
    rc_filter_print(notch);
    printf("\n\n");

    // print header
    printf("   time   |");
    printf("   step   |");
    printf("  input u |");
    printf("   notch  |");
    printf("  lowpass |");
    printf(" highpass |");
    printf("complement|");
    printf("integrator|");
    printf(" lp_butter|");
    printf("hp_butter |");
    
    printf("\n");

    // set signal handler so the loop can exit cleanly
    signal(SIGINT, __signal_handler);
    running = 1;

    // start filter input at 1, the loop will toggle this later
    u=1.0;

    // Keep Running until program state changes to 0
    while(running){
        // march all filters one step forward with u as the common input.
        // new outputs saved as lp,hp,and i. complement is lp+hp
        lp = rc_filter_march(&low_pass, u);
        hp = rc_filter_march(&high_pass, u);
        i  = rc_filter_march(&integrator, u);
        lpb = rc_filter_march(&lp_butter, u);
        hpb = rc_filter_march(&hp_butter, u);
        nch = rc_filter_march(&notch, u);

        printf("\r");
        printf("%8.3f  |", time);
        printf("     %i    |", step);
        printf("%8.3f  |", u);
        printf("%8.3f  |", nch);
        printf("%8.3f  |", lp);
        printf("%8.3f  |", hp);
        printf("%8.3f  |", lp+hp);
        printf("%8.3f  |", i);
        printf("%8.3f  |", lpb);
        printf("%8.3f  |", hpb);
        
        fflush(stdout);

        // toggle u between 0 and 1 every 10 seconds
        time += dt;
        u = sin(time * sin_freq);
        if ((int) time % 20 >= 15)
        {
            step = 3;
        } else if ((int) time % 20 >= 10)
        {
            step = 2;
        } else if ((int) time % 20 >= 5)
        {
            step = 1;
        } else
        {
            step = 0;
        }


        switch (step)
        {
        case 0:
            sin_freq = 2 * M_PI;
            update_stop_wc(&notch, 2 * M_PI, 3, 40);
            break;
        case 1:
            sin_freq = M_PI;
            update_stop_wc(&notch, 2 * M_PI, 3, 40);
            break;
        case 2:
            sin_freq = M_PI;
            update_stop_wc(&notch, M_PI, 3, 40);
            break;
        case 3:
            sin_freq = 2 * M_PI;
            update_stop_wc(&notch, M_PI, 3, 40);
            break;
        default:
            break;
        }
        // if(counter >= SAMPLE_RATE*10){
        //     counter = 0.0;
        //     if(u>0.0) u = 0.0;
        //     else u = 1.0;
        // }

        // sleep enough for a rough timed loop
        __nanosleep(1000000000/SAMPLE_RATE);
    }

    printf("\n");
    return 0;
}
