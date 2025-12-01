#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include "structures.h"


void* thread_aeronave_function(void *arg) {
    Aeronave id = *((Aeronave *)arg);
    printf("Aeronave %d started\n", id.id);
    pthread_exit(NULL);
    return NULL;
}

void* thread_centralized_control_mechanism(void *arg) {
    CentralizedControlMechanism id = *((CentralizedControlMechanism *)arg);
    printf("Centralized Control Mechanism started\n");
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
    Sector * sectors = malloc(sizeof(Sector) * number_sectors);
    Aeronave * aeronaves = malloc(sizeof(Aeronave) * number_aeronaves);
    CentralizedControlMechanism centralized_control_mechanism = create_centralized_control_mechanism();

    for (int i = 0; i < number_sectors; i++) {
        sectors[i] = create_sector(i);
    }
    
    for (int j = 0; j < number_aeronaves; j++) {
        aeronaves[j] = create_aeronave(j);
    }

    // initialize threads
    pthread_t * aeronaves_threads = malloc(sizeof(pthread_t) * number_aeronaves);
    pthread_t centralized_control_mechanism_thread;
    pthread_create(&centralized_control_mechanism_thread, NULL, (void *)thread_centralized_control_mechanism, (void *)&centralized_control_mechanism);

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
    destroy_centralized_control_mechanism(&centralized_control_mechanism);
    printf("Main thread finished\n");
    return 0; 
}    