#include <stdio.h>
#include <stdlib.h>
#include "structures.h"

// Sector MutexPriority functions
MutexPriority* create_mutex_priority(int max_size, int id){ // max size is the number of aeronaves
    MutexPriority* mutex_priority = (MutexPriority*)malloc(sizeof(MutexPriority));
    mutex_priority->id = id;
    mutex_priority->waiting_list = (Aeronave**)malloc(max_size * sizeof(Aeronave*));
    mutex_priority->max_size = max_size;
    mutex_priority->waiting_list_size = 0;
    pthread_mutex_init(&mutex_priority->mutex_sector, NULL);
    return mutex_priority;
}

void destroy_mutex_priority(MutexPriority * mutex_priority){
    pthread_mutex_destroy(&mutex_priority->mutex_sector);
    free(mutex_priority->waiting_list);
    free(mutex_priority);
}

int order_list_by_priority(MutexPriority * mutex_priority){
    return 0; // i think it's not necessary (and would complicate a lot)
}

void insert_aeronave_mutex_priority(MutexPriority * mutex_priority, Aeronave * aeronave){ // inserts the aeronaves by priority
    // since the centralized_control will call this function and it's managed by a single thread, there is no mutual exclusion here
    int n = mutex_priority->waiting_list_size;
    int i;
    for(i = 0; i < n; i++){
        if(mutex_priority->waiting_list[i]->priority < aeronave->priority){ // if they are equal, the order is preserved (the one that enters now will be added after the ones equal to it)
            for(int j = n-1; j >= i; j--){ // open space for the aeronave that's entering
                mutex_priority->waiting_list[j+1] = mutex_priority->waiting_list[j];
            }
            break;
        }
    }
    mutex_priority->waiting_list[i] = aeronave; // even if this aeronave priority is the lowest, i == n and the aeronave will enter by the end of the queue
    mutex_priority->waiting_list_size++;
}

Aeronave* remove_aeronave_mutex_priority(MutexPriority * mutex_priority){
    // if(mutex_priority->waiting_list_size == mutex_priority->max_size){
    //     return NULL;
    // }
    Aeronave *out = mutex_priority->waiting_list[0]; // takes the first one
    // printf("remotion succesfull\n");
    for(int i = 0; i < mutex_priority->waiting_list_size - 1; i++){ // dislocate the next ones to the head of the queue
        mutex_priority->waiting_list[i] = mutex_priority->waiting_list[i+1];
    }
    // printf("dislocation successfull\n");
    mutex_priority->waiting_list_size--;
    mutex_priority->waiting_list[mutex_priority->waiting_list_size] = NULL; // cleans last position
    return out;
}
int is_empty_mutex_priority(MutexPriority * mutex_priority){
    return mutex_priority->waiting_list_size == 0 ? 1 : 0;
}
int is_full_mutex_priority(MutexPriority * mutex_priority){
    return mutex_priority->waiting_list_size == mutex_priority->max_size ? 1 : 0;
}