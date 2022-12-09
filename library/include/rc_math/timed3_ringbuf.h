/**
 * @headerfile timed3_ringbuf.h <rc_math/timed3_ringbuf.h>
 *
 * @brief      Ring buffer implementation for timestamped triplets of doubles.
 *             Good for sensor data, particularly gyro or accelerometer.
 *
 *             See timed_ringbuf.h for a general description. This is mostly the
 *             same but operates on triplets. Data is stored in the buffer as
 *             xyzxyzxyz...
 *
 * @author     James Strawson
 * @date       2022
 *
 * @addtogroup Timed3 Ringbuf
 * @ingroup    Math @{
 */


#ifndef RC_TIMED3_RINGBUF_H
#define RC_TIMED3_RINGBUF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include <rc_math/matrix.h>


typedef struct rc_timed3_ringbuf_t {
	double* d;			///< pointer to dynamically allocated data
	int64_t* t;			///< point to timestamp data
	int size;			///< number of elements the buffer can hold
	int64_t forward_limit; ///< max nanoseconds into the future to predict, default 2e8 (0.2s)
	int index;			///< index of the most recently added value
	int items_in_buf;	///< number of items in the buffer, between 0 and size
	int initialized;	///< flag indicating if memory has been allocated for the buffer
	pthread_mutex_t mutex; ///< this gets locked and unlocked by almost every function here
} rc_timed3_ringbuf_t;


/**
 * initializer for the rc_timed3_ringbuf_t to make sure it starts zero'd out
 */
#define RC_TIMED3_RINGBUF_INITIALIZER {\
	.d = NULL,\
	.t = 0,\
	.size = 0,\
	.forward_limit = 200000000,\
	.index = 0,\
	.items_in_buf = 0,\
	.initialized = 0,\
	.mutex = PTHREAD_MUTEX_INITIALIZER\
}


/**
 * @brief      Allocates memory for a ring buffer and initializes an
 *             rc_timed_ringbuf_t struct.
 *
 *             buffer length must be >=2
 *
 *             This will throw an error if the buffer is already initialized or
 *             not in a clean state. Use RC_TIMED3_RINGBUF_INITIALIZER or
 *             rc_timed3_ringbuf_empty() to ensure you start with a clean struct.
 *
 * @param[in]  buf   Pointer to user's buffer to initialize
 * @param[in]  size  Number of elements to allocate space for
 *
 * @return     Returns 0 on success or -1 on failure.
 */
int rc_timed3_ringbuf_alloc(rc_timed3_ringbuf_t* buf, int size);


/**
 * @brief      Frees the memory allocated for a buffer.
 *
 *             Also set the initialized flag to 0 so other functions don't try
 *             to access unallocated memory.
 *
 * @param      buf   Pointer to user's buffer
 *
 * @return     Returns 0 on success or -1 on failure.
 */
int rc_timed3_ringbuf_free(rc_timed3_ringbuf_t* buf);


/**
 * @brief      return an empty uninitialized rc_timed3_ringbuf_t with defaults
 */
rc_timed3_ringbuf_t rc_timed3_ringbuf_empty(void);


/**
 * @brief      Puts a new triplet value into the ring buffer and updates the
 *             index accordingly. val should point to an array of 3 doubles
 *
 *             If the buffer was full then the oldest value in the buffer is
 *             automatically removed.
 *
 * @param[in]  buf    Pointer to user's buffer
 * @param[in]  ts_ns  timestamp in nanoseconds
 * @param[in]  val    pointer to 3 doubles to be inserted
 *
 * @return     Returns 0 on success or -1 on failure.
 */
int rc_timed3_ringbuf_insert(rc_timed3_ringbuf_t* buf, int64_t ts_ns, double* val);


/**
 * @brief      fetch the value and timestamp which is 'position' steps behind
 *             the last value added to the buffer.
 *
 *             If 'position' is given as 0 then the most recent entry is
 *             returned. The position obviously can't be larger than size-1.
 *             This will also check and silently return -2 if the buffer hasn't
 *             been filled up enough to go back that far in time.
 *
 *             on error this will also set ts to -1 so assuming you are only
 *             using positive timestamps you can check just the contents of ts
 *             instead of checking the return value and ts.
 *
 * @param[in]  buf       The buffer
 * @param[in]  position  The position
 * @param[out] ts        The result
 *
 * @return     0 on success, -1 on generic error, -2 if the buffer does not
 *             contain enough data yet
 */
int rc_timed3_ringbuf_get_ts_at_pos(rc_timed3_ringbuf_t* buf, int position, int64_t* ts);


/**
 * @brief      fetch the value which is 'position' steps behind the last value
 *             added to the buffer.
 *
 *             If 'position' is given as 0 then the most recent entry is
 *             returned. The position obviously can't be larger than size-1.
 *             This will also check and silently return -2 if the buffer hasn't
 *             been filled up enough to go back that far in time.
 *
 *             Make sure val points to enough space for an array of 3 doubles
 *
 * @param[in]  buf       The buffer
 * @param[in]  position  The position
 * @param[out] val       The result
 *
 * @return     0 on success, -1 on generic error, -2 if the buffer does not
 *             contain enough data yet
 */
int rc_timed3_ringbuf_get_val_at_pos(rc_timed3_ringbuf_t* buf, int position, double* val);


/**
 * @brief      return the position in the buffer of the entry at or immediately
 *             before a specified timestamp
 *
 * @param[in]  buf    The buffer
 * @param[in]  ts_ns  timestamp
 *
 * @return     requested position, or -1 on error, -2 if not found
 */
int rc_timed3_ringbuf_get_pos_b4_ts(rc_timed3_ringbuf_t* buf, int64_t ts_ns);


/**
 * @brief      fetches a value at a requested timestamp using linear
 *             interpolation. Each of the 3 values are considered independent
 *             signals.
 *
 *             If the requested timestamp is up to buf.forward_limit nanoseconds
 *             ahead of the most recently entry in the buffer then it will
 *             forward predict a future value using the most recent two. Set
 *             buf.forward_limit to 0 to disable this feature. Feel free to
 *             adjust it to suite your applications.
 *
 * @param[in]  buf    The buffer
 * @param[in]  ts_ns  Timestamp
 * @param[out] val    The value
 *
 * @return     0 on success. -1 on general error. -2 if the buffer doesn't
 *             contain enough data.
 */
int rc_timed3_ringbuf_get_val_at_time(rc_timed3_ringbuf_t* buf, int64_t ts_ns, double* val);


/**
 * @brief      integrate all 3 signals in the ringbuf between two given times
 *
 *             NOTE this is originally for integrating high-rate gyro data over
 *             long periods where +- a sample is negligible. As such, it
 *             integrates from the gyro sample immediately before or at t1 to
 *             the sample immediately before or at t2.
 *
 *             This uses trapezoidal integration.
 *
 * @param[in]  buf       The buffer
 * @param[in]  t_start   The t start
 * @param[in]  t_end     The t end
 * @param[out] integral  output integral, must have enough space for 3 doubles
 *
 * @return     0 on success, -2 if the buffer does not contain data old enough,
 *             -3 if the buffer does not contain data new enough. -1 on other
 *             error.
 */
int rc_timed3_ringbuf_integrate_over_time(rc_timed3_ringbuf_t* buf, \
							int64_t t_start, int64_t t_end, double* integral);


/**
 * @brief      calculate the mean of the last n samples
 *
 *
 * @param[in]  buf   The buffer
 * @param[in]  n     number of samples to use for the calculation
 * @param[out] out   pointer to write the answer out to
 *
 * @return     0 on success, -1 on failure
 */
int rc_timed3_ringbuf_mean(rc_timed3_ringbuf_t* buf, int n, double* out);


/**
 * @brief      calculate the standard deviation of the last n samples
 *
 *
 * @param[in]  buf   The buffer
 * @param[in]  n     number of samples to use for the calculation
 * @param[out] out   pointer to write the answer out to
 *
 * @return     0 on success, -1 on failure
 */
int rc_timed3_ringbuf_std_dev(rc_timed3_ringbuf_t* buf, int n, double* out);


/**
 * @brief      assuming the triplet ringbuf contains XYZ gyroscope data in rad/s
 *             this function calculates the rotation of the body in 3d between
 *             two points in time. This is intended for things like EIS and
 *             gyro-assisted feature tracking.
 *
 *             the output matrix will be allocated or reallocated as necessary
 *             to hold the resulting 3x3 rotation matrix.
 *
 * @param[in]  buf      The buffer
 * @param[in]  t_start  start time nanoseconds
 * @param[in]  t_end    end time nanoseconds
 * @param[out] out      pointer to a matrix to write the answer out to
 *
 * @return     0 on success, -2 if the buffer does not contain data old enough,
 *             -3 if the buffer does not contain data new enough. -1 on other
 *             error.
 */
int rc_timed3_ringbuf_integrate_gyro_3d(rc_timed3_ringbuf_t* buf, \
							int64_t t_start, int64_t t_end, rc_matrix_t* out);


#ifdef __cplusplus
}
#endif

#endif // RC_TIMED3_RINGBUF_H

