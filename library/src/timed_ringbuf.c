/**
 * @author     james@modalai.com
 * @date       2022
 *
 */


#ifndef unlikely
#define unlikely(x)	__builtin_expect (!!(x), 0)
#endif

#ifndef likely
#define likely(x)	__builtin_expect (!!(x), 1)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <rc_math/timed_ringbuf.h>


rc_timed_ringbuf_t rc_timed_ringbuf_empty(void)
{
	rc_timed_ringbuf_t new = RC_TIMED_RINGBUF_INITIALIZER;
	return new;
}


int rc_timed_ringbuf_alloc(rc_timed_ringbuf_t* buf, int size)
{
	// sanity checks
	if(unlikely(buf==NULL)){
		fprintf(stderr,"ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(size<2)){
		fprintf(stderr,"ERROR in %s, size must be >=2\n", __FUNCTION__);
		return -1;
	}
	// if it's already allocated, nothing to do
	if(buf->initialized && buf->size==size && buf->d!=NULL) return 0;
	// make sure it's zero'd out
	buf->size = 0;
	buf->index = 0;
	buf->items_in_buf = 0;
	buf->initialized = 0;
	// allocate mem for array
	buf->d = (rc_ts_dbl_t*)calloc(size,sizeof(rc_ts_dbl_t));
	if(buf->d==NULL){
		fprintf(stderr,"ERROR in %s, failed to allocate memory\n", __FUNCTION__);
		return -1;
	}
	// write out other details
	buf->size = size;
	buf->initialized = 1;
	return 0;
}


int rc_timed_ringbuf_free(rc_timed_ringbuf_t* buf)
{
	rc_timed_ringbuf_t new = RC_TIMED_RINGBUF_INITIALIZER;
	if(unlikely(buf==NULL)){
		fprintf(stderr, "ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(buf->initialized){
		free(buf->d);
		// put buffer back to default
		*buf = new;
	}
	return 0;
}



int rc_timed_ringbuf_insert(rc_timed_ringbuf_t* buf, int64_t ts_ns, double val)
{
	int new_index;

	// sanity checks
	if(unlikely(buf==NULL)){
		fprintf(stderr,"ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(!buf->initialized)){
		fprintf(stderr,"ERROR in %s, ringbuf uninitialized\n", __FUNCTION__);
		return -1;
	}

	pthread_mutex_lock(&buf->mutex);

	// increment index and check for loop-around
	// if this is the first thing to be entered make sure to start at zero
	if(buf->items_in_buf==0){
		new_index=0;
	}
	else{
		if(ts_ns<=buf->d[buf->index].ts_ns){
			fprintf(stderr,"ERROR in %s, timestamp out of order\n", __FUNCTION__);
			pthread_mutex_unlock(&buf->mutex);
			return -1;
		}
		new_index=buf->index+1;
		if(new_index>=buf->size) new_index=0;
	}

	// write out new value
	buf->d[new_index].ts_ns  = ts_ns;
	buf->d[new_index].val = val;
	// bump index
	buf->index=new_index;
	// increment number of items if necessary
	if(buf->items_in_buf<buf->size) buf->items_in_buf++;

	pthread_mutex_unlock(&buf->mutex);

	return 0;
}


int rc_timed_ringbuf_get_entry_at_pos(rc_timed_ringbuf_t* buf, int position, rc_ts_dbl_t* result)
{
	int return_index;
	// sanity checks
	if(unlikely(buf==NULL || result==NULL)){
		fprintf(stderr,"ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(position<0 || (position>buf->size-1))){
		fprintf(stderr,"ERROR in %s, position out of bounds\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(!buf->initialized)){
		fprintf(stderr,"ERROR in %s, ringbuf uninitialized\n", __FUNCTION__);
		return -1;
	}
	// silently return if user requested an item that hasn't been added yet
	if(position>=buf->items_in_buf){
		//fprintf(stderr,"ERROR in rc_ringbuf_get_entry_at_pos %d, not enough entries\n", position);
		return -2;
	}

	pthread_mutex_lock(&buf->mutex);
	return_index=buf->index-position;

	// check for looparound
	if(return_index<0) return_index+=buf->size;
	*result = buf->d[return_index];

	pthread_mutex_unlock(&buf->mutex);
	return 0;
}

/**
 * @brief      Fetches the timestamp which is 'position' steps behind the last
 *             value added to the buffer.
 *
 *             If 'position' is given as 0 then the most recent entry is
 *             returned. The position obviously can't be larger than size-1.
 *             This will also check and return -2 if the buffer hasn't been
 *             filled up enough to go back that far in time.
 *
 *             don't lock the mutex in here! this function is used locally by
 *             get_tf_at_time which calls this many times while the mutex is
 *             locked already
 *
 * @param[in]  buf       Pointer to user's buffer
 * @param[in]  position  steps back in the buffer to fetch the entry from
 *
 * @return     requested timestamp on success, -2 if buffer doesn't contain
 *             enough entries to go back to requested position, -1 on other
 *             error
 */
static int64_t __get_ts_at_pos(rc_timed_ringbuf_t* buf, int position)
{
	// sanity checks
	if(unlikely(buf==NULL)){
		fprintf(stderr,"ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(position<0 || (position>buf->size-1))){
		fprintf(stderr,"ERROR in %s, position out of bounds\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(!buf->initialized)){
		fprintf(stderr,"ERROR in %s, ringbuf uninitialized\n", __FUNCTION__);
		return -1;
	}
	// silently return if user requested an item that hasn't been added yet
	if(position>=buf->items_in_buf){
		//fprintf(stderr,"ERROR in rc_ringbuf_get_timestamp_at_pos %d, not enough entries\n", position);
		return -2;
	}

	int return_index = buf->index-position;
	// check for looparound
	if(return_index<0) return_index+=buf->size;
	int64_t ret = buf->d[return_index].ts_ns;

	return ret;
}



int rc_timed_ringbuf_get_pos_b4_ts(rc_timed_ringbuf_t* buf, int64_t ts)
{
	// sanity checks
	if(unlikely(buf==NULL)){
		fprintf(stderr,"ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(!buf->initialized)){
		fprintf(stderr,"ERROR in %s, ringbuf uninitialized\n", __FUNCTION__);
		return -1;
	}

	pthread_mutex_lock(&buf->mutex);

	for(int i=0; i<buf->items_in_buf; i++){
		int64_t tmp = __get_ts_at_pos(buf, i);
		if(tmp<=ts){
			pthread_mutex_unlock(&buf->mutex);
			return i;
		}
	}

	pthread_mutex_unlock(&buf->mutex);
	return -2;
}




int rc_timed_ringbuf_get_val_at_time(rc_timed_ringbuf_t* buf, int64_t ts_ns, double* val)
{
	int i;
	int found = 0;
	static rc_ts_dbl_t val_before;
	static rc_ts_dbl_t val_after;
	int64_t tmp;

	// sanity checks
	if(unlikely(buf==NULL)){
		fprintf(stderr,"ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(!buf->initialized)){
		fprintf(stderr,"ERROR in %s, ringbuf uninitialized\n", __FUNCTION__);
		return -1;
	}
	if(ts_ns<=0){
		fprintf(stderr,"ERROR in %s, requested timestamp must be >0\n", __FUNCTION__);
		return -1;
	}

	// usual mode of operation is linear interpolation, so make sure at least
	// two entries are in the buffer.
	if(buf->items_in_buf < 2){
		return -2;
	}
	// allow timestamps up to 0.2s newer than our last position record
	int64_t latest_ts = __get_ts_at_pos(buf,0);
	if(ts_ns > (latest_ts+buf->forward_limit)){
		fprintf(stderr,"ERROR in %s, requested timestamp too new\n", __FUNCTION__);
		fprintf(stderr,"Requested time %7.2fs newer than latest data\n", (double)(ts_ns-__get_ts_at_pos(buf,0))/1000000000.0);
		return -3;
	}

	// check for timestamp newer than we have record of, if so, extrapolate given
	// the two most recent records
	if(ts_ns > latest_ts){
		if(unlikely(rc_timed_ringbuf_get_entry_at_pos(buf, 1,   &val_before))){
			fprintf(stderr,"ERROR in %s, failed to fetch entry 0\n", __FUNCTION__);
			return -1;
		}
		if(unlikely(rc_timed_ringbuf_get_entry_at_pos(buf, 0, &val_after))){
			fprintf(stderr,"ERROR in %s, failed to fetch entry 1\n", __FUNCTION__);
			return -1;
		}
		found = 1;
	}
	else{
		// now go searching through the buffer to find which two entries to interpolate between
		for(i=0;i<buf->items_in_buf;i++){
			tmp = __get_ts_at_pos(buf,i);

			// check for error
			if(tmp<=0){
				fprintf(stderr,"ERROR in %s, found unpopulated entry at position%d\n", __FUNCTION__, i);
				return -1;
			}

			// found the right value! no interpolation needed
			if(tmp == ts_ns){
				if(unlikely(rc_timed_ringbuf_get_entry_at_pos(buf, i, &val_before))){
					fprintf(stderr,"ERROR in %s, failed to fetch entry at ts\n", __FUNCTION__);
					return -1;
				}
				*val = val_before.val;
				return 0;
			}

			// once we get a timestamp older than requested ts, we have found the
			// right interval, grab the appropriate transforms
			if(tmp<ts_ns){
				// if this condition is met on the newest entry in the buffer where
				// position=0 then the user is asking for data that's in the future
				// or at least newer than the buffer is aware of.
				// should never get here due to previous checks but keep it as a
				// sanity check, might be handy in future development
				if(i==0){
					fprintf(stderr,"WARNING in %s, requested timestamp is newer than buffer's newest entry\n", __FUNCTION__);
					return -1;
				}

				// This step should be older than requested ts, make sure the next
				// entry is actually newer, it should be if data went into the
				// buffer monotonically
				if(__get_ts_at_pos(buf,i-1)<ts_ns){
					fprintf(stderr,"ERROR in %s, bad timestamp found\n", __FUNCTION__);
					return -1;
				}

				// these fetches shouldn't fail since we already got the timestamp
				// at this position.
				if(unlikely(rc_timed_ringbuf_get_entry_at_pos(buf, i,   &val_before))){
					fprintf(stderr,"ERROR in %s, failed to fetch entry before ts\n", __FUNCTION__);
					return -1;
				}
				if(unlikely(rc_timed_ringbuf_get_entry_at_pos(buf, i-1, &val_after))){
					fprintf(stderr,"ERROR in %s, failed to fetch entry after ts\n", __FUNCTION__);
					return -1;
				}
				// break and start interpolation
				found = 1;
				break;
			}
		} // end loop through buffer
	}

	// unsuccessful in finding
	if(!found) return -2;

	// calculate interpolation constant t which is between 0 and 1.
	// 0 would be right at tf_before, 1 would be right at tf_after.
	double t = (double)(ts_ns-val_before.ts_ns) / (double)(val_after.ts_ns-val_before.ts_ns);
	double v1 = val_before.val;
	double v2 = val_after.val;

	*val = v1 + (t*(v2-v1));

	return 0;
}

