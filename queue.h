#include <pthread.h>
#include <stdint.h>

typedef struct {
    // backing buffer and size
    uint8_t *buffer;
    size_t size;

    // backing buffer's memfd descriptor
    int fd;

    // read / write indices
    size_t head, tail;

    // synchronization primitives
    pthread_cond_t readable, writeable;
    pthread_mutex_t lock;
} queue_t;

typedef struct record_item{
    const char* op_name;
    uint64_t Memory_usage;
    uint64_t op_time;
}record_item;

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>

/* Convenience wrappers for erroring out */
static inline void queue_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "queue error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}

static inline void queue_error_errno(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "queue error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, " (errno %d)\n", errno);
    va_end(args);
    abort();
}
void queue_init(queue_t *q, size_t s)
{
    /* We mmap two adjacent pages (in virtual memory) that point to the same
     * physical memory. This lets us optimize memory access, so that we don't
     * need to even worry about wrapping our pointers around until we go
     * through the entire buffer.
     */
    size_t real_mmap = s;
    size_t buffer_algin = ((s - 1 + getpagesize()) / getpagesize()) * getpagesize();
    s =  real_mmap > buffer_algin ? real_mmap : buffer_algin;
    // Check that the requested size is a multiple of a page. If it isn't, we're
    // in trouble.
    if (s % getpagesize() != 0) {
        queue_error(
            "Requested size (%lu) is not a multiple of the page size (%d)", s,
            getpagesize());
    }
    // Create an anonymous file backed by memory
    //if ((q->fd = memfd_create("queue_region", 0)) == -1)
        //queue_error_errno("Could not obtain anonymous file");
    q->fd = shm_open("/shm_queue", O_RDWR, 0666);
    if (q->fd == -1) {
            perror("shm_open failed");
            exit(1);
    }
    // Set buffer size
    if (ftruncate(q->fd, s) != 0)
        perror("Could not set size of shared memory");
    // Ask mmap for a good address
    q->buffer = mmap(0, s, PROT_READ, MAP_SHARED, q->fd, 0);
    if (q->buffer == MAP_FAILED){
        perror("mmap failed"); 
        exit(1);
    } 
    if (pthread_mutex_init(&q->lock, NULL) != 0)
        queue_error_errno("Could not initialize mutex");
    // Initialize remaining members
    q->size = s;
    q->head = q->tail = 0;
}


/** Destroy the blocking queue *q* */
void queue_destroy(queue_t *q)
{
    if (munmap(q->buffer + q->size, q->size) != 0)
        queue_error_errno("Could not unmap buffer");

    if (munmap(q->buffer, q->size) != 0)
        queue_error_errno("Could not unmap buffer");
    shm_unlink("/shm_queue"); 
    if (pthread_mutex_destroy(&q->lock) != 0)
        queue_error_errno("Could not destroy mutex");
}

/** Retrieves a message of at most *max* bytes from queue *q* and writes
 * it to *buffer*.
 *
 * Returns the number of bytes in the written message.
 */
size_t queue_get(queue_t *q, uint8_t *buffer, size_t max)
{
    pthread_mutex_lock(&q->lock);

    // Wait for a message that we can successfully consume to reach
    // the front of the queue
    if ((q->tail - q->head) == 0) {
        printf("end \n");
        return 0;
        }

    // Read message body
    memcpy(buffer, &q->buffer[q->head], sizeof(size_t));
    // printf("%ld\n", (size_t) *(size_t *)buffer);

    // Consume the message by incrementing the read pointer
    q->head += max;

    // When read buffer moves into 2nd memory region, we can reset to
    // the 1st region
    if (q->head >= q->size) {
        q->head -= q->size; q->tail -= q->size;
    }
    pthread_mutex_unlock(&q->lock);
    return max;
}
