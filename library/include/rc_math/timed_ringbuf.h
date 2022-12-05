


#ifndef RC_TIMED_RINGBUF_H
#define RC_TIMED_RINGBUF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>



typedef struct rc_timed_ringbuf_t {
	double* d;		///< pointer to dynamically allocated data
	int64_t* t;		///< point to timestamp data
	int size;			///< number of elements the buffer can hold
	int64_t forward_limit; ///< max nanoseconds into the future to predict, default 2e8 (0.2s)
	int index;			///< index of the most recently added value
	int items_in_buf;	///< number of items in the buffer, between 0 and size
	int initialized;	///< flag indicating if memory has been allocated for the buffer
	pthread_mutex_t mutex; ///< this gets locked and unlocked by almost every function here
} rc_timed_ringbuf_t;


/**
 * initializer for the rc_timed_ringbuf_t to make sure it starts zero'd out
 */
#define RC_TIMED_RINGBUF_INITIALIZER {\
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
 * rc_timed_ringbuf_t struct.
 *
 * If buf is already the right size then it is left untouched. Otherwise any
 * existing memory allocated for buf is freed to avoid memory leaks and new
 * memory is allocated.
 *
 * @param      buf   Pointer to user's buffer
 * @param[in]  size  Number of elements to allocate space for
 *
 * @return     Returns 0 on success or -1 on failure.
 */
int rc_timed_ringbuf_alloc(rc_timed_ringbuf_t* buf, int size);


/**
 * @brief      Frees the memory allocated for a buffer.
 *
 * Also set the initialized flag to 0 so other functions don't try to access
 * unallocated memory.
 *
 * @param      buf   Pointer to user's buffer
 *
 * @return     Returns 0 on success or -1 on failure.
 */
int rc_timed_ringbuf_free(rc_timed_ringbuf_t* buf);


/**
 * @brief      return an empty uninitialized rc_timed_ringbuf_t with defaults
 */
rc_timed_ringbuf_t rc_timed_ringbuf_empty(void);


/**
 * @brief      Puts a new value into the ring buffer and updates the index
 * accordingly.
 *
 * If the buffer was full then the oldest value in the buffer is automatically
 * removed.
 *
 * @param      buf   Pointer to user's buffer
 * @param      ts    timestamp
 * @param[in]  val   The value to be inserted
 *
 * @return     Returns 0 on success or -1 on failure.
 */
int rc_timed_ringbuf_insert(rc_timed_ringbuf_t* buf, int64_t ts_ns, double val);


/**
 * @brief      return the position in the buffer of the entry at or immediately
 *             before a specified timestamp;
 *
 * @param      buf    The buffer
 * @param[in]  ts_ns  timestamp
 *
 * @return     requested position, or -1 on error, -2 if not found
 */
int rc_timed_ringbuf_get_pos_b4_ts(rc_timed_ringbuf_t* buf, int64_t ts_ns);

/**
 * @brief      fetch the value and timestamp which is 'position' steps behind
 *             the last value added to the buffer.
 *
 *             If 'position' is given as 0 then the most recent entry is
 *             returned. The position obviously can't be larger than size-1.
 *             This will also check and silently return -2 if the buffer hasn't
 *             been filled up enough to go back that far in time.
 *
 * @param      buf       The buffer
 * @param[in]  position  The position
 * @param      ts        The result
 *
 * @return     0 on success, -1 on generic error, -2 if the buffer does not
 *             contain enough data yet
 */
int rc_timed_ringbuf_get_ts_at_pos(rc_timed_ringbuf_t* buf, int position, int64_t* ts);

/**
 * @brief      fetch the value which is 'position' steps behind the last value
 *             added to the buffer.
 *
 *             If 'position' is given as 0 then the most recent entry is
 *             returned. The position obviously can't be larger than size-1.
 *             This will also check and silently return -2 if the buffer hasn't
 *             been filled up enough to go back that far in time.
 *
 * @param      buf       The buffer
 * @param[in]  position  The position
 * @param      result    The result
 * @param      val   The result
 *
 * @return     0 on success, -1 on generic error, -2 if the buffer does not
 *             contain enough data yet
 */
int rc_timed_ringbuf_get_val_at_pos(rc_timed_ringbuf_t* buf, int position, double* val);


/**
 * @brief      fetches a value at a requested timestamp using linear
 *             imterpolation.
 *
 *             If the requested timestamp is up to buf.forward_limit nanoseconds
 *             ahead of the most recently entry in the buffer then it will
 *             forward predict a future value using the most recent two. Set
 *             buf.forward_limit to 0 to disable this feature. Feel free to
 *             adjust it to suite your applications.
 *
 * @param      buf    The buffer
 * @param[in]  ts_ns  The ts ns
 * @param      val    The value
 *
 * @return     0 on success. -1 on general error. -2 if the buffer doesn't contain enough data.
 */
int rc_timed_ringbuf_get_val_at_time(rc_timed_ringbuf_t* buf, int64_t ts_ns, double* val);



/**
 * @brief      integrate the signal in the ringbuf between two given times
 *
 * NOTE this is originally for integrating high-rate gyro data over long periods
 * where +- a sample is negligible. As such, it integrates from the gyro sample
 * immediately after or at t1 to the sample immediately after or at t2.
 *
 * This uses trapezoidal integration.
 *
 * @param      buf       The buffer
 * @param[in]  t1        start time (older than t2)
 * @param[in]  t2        end time (after t1)
 * @param      integral  output integral
 *
 * @return     0 on success, -1 on general error, -2 if sample at t1 was not found, -3 if sample at t2 was not found
 */
int rc_timed_ringbuf_integrate_over_time(rc_timed_ringbuf_t* buf, int64_t t_start, int64_t t_end, double* integral);


/**
 * @brief      copy out the n most recent samples into contiguous memory.
 *
 *             Make sure you've allocated enough space in the output buffer.
 *             Data is copied out in the same order it is stored in the buffer
 *             which is oldest at the start (left), newest at the end (right).
 *
 *             This is useful, for example, if you want to run an FFT on a
 *             subset of a larger buffer of data.
 *
 * @param      buf   The buffer
 * @param[in]  n     number of entries to copy out, must be between 1 and the
 *                   buffer size inclusive.
 * @param      out   The out
 *
 * @return     0 on success, -1 on failure
 */
int rc_timed_ringbuf_copy_out_n_newest(rc_timed_ringbuf_t* buf, int n, double* out);



#ifdef __cplusplus
}
#endif

#endif // RC_TIMED_RINGBUF_H

