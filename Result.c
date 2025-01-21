#include <stdio.h>
#include <stdlib.h>
#include "ringbuffer.h"

int main(){
    ringbuf_shm_t ringbuf_shm;
    assert(ringbuf_shm_init(&ringbuf_shm, "/ringbuf_shm_test", ARRAY_LENGTH*8, true) == 0);
    int shm_fd;
    record_item *shared_array;
    shm_fd = shm_open("/shm_array", O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(1);
    }
    shared_array = mmap(NULL, sizeof(record_item) * ARRAY_LENGTH, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared_array == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        exit(1);
    }
    uint64_t prev = 0;
    uint64_t itr = 100000;
    
    while (itr --)
    {
        Reading(ringbuf_shm.ringbuf, shared_array,shm_fd, &prev);
    }
    printf("End\n");
    return 0;
}