/**
 * @headerfile rc_math.h <rc_math.h>
 *
 * @brief catch-all include for the RC math library functions
 *
 *
 * @author     James Strawson
 *
 * @date       1/19/2018
 */

#ifndef RC_MATH_H
#define RC_MATH_H

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

#include <rc_math/algebra.h>
#include <rc_math/filter.h>
#include <rc_math/kalman.h>
#include <rc_math/matrix.h>
#include <rc_math/other.h>
#include <rc_math/polynomial.h>
#include <rc_math/quaternion.h>
#include <rc_math/ring_buffer.h>
#include <rc_math/timestamp_filter.h>
#include <rc_math/timed_ringbuf.h>
#include <rc_math/vector.h>

#endif // RC_MATH_H
