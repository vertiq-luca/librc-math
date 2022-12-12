/**
 * @author     james@modalai.com
 * @date       2022
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> // for sqrt
#include <rc_math/timed3_ringbuf.h>
#include <rc_math/quaternion.h>
#include "algebra_common.h"


rc_timed3_ringbuf_t rc_timed3_ringbuf_empty(void)
{
	rc_timed3_ringbuf_t new = RC_TIMED3_RINGBUF_INITIALIZER;
	return new;
}


int rc_timed3_ringbuf_alloc(rc_timed3_ringbuf_t* buf, int size)
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
	// Don't allocate twice, throw an error if it's not starting clean
	if(buf->initialized || buf->size==size || buf->d!=NULL){
		fprintf(stderr,"ERROR in %s, buf already initialized or not in a clean state\n", __FUNCTION__);
		return -1;
	}
	// make sure it's zero'd out
	buf->size = 0;
	buf->index = 0;
	buf->items_in_buf = 0;
	buf->initialized = 0;
	// allocate mem for array
	buf->d = (double*)calloc(size,3*sizeof(double));
	buf->t = (int64_t*)calloc(size,sizeof(int64_t));
	if(buf->d==NULL || buf->t==NULL){
		fprintf(stderr,"ERROR in %s, failed to allocate memory\n", __FUNCTION__);
		return -1;
	}
	// write out other details
	buf->size = size;
	buf->initialized = 1;
	return 0;
}


int rc_timed3_ringbuf_free(rc_timed3_ringbuf_t* buf)
{
	rc_timed3_ringbuf_t new = RC_TIMED3_RINGBUF_INITIALIZER;
	if(unlikely(buf==NULL)){
		fprintf(stderr, "ERROR in %s, received NULL pointer\n", __FUNCTION__);
		return -1;
	}
	if(buf->initialized){
		free(buf->d);
		free(buf->t);
		// put buffer back to default
		*buf = new;
	}
	return 0;
}


int rc_timed3_ringbuf_insert(rc_timed3_ringbuf_t* buf, int64_t ts_ns, double* val)
{
	int new_index;

	// sanity checks
	if(unlikely(buf==NULL || val==NULL)){
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
		if(ts_ns<=buf->t[buf->index]){
			fprintf(stderr,"ERROR in %s, timestamp out of order\n", __FUNCTION__);
			pthread_mutex_unlock(&buf->mutex);
			return -1;
		}
		new_index=buf->index+1;
		if(new_index>=buf->size) new_index=0;
	}

	// write out new value
	buf->t[new_index] = ts_ns;
	memcpy(&buf->d[3*new_index], val, 3*sizeof(double));
	// bump index
	buf->index=new_index;
	// increment number of items if necessary
	if(buf->items_in_buf<buf->size) buf->items_in_buf++;

	pthread_mutex_unlock(&buf->mutex);

	return 0;
}


static int _get_ts_at_pos_nolock(rc_timed3_ringbuf_t* buf, int position, int64_t* ts)
{
	// check for looparound
	int return_index = buf->index-position;
	if(return_index<0) return_index+=buf->size;

	*ts = buf->t[return_index];

	return 0;
}


int rc_timed3_ringbuf_get_ts_at_pos(rc_timed3_ringbuf_t* buf, int position, int64_t* ts)
{
	// sanity checks
	if(unlikely(buf==NULL || ts==NULL)){
		fprintf(stderr,"ERROR in %s, received NULL pointer\n", __FUNCTION__);
		*ts = -1;
		return -1;
	}
	if(unlikely(position<0 || (position>buf->size-1))){
		fprintf(stderr,"ERROR in %s, position out of bounds\n", __FUNCTION__);
		*ts = -1;
		return -1;
	}
	if(unlikely(!buf->initialized)){
		fprintf(stderr,"ERROR in %s, ringbuf uninitialized\n", __FUNCTION__);
		*ts = -1;
		return -1;
	}
	// silently return if user requested an item that hasn't been added yet
	if(position>=buf->items_in_buf){
		*ts = -1;
		return -2;
	}

	pthread_mutex_lock(&buf->mutex);
	_get_ts_at_pos_nolock(buf, position, ts);
	pthread_mutex_unlock(&buf->mutex);
	return 0;
}

static int _get_val_at_pos_nolock(rc_timed3_ringbuf_t* buf, int position, double* val)
{
	// convert position to index, check for looparound
	int return_index = buf->index - position;
	if(return_index<0) return_index += buf->size;

	memcpy(val, &buf->d[3*return_index], 3*sizeof(double));

	return 0;
}


int rc_timed3_ringbuf_get_val_at_pos(rc_timed3_ringbuf_t* buf, int position, double* val)
{
	// sanity checks
	if(unlikely(buf==NULL || val==NULL)){
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
		return -2;
	}

	pthread_mutex_lock(&buf->mutex);
	_get_val_at_pos_nolock(buf, position, val);
	pthread_mutex_unlock(&buf->mutex);

	return 0;
}


static int _get_pos_b4_ts_nolock(rc_timed3_ringbuf_t* buf, int64_t ts)
{
	int64_t tmp;

	// make sure we actually have some data in the buffer
	if(buf->items_in_buf<2) return -2;

	// quick check that we have data old enough before searching through
	_get_ts_at_pos_nolock(buf, buf->items_in_buf-1, &tmp);
	if(tmp>ts) return -2;

	for(int i=0; i<buf->items_in_buf; i++){
		_get_ts_at_pos_nolock(buf, i, &tmp);
		if(tmp<=ts) return i;
	}
	return -3;
}


int rc_timed3_ringbuf_get_pos_b4_ts(rc_timed3_ringbuf_t* buf, int64_t ts)
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
	int ret = _get_pos_b4_ts_nolock(buf, ts);
	pthread_mutex_unlock(&buf->mutex);

	return ret;
}


int rc_timed3_ringbuf_get_val_at_time(rc_timed3_ringbuf_t* buf, int64_t ts_ns, double* val)
{
	int i;
	int found = 0;
	int64_t t1, t2 = 0;
	double x1[3], x2[3];

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

	// going to do a lot of operations, lock the mutex and only call nolock operations in here
	pthread_mutex_lock(&buf->mutex);

	// grab newest entry
	int64_t latest_ts = 0;
	if(unlikely(_get_ts_at_pos_nolock(buf,0, &latest_ts))){
		fprintf(stderr,"ERROR in %s, failed to fetch entry\n", __FUNCTION__);
		pthread_mutex_unlock(&buf->mutex);
		return -1;
	}

	// allow timestamps up to forward_limit ns newer than our last position record
	if(ts_ns > (latest_ts+buf->forward_limit)){
		fprintf(stderr,"ERROR in %s, requested timestamp too new\n", __FUNCTION__);
		fprintf(stderr,"Requested time %7.2fs newer than latest data\n", \
											(double)(ts_ns-latest_ts)/1000000000.0);
		pthread_mutex_unlock(&buf->mutex);
		return -3;
	}

	// check for timestamp newer than we have record of, if so, extrapolate given
	// the two most recent records
	if(ts_ns > latest_ts){

		if(unlikely(_get_val_at_pos_nolock(buf, 1, x1))){
			fprintf(stderr,"ERROR in %s, failed to fetch entry before ts\n", __FUNCTION__);
			pthread_mutex_unlock(&buf->mutex);
			return -1;
		}
		if(unlikely(_get_ts_at_pos_nolock(buf, 1,  &t1))){
			fprintf(stderr,"ERROR in %s, failed to fetch entry before ts\n", __FUNCTION__);
			pthread_mutex_unlock(&buf->mutex);
			return -1;
		}
		if(unlikely(_get_val_at_pos_nolock(buf, 0, x2))){
			fprintf(stderr,"ERROR in %s, failed to fetch entry after ts\n", __FUNCTION__);
			pthread_mutex_unlock(&buf->mutex);
			return -1;
		}
		if(unlikely(_get_ts_at_pos_nolock(buf, 0, &t2))){
			fprintf(stderr,"ERROR in %s, failed to fetch entry after ts\n", __FUNCTION__);
			pthread_mutex_unlock(&buf->mutex);
			return -1;
		}
		found = 1;
	}
	else{
		// now go searching through the buffer to find which two entries to interpolate between
		for(i=0;i<buf->items_in_buf;i++){

			_get_ts_at_pos_nolock(buf,i, &t1);

			// check for error
			if(t1<=0){
				fprintf(stderr,"ERROR in %s, found bad timestamp at position%d\n", __FUNCTION__, i);
				pthread_mutex_unlock(&buf->mutex);
				return -1;
			}

			// found the right value! no interpolation needed
			if(t1 == ts_ns){
				if(unlikely(_get_val_at_pos_nolock(buf, i, val))){
					fprintf(stderr,"ERROR in %s, failed to fetch entry at ts\n", __FUNCTION__);
					pthread_mutex_unlock(&buf->mutex);
					return -1;
				}
				pthread_mutex_unlock(&buf->mutex);
				return 0;
			}

			// once we get a timestamp older than requested ts, we have found the
			// right interval, grab the appropriate transforms
			if(t1<ts_ns){
				// if this condition is met on the newest entry in the buffer where
				// position=0 then the user is asking for data that's in the future
				// or at least newer than the buffer is aware of.
				// should never get here due to previous checks but keep it as a
				// sanity check, might be handy in future development
				if(i==0){
					fprintf(stderr,"WARNING in %s, requested timestamp is newer than buffer's newest entry\n", __FUNCTION__);
					pthread_mutex_unlock(&buf->mutex);
					return -1;
				}

				// This step should be older than requested ts, make sure the next
				// entry is actually newer, it should be if data went into the
				// buffer monotonically
				int64_t tmp;
				_get_ts_at_pos_nolock(buf,i-1,&tmp);
				if(unlikely(tmp<ts_ns)){
					fprintf(stderr,"ERROR in %s, bad timestamp found\n", __FUNCTION__);
					pthread_mutex_unlock(&buf->mutex);
					return -1;
				}

				// these fetches shouldn't fail since we already got the timestamp
				// at this position.
				_get_val_at_pos_nolock(buf, i,   x1);
				_get_val_at_pos_nolock(buf, i-1, x2);
				_get_ts_at_pos_nolock(buf,  i-1, &t2);
				// break and start interpolation
				found = 1;
				break;
			}
		} // end loop through buffer
	}

	pthread_mutex_unlock(&buf->mutex);

	// unsuccessful in finding
	if(!found) return -2;

	// calculate interpolation constant t which is between 0 and 1.
	// 0 would be right at tf_before, 1 would be right at tf_after.
	double t = (double)(ts_ns-t1) / (double)(t2-t1);
	val[0] = x1[0] + (t*(x2[0]-x1[0]));
	val[1] = x1[1] + (t*(x2[1]-x1[1]));
	val[2] = x1[2] + (t*(x2[2]-x1[2]));

	return 0;
}


// TODO optimize this, just got it working for now
int rc_timed3_ringbuf_integrate_over_time(rc_timed3_ringbuf_t* buf, int64_t t_start, int64_t t_end, double* integral)
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
	if(t_start>=t_end){
		fprintf(stderr,"ERROR in %s, t_start must be older than t_end\n", __FUNCTION__);
		return -1;
	}

	pthread_mutex_lock(&buf->mutex);

	int pos_start = _get_pos_b4_ts_nolock(buf, t_start);
	if(pos_start<0){
		pthread_mutex_unlock(&buf->mutex);
		return -2;
	}
	int pos_end = _get_pos_b4_ts_nolock(buf, t_end);
	if(pos_end<0){
		pthread_mutex_unlock(&buf->mutex);
		return -3;
	}

	double accumulator[3];
	memset(accumulator, 0, 3*sizeof(double));
	memset(integral, 0, 3*sizeof(double));

	int64_t t1, t2;
	double x1[3], x2[3];

	if(_get_ts_at_pos_nolock(buf, pos_start, &t1)){
		fprintf(stderr,"ERROR in %s\n", __FUNCTION__);
		pthread_mutex_unlock(&buf->mutex);
		return -1;
	}
	if(_get_val_at_pos_nolock(buf, pos_start, x1)){
		fprintf(stderr,"ERROR in %s\n", __FUNCTION__);
		pthread_mutex_unlock(&buf->mutex);
		return -1;
	}

	for(int i=(pos_start-1); i>=pos_end; i--){


		if(_get_ts_at_pos_nolock(buf, i, &t2)){
			fprintf(stderr,"ERROR in %s\n", __FUNCTION__);
			pthread_mutex_unlock(&buf->mutex);
			return -1;
		}
		if(_get_val_at_pos_nolock(buf, i, x2)){
			fprintf(stderr,"ERROR in %s\n", __FUNCTION__);
			pthread_mutex_unlock(&buf->mutex);
			return -1;
		}

		double dt_s = (double)(t2 - t1)/1000000000.0;
		accumulator[0] += dt_s * (x1[0] + x2[0])/2.0;
		accumulator[1] += dt_s * (x1[1] + x2[1])/2.0;
		accumulator[2] += dt_s * (x1[2] + x2[2])/2.0;

		memcpy(x1,x2,3*sizeof(double));
		t1 = t2;

	}

	memcpy(integral, accumulator, 3*sizeof(double));
	pthread_mutex_unlock(&buf->mutex);

	return 0;
}




static int _mean_nolock(rc_timed3_ringbuf_t* buf, int n, double* out)
{
	double sum[3] = {0,0,0};
	int i;

	// find where to start the first pass
	int start = buf->index - n + 1;
	if(start<0) start += buf->size;

	// find how long the first pass will be
	int n_first_pass = buf->size - start;
	if(n_first_pass>n) n_first_pass = n;

	// sum the first chunk up to the end of the buffer
	for(i=start; i<(start+n_first_pass); i++){
		sum[0] += buf->d[(i*3)+0];
		sum[1] += buf->d[(i*3)+1];
		sum[2] += buf->d[(i*3)+2];
	}

	// see if a second pass is needed due to wrap
	if(n_first_pass < n){
		int n_second_pass = n-n_first_pass;
		for(i=0; i<n_second_pass; i++){
			sum[0] += buf->d[(i*3)+0];
			sum[1] += buf->d[(i*3)+1];
			sum[2] += buf->d[(i*3)+2];
		}
	}

	// all done!
	out[0] = sum[0]/n;
	out[1] = sum[1]/n;
	out[2] = sum[2]/n;
	return 0;
}


int rc_timed3_ringbuf_mean(rc_timed3_ringbuf_t* buf, int n, double* out)
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
	if(unlikely(n<1 || n>buf->size)){
		fprintf(stderr,"ERROR in %s, requested too many or too few\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(n>buf->items_in_buf)){
		fprintf(stderr,"ERROR in %s, requested %d items but buffer has only been populated with %d items\n",\
							__FUNCTION__, n, buf->items_in_buf);
		return -1;
	}

	pthread_mutex_lock(&buf->mutex);
	_mean_nolock(buf, n, out);
	pthread_mutex_unlock(&buf->mutex);

	return 0;
}


int rc_timed3_ringbuf_std_dev(rc_timed3_ringbuf_t* buf, int n, double* out)
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
	if(unlikely(n<1 || n>buf->size)){
		fprintf(stderr,"ERROR in %s, requested too many or too few\n", __FUNCTION__);
		return -1;
	}
	if(unlikely(n>buf->items_in_buf)){
		fprintf(stderr,"ERROR in %s, requested %d items but buffer has only been populated with %d items\n",\
							__FUNCTION__, n, buf->items_in_buf);
		return -1;
	}

	// shortcut for length 1
	if(n==1){
		memset(out, 0, 3*sizeof(double));
		return 0;
	}

	pthread_mutex_lock(&buf->mutex);

	double mean[3], diff;
	int i;

	// find mean
	_mean_nolock(buf, n, mean);

	// find where to start the first pass
	int start = buf->index - n + 1;
	if(start<0) start += buf->size;

	// find how long the first pass will be
	int n_first_pass = buf->size - start;
	if(n_first_pass>n) n_first_pass = n;

	// sum the first chunk up to the end of the buffer
	double mean_sqr[3] = {0, 0, 0};
	for(i=start; i<(start+n_first_pass); i++){
		diff = buf->d[(i*3)+0]-mean[0];
		mean_sqr[0] += diff*diff;
		diff = buf->d[(i*3)+1]-mean[1];
		mean_sqr[1] += diff*diff;
		diff = buf->d[(i*3)+2]-mean[2];
		mean_sqr[2] += diff*diff;
	}

	// see if a second pass is needed due to wrap
	if(n_first_pass < n){
		int n_second_pass = n-n_first_pass;
		for(i=0; i<n_second_pass; i++){
			diff = buf->d[(i*3)+0]-mean[0];
			mean_sqr[0] += diff*diff;
			diff = buf->d[(i*3)+1]-mean[1];
			mean_sqr[1] += diff*diff;
			diff = buf->d[(i*3)+2]-mean[2];
			mean_sqr[2] += diff*diff;
		}
	}

	// all done!
	out[0] = sqrt(mean_sqr[0]/(double)(n-1));
	out[1] = sqrt(mean_sqr[1]/(double)(n-1));
	out[2] = sqrt(mean_sqr[2]/(double)(n-1));
	pthread_mutex_unlock(&buf->mutex);

	return 0;
}

// small angle approximation removes need for sin/cos
static inline void _small_angle_q_from_gyro(double* xyz, double* q)
{
	// small angle approximation
	double sinX = xyz[0] * 0.5;
	double sinY = xyz[1] * 0.5;
	double sinZ = xyz[2] * 0.5;

	q[0] = 1.0 + sinX*sinY*sinZ;
	q[1] = sinX - sinY*sinZ;
	q[2] = sinY + sinX*sinZ;
	q[3] = sinZ - sinX*sinY;

	// no need to normalize here, we are only going to be doing quaterion
	// multiplication, not rotating vectors
	return;
}


// This is faster than rc_quaternion_multiply_array since it's inlined
static inline void _quaternion_left_multiply_inplace(double* a, double* b)
{
	double tmp[4];
	tmp[0] = b[0];
	tmp[1] = b[1];
	tmp[2] = b[2];
	tmp[3] = b[3];

	b[0] = (tmp[0] * a[0]) - (tmp[1] * a[1]) - (tmp[2] * a[2]) - (tmp[3] * a[3]);
	b[1] = (tmp[0] * a[1]) + (tmp[1] * a[0]) + (tmp[2] * a[3]) - (tmp[3] * a[2]);
	b[2] = (tmp[0] * a[2]) + (tmp[2] * a[0]) + (tmp[3] * a[1]) - (tmp[1] * a[3]);
	b[3] = (tmp[0] * a[3]) + (tmp[3] * a[0]) + (tmp[1] * a[2]) - (tmp[2] * a[1]);
	return;
}





int rc_timed3_ringbuf_integrate_gyro_3d(rc_timed3_ringbuf_t* buf, \
							int64_t t_start, int64_t t_end, rc_matrix_t* out)
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
	if(t_start>=t_end){
		fprintf(stderr,"ERROR in %s, t_start must be older than t_end\n", __FUNCTION__);
		return -1;
	}
	if(rc_matrix_identity(out, 3)){
		fprintf(stderr,"ERROR in %s, failed to allocate output matrix\n", __FUNCTION__);
		return -1;
	}

	pthread_mutex_lock(&buf->mutex);

	int pos_start = _get_pos_b4_ts_nolock(buf, t_start);
	if(pos_start<0){
		pthread_mutex_unlock(&buf->mutex);
		return -2;
	}
	int pos_end = _get_pos_b4_ts_nolock(buf, t_end);
	if(pos_end<0){
		pthread_mutex_unlock(&buf->mutex);
		return -3;
	}

	//printf("pos_start: %4d pos_end: %4d\n", pos_start, pos_end);

	double q[4] = {1,0,0,0};
	double qnew[4];
	int64_t t1, t2;
	double x1[3], x2[3];

	if(_get_ts_at_pos_nolock(buf, pos_start, &t1)){
		fprintf(stderr,"ERROR in %s\n", __FUNCTION__);
		pthread_mutex_unlock(&buf->mutex);
		return -1;
	}
	if(_get_val_at_pos_nolock(buf, pos_start, x1)){
		fprintf(stderr,"ERROR in %s\n", __FUNCTION__);
		pthread_mutex_unlock(&buf->mutex);
		return -1;
	}

	// loop through
	for(int i=(pos_start-1); i>=pos_end; i--){

		if(_get_ts_at_pos_nolock(buf, i, &t2)){
			fprintf(stderr,"ERROR in %s\n", __FUNCTION__);
			pthread_mutex_unlock(&buf->mutex);
			return -1;
		}
		if(_get_val_at_pos_nolock(buf, i, x2)){
			fprintf(stderr,"ERROR in %s\n", __FUNCTION__);
			pthread_mutex_unlock(&buf->mutex);
			return -1;
		}

		// trapezoidal integration
		double dt_s_over_two = ((double)(t2-t1))/2000000000.0;
		double d[3];
		d[0] = (x1[0]+x2[0]) * dt_s_over_two;
		d[1] = (x1[1]+x2[1]) * dt_s_over_two;
		d[2] = (x1[2]+x2[2]) * dt_s_over_two;

		// inlined functions
		_small_angle_q_from_gyro(d, qnew);
		_quaternion_left_multiply_inplace(qnew, q);

		// save x2/t2 for next step
		t1 = t2;
		memcpy(x1,x2,3*sizeof(double));
	}

	// output final rotation matrix for that period
	RC_VECTOR_ON_STACK(v, 4);
	v.d[0] = q[0];
	v.d[1] = -q[1];
	v.d[2] = -q[2];
	v.d[3] = -q[3];
	rc_quaternion_to_rotation_matrix(v, out);

	pthread_mutex_unlock(&buf->mutex);

	return 0;
}
