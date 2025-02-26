/**
 * @headerfile other.h <rc_math/other.h>
 *
 * @brief      Math functions that don't fit elsewhere.
 *
 * @author     James Strawson
 * @date       2022
 *
 * @addtogroup Other_Math
 * @ingroup Math
 * @{
 */


#ifndef RC_MATH_OTHER_H
#define RC_MATH_OTHER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Returns a random floating point number between -1 and 1.
 *
 * Uses standard C rand function and bitwise operations which is much faster
 * than doing floating point arithmetic.
 *
 * @return     random floating point number between -1 and 1
 */
float rc_get_random_float(void);

/**
 * @brief      Returns a random double-precision floating point number between
 * -1 and 1.
 *
 * Uses standard C rand function and bitwise operations which is much faster
 * than doing floating point arithmetic.
 *
 * @return     random double-precision floating point number between -1 and 1
 */
double rc_get_random_double(void);

/**
 * @brief      Modifies val to be bounded between between min and max.
 *
 * @param      val   The value to be checked and possibly modified
 * @param[in]  min   The lower bound
 * @param[in]  max   The upper bound
 *
 * @return     Returns 1 if saturation occurred, 0 if val was already in bound,
 * and -1 if min was falsely larger than max.
 */
int rc_saturate_float(float* val, float min, float max);

/**
 * @brief      Modifies val to be bounded between between min and max.
 *
 * @param      val   The value to be checked and possibly modified
 * @param[in]  min   The lower bound
 * @param[in]  max   The upper bound
 *
 * @return     Returns 1 if saturation occurred, 0 if val was already in bound,
 * and -1 if min was falsely larger than max.
 */
int rc_saturate_double(double* val, double min, double max);


/**
 * @brief      return current monotonic time in nanoseconds
 *
 * @return     current monotonic time in nanoseconds
 */
int64_t rc_time_monotonic_ns(void);
int64_t rc_time_realtime_ns(void);

void rc_nanosleep(int64_t ns);
int rc_loop_sleep(double rate_hz, int64_t* next_time);



#ifdef __cplusplus
}
#endif

#endif // RC_MATH_OTHER_H

/** @}  end group math*/
