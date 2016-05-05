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

/** @file pthread_event.h
 * @brief pthread event implementation
 */

#ifndef PTHREAD_EVENT_H
#define PTHREAD_EVENT_H

#include <stdint.h>

typedef enum { PTHREAD_EVENT_ANY, PTHREAD_EVENT_ALL } pthread_event_test;
typedef enum { PTHREAD_EVENT_CLEAR, PTHREAD_EVENT_KEEP } pthread_event_action;

typedef uint32_t	pthread_event_mask;

typedef struct pthread_event_s {
	pthread_mutex_t			mutex;			/* lock the structure */
	pthread_cond_t			cond;			/* condition variable*/
	pthread_event_mask		mask;			/* event mask */
	uint8_t					reset;			/* 0 = not reset, otherwise reset */
	uint8_t					destroyFree;	/* 1 = free memory on destroy */
} pthread_event_t;

/** Create an event.
 *
 * Set *ppevent = NULL to allocate memory for event. Otherwise, caller allocates memory.
 *
 * @param[inout] ppevent	if *ppevent == NULL, allocate memory for event. Returns event pointer.
 * @returns                 0 for success, otherwise an error number for failure
 * @ERRORS
 *      [ENOMEM]            memory for event not available
 */
int pthread_event_create(pthread_event_t ** ppevent);



/** Destroy an event.
 * 
 * @param[in] queue			pointer to the event
 * @returns					nothing
 */
void pthread_event_destroy(pthread_event_t *event);



/** Set event flags.
 * 
 * @param[in] event         pointer to the event
 * @param[in] mask          event flags to set
 * @returns                 0 for success
 */
int pthread_event_set(pthread_event_t *event, pthread_event_mask mask);



/** Clear event flags.
 * 
 * @param[in] event         pointer to the event
 * @param[in] mask          event flags to clear
 * @returns                 0 for success
 */
int pthread_event_clr(pthread_event_t *event, pthread_event_mask mask);



/** Wait for an event.
 *
 * If the event test is not satisfied, and timeout == PTHREAD_NOWAIT, function returns immediately
 * with return value of ETIMEDOUT. If timeout == PTHREAD_WAIT, function waits indefinitely for
 * the event test to be satisfied. Otherwise, if timeout is a positive value > 0,
 * the function waits for <timeout> ms for the event test to be satisfied.
 *
 * Function will wait for all event flags in 'mask' to be set if 'test' = PTHREAD_EVENT_ALL.
 * Function waits for any event flag in 'mask' to be set if 'test' = PTHREAD_EVENT_ANY.
 *
 * Function clears event flags which caused successful return if 'action' = PTHREAD_EVENT_CLEAR.
 * Function does not clear any event flags if 'action' = PTHREAD_EVENT_KEEP.
 *
 * @param[in] event			pointer to the event
 * @param[in] mask			bits to test
 * @param[in] test			PTHREAD_EVENT_ANY (logical OR), PTHREAD_EVENT_ALL (logical AND)
 * @param[in] action		PTHREAD_EVENT_CLEAR (clear event bits) or PTHREAD_EVENT_KEEP (leave as is)
 * @param[in] timeout		PTHREAD_WAIT or PTHREAD_NOWAIT or timeout in ms
 * @returns                 0 for success, otherwise an error number for failure
 * @ERRORS
 *      [ETIMEDOUT]         timeout has passed (or, if PTHREAD_NOWAIT, queue is full)
 *      [EINVAL]            timeout value is invalid
 *      [ECANCELED]         event was reset
 */
int pthread_event_wait(pthread_event_t *event, pthread_event_mask mask, pthread_event_test test,
						pthread_event_action action, long timeout);



/** Return current event mask.
 *
 * @param[in] event			pointer to the event
 */
pthread_event_mask pthread_event_current(pthread_event_t * event);



/** Reset event, set all bits to 0, prevent further inputs.
 *
 * @param[in] event			pointer to the event
 */
int pthread_event_reset(pthread_event_t * event);



/** Unreset event, allow bits to be set.
 *
 * @param[in] event			pointer to the event
 */
int pthread_event_unreset(pthread_event_t * event);

#endif /* PTHREAD_EVENT_H */
