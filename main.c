#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include "structures.h"

// global variables
Sector * sectors;
Aeronave * aeronaves;
CentralizedControlMechanism * centralized_control_mechanism;

void* thread_aeronave_function(void *arg) {
    Aeronave id = *((Aeronave *)arg);
    printf("Aeronave %d started\n", id.id);
    init_aeronave(&id);
    pthread_exit(NULL);
    return NULL;
}

void* thread_centralized_control_mechanism(void *arg) {
    int number_aeronaves = *((int *)arg);
    (void)number_aeronaves; // Mark as intentionally unused
    printf("[CCM_THREAD] Centralized Control Mechanism thread started\n");
    
    // Main loop: continuously process requests from the queue
    while (1) {
        // Dequeue a request from the front of the queue (FIFO)
        RequestSector * current_request = dequeue_request(centralized_control_mechanism);
        
        // Check if there is a valid request
        if (current_request != NULL) {
            printf("[CCM_THREAD] Processing request: Aircraft %d for Sector %d\n", 
                   current_request->id_aeronave,
                   current_request->id_sector);
            
            // Call control_priority to attempt to acquire the sector
            Sector * result = control_priority(current_request, 
                                              centralized_control_mechanism->mutex_sections,
                                              &centralized_control_mechanism->mutex_request);
            
            if (result != NULL) {
                printf("[CCM_THREAD] Request processed successfully. Sector assigned.\n");
            } else {
                printf("[CCM_THREAD] Request queued in waiting list. Aircraft will wait.\n");
            }
        } 
    }
    
    printf("[CCM_THREAD] Centralized Control Mechanism thread finished\n");
    pthread_exit(NULL);
    return NULL;
}


int main(int argc, char *argv[]) {
    // doesn't have the right number of arguments
    if (argc != 3) {
        printf("Usage : %s <nombre1> <nombre2>\n", argv[0]);
        return 1; 
    }

    int number_sectors = atoi(argv[1]);
    int number_aeronaves = atoi(argv[2]);
    // initialize structures
    sectors = malloc(sizeof(Sector) * number_sectors);
    aeronaves = malloc(sizeof(Aeronave) * number_aeronaves);
    centralized_control_mechanism = create_centralized_control_mechanism(number_aeronaves);

    for (int i = 0; i < number_sectors; i++) {
        sectors[i] = create_sector(i);
    }
    
    for (int j = 0; j < number_aeronaves; j++) {
        aeronaves[j] = create_aeronave(j);
    }

    // initialize threads
    pthread_t * aeronaves_threads = malloc(sizeof(pthread_t) * number_aeronaves);
    pthread_t centralized_control_mechanism_thread;
    int *num_aero_ptr = malloc(sizeof(int));
    *num_aero_ptr = number_aeronaves;
    pthread_create(&centralized_control_mechanism_thread, NULL, thread_centralized_control_mechanism, (void *)num_aero_ptr);

    for(int j = 0; j < number_aeronaves; j++) {
        pthread_create(&aeronaves_threads[j], NULL, (void *)thread_aeronave_function, (void *)&aeronaves[j]);
    }

    pthread_join(centralized_control_mechanism_thread, NULL);
    
    for(int j = 0; j < number_aeronaves; j++) {
        pthread_join(aeronaves_threads[j], NULL);
    }

    free(aeronaves_threads);
    // TODO : verify if all the destroy are correct
    free(sectors);
    free(aeronaves);
    destroy_centralized_control_mechanism(centralized_control_mechanism);
    free(num_aero_ptr);
    printf("Main thread finished\n");
    return 0; 
}    