


#ifndef RC_TIMED_RINGBUF_H
#define RC_TIMED_RINGBUF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>

typedef struct rc_ts_dbl_t {
	int64_t ts_ns;
	double val;
} rc_ts_dbl_t;



typedef struct rc_timed_ringbuf_t {
	rc_ts_dbl_t* d;		///< pointer to dynamically allocated data
	int size;			///< number of elements the buffer can hold
	int64_t forward_limit; ///< max nanoseconds into the future to predict, default 2e8 (0.2s)
	int index;			///< index of the most recently added value
	int items_in_buf;	///< number of items in the buffer, between 0 and size
	int initialized;	///< flag indicating if memory has been allocated for the buffer
	pthread_mutex_t mutex;
} rc_timed_ringbuf_t;


/**
 * initializer for the rc_timed_ringbuf_t to make sure it starts zero'd out
 */
#define RC_TIMED_RINGBUF_INITIALIZER {\
	.d = NULL,\
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
 * @param      result    The result
 *
 * @return     0 on success, -1 on generic error, -2 if the buffer does not
 *             contain enough data yet
 */
int rc_timed_ringbuf_get_entry_at_pos(rc_timed_ringbuf_t* buf, int position, rc_ts_dbl_t* result);

/**
 * @brief      fetch the value which is 'position' steps behind
 *             the last value added to the buffer.
 *
 *             If 'position' is given as 0 then the most recent entry is
 *             returned. The position obviously can't be larger than size-1.
 *             This will also check and silently return -2 if the buffer hasn't
 *             been filled up enough to go back that far in time.
 *
 * @param      buf       The buffer
 * @param[in]  position  The position
 * @param      result    The result
 *
 * @return     0 on success, -1 on generic error, -2 if the buffer does not
 *             contain enough data yet
 */
int rc_timed_ringbuf_get_val_at_pos(rc_timed_ringbuf_t* buf, int position, double* result);


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


#ifdef __cplusplus
}
#endif

#endif // RC_TIMED_RINGBUF_H

