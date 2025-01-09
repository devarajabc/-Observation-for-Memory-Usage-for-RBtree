#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int main(){
    queue_t Q;
    queue_init(&Q,sizeof(record_item)*9000);
    while(1){
        record_item tmp;
        if(!queue_get(&Q, &tmp, sizeof(record_item))) break;
        printf("Action : %s, Time : %ld, Memory Usage %lld \n",tmp.op_name, tmp.op_time, tmp.Memory_usage);

    }
    queue_destroy(&Q);
    return 0;
}