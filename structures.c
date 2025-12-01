#include <stdio.h>
#include <stdlib.h>
#include "structures.h"

// Sector MutexPriority functions
MutexPriority create_mutex_priority(int max_size, int id){
    MutexPriority mutex_priority;
    mutex_priority.id = id;
    mutex_priority.waiting_list[max_size];
    pthread_mutex_init(&mutex_priority.mutex_sector, NULL);
    return mutex_priority;
}

void destroy_mutex_priority(MutexPriority * mutex_priority){
    free(mutex_priority);
}

int order_by_priority(Aeronave * waiting_list, int size); // max size is the number of aeronaves
void insert_aeronave_mutex_priority(MutexPriority * mutex_priority, Aeronave aeronave, int max_size);
Aeronave remove_aeronave_mutex_priority(MutexPriority * mutex_priority, int max_size);
int is_empty_mutex_priority(MutexPriority * mutex_priority);
int is_full_mutex_priority(MutexPriority * mutex_priority, int max_size);