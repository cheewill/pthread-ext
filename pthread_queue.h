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

/** @file pthread_queue.h
 * @brief pthread FIFO message queue implementation
 */

#ifndef PTHREAD_QUEUE_H
#define PTHREAD_QUEUE_H

#include <stdint.h>

struct pthread_queue_s;
typedef struct pthread_queue_s pthread_queue_t;

/** Timeout identifiers */
#define PTHREAD_WAIT	(-1)
#define PTHREAD_NOWAIT	(0)


/** Create a message queue with fixed length messages.
 *
 * @param queue[in]			pointer to the new queue
 * @param num_msg[in]		maximum number of messages in the queue
 * @param msg_len_bytes[in]	maximum size of each message in bytes
 * @returns					0 for success, -1 for failure
 */
int pthread_queue_create(pthread_queue_t ** ppqueue,  uint32_t num_msg, uint32_t msg_len_bytes);



/** Destroy a message queue.
 * 
 * @param queue[in]			pointer to the queue to destroy
 * @returns					nothing
 */
void pthread_queue_destroy(pthread_queue_t *queue);



/** Send message to a queue.
 * 
 * Message is copied into the queue. Calling function can deallocate local copy of
 * message immediately after function returns.
 *
 * If the queue is full, and timeout == PTHREAD_NOWAIT, function returns immediately
 * with return value of -1. If timeout == PTHREAD_WAIT, function waits indefinitely for
 * space to become available in the queue.
 *
 * @param queue[in]			pointer to the queue
 * @param msg[in]			message to place in the queue
 * @param timeout[in]		PTHREAD_WAIT or PTHREAD_NOWAIT
 * @returns					0 for success, -1 for failure
 */
int pthread_queue_sendmsg(pthread_queue_t *queue, void *msg, long timeout);



/** Get message from a queue.
 *
 * Message is copied from the queue. Calling function can deallocate local copy of
 * message whenever it has finished with the data.The buffer pointed to by msg must
 * be at least as large as the message size specified when the queue was created
 * (see msg_len_bytes arg in pthread_queue_create).
 *
 * If the queue is empty, and timeout == PTHREAD_NOWAIT, function returns immediately
 * with return value of -1. If timeout == PTHREAD_WAIT, function waits indefinitely for
 * a message to become available in the queue.
 *
 * @param queue[in]			pointer to the queue
 * @param msg[out]			buffer to receive message from the queue.
 * @param timeout[in]		PTHREAD_WAIT or PTHREAD_NOWAIT
 * @returns					0 for success, -1 for failure
 */
int pthread_queue_getmsg(pthread_queue_t *queue, void *msg, long timeout);

#endif /* PTHREAD_QUEUE_H */
