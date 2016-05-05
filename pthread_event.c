/*
The MIT License (MIT)

Copyright (c) 2014, Stephen Scott
All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

/* 
 * pthread_event implementation
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "pthread_event.h"
#include "pthread_ext_common.h"

/**************************************************************************************************/
static void cleanup_handler(void *arg)
{
	pthread_mutex_unlock((pthread_mutex_t*)arg);
}

/**************************************************************************************************/
/* pthread_event_create
 * create and initialize a new event.
 */
int pthread_event_create(pthread_event_t ** ppevent)
{
	pthread_event_t * event;

	if (NULL == *ppevent)
	{
		event = (pthread_event_t *) malloc(sizeof(pthread_event_t));
		if (NULL == event)
			return ENOMEM;

		*ppevent = event;
		event->destroyFree = 1;
	}
	else
	{
		event = *ppevent;
		event->destroyFree = 0;
	}

	pthread_mutex_init(&event->mutex, NULL);
	pthread_cond_init(&event->cond, NULL);
	event->mask = 0;
	event->reset = 0;

	return 0;

} // pthread_event_create

/**************************************************************************************************/
/* pthread_event_destroy
 * free an event.
 */
void pthread_event_destroy(pthread_event_t *event)
{
	pthread_mutex_destroy(&event->mutex);
	pthread_cond_destroy(&event->cond);
	if (event->destroyFree)
		free(event);

} /* pthread_event_destroy */


/**************************************************************************************************/
/* pthread_event_set
 * set event flags.
 */
int pthread_event_set(pthread_event_t *event, pthread_event_mask mask)
{
	pthread_mutex_lock(&event->mutex);

	event->mask |= mask;

	/* signal waiters */
	pthread_mutex_unlock(&event->mutex);
	pthread_cond_broadcast(&event->cond);

	return 0;

} /* pthread_event_set */


/**************************************************************************************************/
/* pthread_event_clr
 * clear event flags.
 */
int pthread_event_clr(pthread_event_t *event, pthread_event_mask mask)
{
	pthread_mutex_lock(&event->mutex);

	event->mask &= ~mask;

	/* signal waiters */
	pthread_mutex_unlock(&event->mutex);

	return 0;

} /* pthread_event_clr */

/**************************************************************************************************/
/* pthread_event_wait
 * test is PTHREAD_EVENT_ANY (logic OR) or PTHREAD_EVENT_ALL (logic AND)
 * timeout is PTHREAD_WAIT or PTHREAD_NOWAIT or timeout in ms
 * If timeout == PTHREAD_WAIT and the event test is not satisfied, function blocks.
 */

int pthread_event_wait(pthread_event_t *event, pthread_event_mask mask, pthread_event_test test,
						pthread_event_action action, long timeout)
{
	struct timespec abstime;
	uint8_t			done;
	int				result;

	if ( (PTHREAD_WAIT != timeout) && (timeout < 0) )
		return EINVAL;

	// convert wait to absolute system time
	if (timeout > 0)
		pthread_ext_ms2abs_time(timeout, &abstime);

	pthread_mutex_lock(&event->mutex);

	/* handle nowait and event test is not satisfied */
	done = (PTHREAD_EVENT_ANY == mask) ? ((event->mask & mask) != 0) : ((event->mask & mask) == mask) ;
	
	if ( (PTHREAD_NOWAIT == timeout) && (!done) )
	{
		pthread_mutex_unlock(&event->mutex);
		return ETIMEDOUT;
	}

	/* wait for the event test to be satisfied */
	while (!done && !event->reset) {

		pthread_cleanup_push(cleanup_handler, &event->mutex);
		if (PTHREAD_WAIT == timeout)
			result = pthread_cond_wait(&event->cond, &event->mutex);
		else
			result = pthread_cond_timedwait(&event->cond, &event->mutex, &abstime);
		pthread_cleanup_pop(0);

		if (ETIMEDOUT == result)
		{
			pthread_mutex_unlock(&event->mutex);
			return ETIMEDOUT;
		}

		result = event->reset ? ECANCELED : 0;	// if we woke up because of reset
		done = (PTHREAD_EVENT_ANY == mask) ? ((event->mask & mask) != 0) : ((event->mask & mask) == mask) ;
	}

	if (PTHREAD_EVENT_CLEAR == action)
		event->mask &= ~mask;

	pthread_mutex_unlock(&event->mutex);

	return (result);

} /* pthread_event_wait */

/**************************************************************************************************/
/* pthread_event_current
 * return current event mask
 */
pthread_event_mask pthread_event_current(pthread_event_t * event)
{
	return event->mask;
}

/**************************************************************************************************/
/* pthread_event_reset
 * clear the event mask and prevent any inputs while reset. Wake up all threads
 * blocked waiting for an event and force them to stop waiting.
 */
int pthread_event_reset(pthread_event_t * event)
{
	pthread_mutex_lock(&event->mutex);
	event->mask = 0;
	event->reset = 1;
	pthread_mutex_unlock(&event->mutex);
	pthread_cond_broadcast(&event->cond);

	return 0;
}

/**************************************************************************************************/
/* pthread_event_unreset
 * reenable the event
 */
int pthread_event_unreset(pthread_event_t * event)
{
	pthread_mutex_lock(&event->mutex);
	event->reset = 0;
	pthread_mutex_unlock(&event->mutex);

	return 0;
}
