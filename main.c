#define _DEFAULT_SOURCE  // Enable usleep and other POSIX features
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "structures.h"

// global variables
Sector ** sectors;
Sector ** aux_sectors;
pthread_mutex_t aux_mutex;
int aux_var = 0;
Aeronave ** aeronaves;
CentralizedControlMechanism * centralized_control_mechanism;
int *thread_returns;
int n_threads;

int check_end_aeronaves(void){
    for (int i = 0; i < n_threads; i++) {
        if(!thread_returns[i]){ // if even single thread is still executing, return 0
            return 0;
        }
    }
    return 1;
}

void* thread_aeronave_function(void *arg) {
    Aeronave *a = (Aeronave *)arg;                    // use the real pointer instead of copying
    init_aeronave(a);                                 // run loop inside the real aeronave
    thread_returns[a->id] = 1; // tell everyone it has ended
    pthread_exit(NULL);
}

void* thread_centralized_control_mechanism(void *arg) {
    (void)arg;
    printf("\033[32m[CCM_THREAD] Centralized Control Mechanism thread started\033[0m\n");
    
    // Main loop: continuously process requests from the queue
    while (1) {
        // Dequeue a request from the front of the queue (FIFO)
        RequestSector * current_request = dequeue_request(centralized_control_mechanism);
        
        // Check if there is a valid request
        if (current_request != NULL) {
            printf("\033[32m[CCM_THREAD] Processing request: Aircraft %d for Sector %d\033[0m\n", 
                   current_request->id_aeronave,
                   current_request->id_sector);
            
            // Call control_priority to attempt to acquire the sector
            control_priority(current_request, 
                            centralized_control_mechanism->mutex_sections,
                            &centralized_control_mechanism->mutex_request);
        } 
        else{
            usleep(100);
        }
        if(check_end_aeronaves()){ // ends only if all other threads have too
            pthread_exit(NULL);
        }
    }
    
    printf("\033[32m[CCM_THREAD] Centralized Control Mechanism thread finished\033[0m\n");
    pthread_exit(NULL);
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage : %s <nombre1> <nombre2>\n", argv[0]);
        return 1; 
    }

    srand(time(NULL));
    int number_sectors = atoi(argv[1]);
    int number_aeronaves = atoi(argv[2]);
    int max_tam_rota = number_sectors*2;
    
    // Initialize structures
    sectors = malloc(sizeof(Sector*) * (number_sectors + number_aeronaves));
    aux_sectors = malloc(sizeof(Sector*) * number_aeronaves);
    aeronaves = malloc(sizeof(Aeronave*) * number_aeronaves);
    centralized_control_mechanism = create_centralized_control_mechanism(number_sectors, number_aeronaves); // use sectors count

    thread_returns = malloc(sizeof(int) * number_aeronaves);
    n_threads = number_aeronaves;
    for (int i = 0; i < number_aeronaves; i++) {
        thread_returns[i] = 0;
    } 

    for (int i = 0; i < number_sectors+number_aeronaves; i++) {
        sectors[i] = create_sector(i);
    }
    for (int i = 0; i < number_aeronaves; i++) {
        aux_sectors[i] = sectors[number_sectors+i];
    }
    pthread_mutex_init(&aux_mutex, NULL);
    
    for (int j = 0; j < number_aeronaves; j++) {
        aeronaves[j] = create_aeronave(j, rand() % 1000, rand() % max_tam_rota + 1);
    }

    // initialize threads
    pthread_t * aeronaves_threads = malloc(sizeof(pthread_t) * number_aeronaves);
    pthread_t centralized_control_mechanism_thread;
    pthread_create(&centralized_control_mechanism_thread, NULL, thread_centralized_control_mechanism, NULL);

    for(int j = 0; j < number_aeronaves; j++) {
        pthread_create(&aeronaves_threads[j], NULL,
                       thread_aeronave_function, (void *)aeronaves[j]);
    }
    
    for(int j = 0; j < number_aeronaves; j++) {
        pthread_join(aeronaves_threads[j], NULL);
    }
    pthread_join(centralized_control_mechanism_thread, NULL);

    printf("Routes finished\n");
    printf("Mean waiting times (only considers from request to acquirement of the sector it wanted. That is: keeps counting while in BACKUP)\n");
    for(int i = 0; i < number_aeronaves; i++){
        printf("[AIRCRAFT %d : Priority %d] Mean waiting time: %.5f\n", i, aeronaves[i]->priority, aeronaves[i]->mean_wait_time/aeronaves[i]->tam_rota);
    }
    

    free(aeronaves_threads);
    free(thread_returns);
    
    for (int i = 0; i < number_sectors+number_aeronaves; i++) {
        destroy_sector(sectors[i]);
    }
    free(sectors);
    free(aux_sectors);
    pthread_mutex_destroy(&aux_mutex);
    for (int i = 0; i < number_aeronaves; i++) {
        destroy_aeronave(aeronaves[i]);
    }
    free(aeronaves);
    destroy_centralized_control_mechanism(centralized_control_mechanism);
    printf("Main thread finished\n");
    return 0; 
}    