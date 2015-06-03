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
 * pthread_queue implementation
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "pthread_queue.h"

struct pthread_queue_s {
	char		  *	buffer;		/* circular buffer */
	pthread_mutex_t	mutex;		/* lock the structure */
	pthread_cond_t	notfull;	/* full -> not full condition */
	pthread_cond_t	notempty;	/* empty -> not empty condition */
	uint32_t		head;		/* head of queue (first element) */
	uint32_t		tail;		/* tail of queue (last element) */
	uint32_t		count;		/* number of elements in queue */
	uint32_t		qsize;		/* max number of elements in queue */
	uint32_t		msg_len;	/* length of each message */
};

/**************************************************************************************************/
static void cleanup_handler(void *arg)
{
	pthread_mutex_unlock((pthread_mutex_t*)arg);
}

/**************************************************************************************************/
/* pthread_queue_create
 * create and initialize a new queue.
 */
int pthread_queue_create(pthread_queue_t ** ppqueue, uint32_t num_msg, uint32_t msg_len_bytes)
{
	pthread_queue_t * queue;
	queue = (pthread_queue_t *) malloc(sizeof(pthread_queue_t));
	if (NULL == queue)
		return -1;

	queue->buffer = (char *) malloc(num_msg * msg_len_bytes);
	if (NULL == queue->buffer)
	{
		free(queue);
		return -1;
	}

	pthread_mutex_init(&queue->mutex, NULL);
	pthread_cond_init(&queue->notfull, NULL);
	pthread_cond_init(&queue->notempty, NULL);
	queue->head = 0;
	queue->tail = 0;
	queue->count = 0;
	queue->qsize = num_msg;
	queue->msg_len = msg_len_bytes;

	*ppqueue = queue;

	return 0;
}

/**************************************************************************************************/
/* pthread_queue_destroy
 * free a queue.
 */
void pthread_queue_destroy(pthread_queue_t *queue)
{
	pthread_mutex_destroy(&queue->mutex);
	pthread_cond_destroy(&queue->notfull);
	pthread_cond_destroy(&queue->notempty);
	free(queue->buffer);
	free(queue);

} /* pthread_queue_destroy */


/**************************************************************************************************/
/* pthread_queue_sendmsg
 * puts new message on the queue.
 * If timeout == PTHREAD_WAIT and the queue is full, function waits until there is room.
 */
int pthread_queue_sendmsg(pthread_queue_t *queue, void *msg, long timeout)
{
	pthread_mutex_lock(&queue->mutex);

	/* handle nowait and queue is full */
	if ( (PTHREAD_NOWAIT == timeout) && (queue->count == queue->qsize) )
	{
		pthread_mutex_unlock(&queue->mutex);
		return -1;
	}

	/* wait while buffer full */
	while (queue->count == queue->qsize) {
		pthread_cleanup_push(cleanup_handler, &queue->mutex);
		pthread_cond_wait(&queue->notfull, &queue->mutex);
		pthread_cleanup_pop(0);
	}

	/* copy message to queue */
	memcpy(&queue->buffer[queue->tail * queue->msg_len], msg, queue->msg_len);
	queue->count += 1;
	queue->tail = (queue->tail == queue->qsize-1) ? 0 : queue->tail+1;

	/* signal waiting consumer */
	pthread_mutex_unlock(&queue->mutex);
	pthread_cond_signal(&queue->notempty);

	return 0;

} /* pthread_queue_sendmsg */


/**************************************************************************************************/
/* pthread_queue_getmsg
 * gets the oldest message in the queue.
 * If timeout == PTHREAD_WAIT and the queue is empty, function waits for a message.
 */

int pthread_queue_getmsg(pthread_queue_t *queue, void *msg, long timeout)
{	
	pthread_mutex_lock(&queue->mutex);

	/* handle nowait and queue is empty */
	if ( (PTHREAD_NOWAIT == timeout) && (queue->count == 0) )
	{
		pthread_mutex_unlock(&queue->mutex);
		return -1;
	}

	/* wait while there is nothing in the buffer */
	while (queue->count == 0) {
		pthread_cleanup_push(cleanup_handler, &queue->mutex);
		pthread_cond_wait(&queue->notempty, &queue->mutex);
		pthread_cleanup_pop(0);
	}

	/* copy message from the queue */
	memcpy(msg, &queue->buffer[queue->head * queue->msg_len], queue->msg_len);

	queue->count--;
	queue->head = (queue->head == queue->qsize-1) ? 0 : queue->head+1;

	/* signal waiting producer */
	pthread_mutex_unlock(&queue->mutex);
	pthread_cond_signal(&queue->notfull);

	return (0);

} /* pthread_queue_getmsg */
