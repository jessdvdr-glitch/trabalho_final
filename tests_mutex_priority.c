#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "structures.h"

int main(void){ // IMPORTANT: COMPILE WITH THE COMMAND: gcc tests_mutex_priority.c structures.c -o tests_mutex_priority
    srand(time(NULL));

    int n = 8;
    Aeronave** aeronaves = (Aeronave**)malloc(n * sizeof(Aeronave));

    for(int i = 0; i < n; i++){
        aeronaves[i] = (Aeronave*)malloc(sizeof(Aeronave));
        aeronaves[i]->id = i;
        aeronaves[i]->priority = rand() % (n*n);
        printf("criada aeronave %d, com prioridade %d\n", i, aeronaves[i]->priority);
    }

    printf("\n");

    MutexPriority * mp;
    mp = create_mutex_priority(n, 0);

    for(int i = 0; i < n; i++){
        insert_aeronave_mutex_priority(mp, aeronaves[i]);
    }

    for(int i = 0; i < mp->waiting_list_size; i++){
        printf("aeronave %d na posicao %d da lista, prioridade %d\n", mp->waiting_list[i]->id, i, mp->waiting_list[i]->priority);
    }

    printf("\n");

    for(int i = 0; i < n; i++){
        Aeronave* out = remove_aeronave_mutex_priority(mp);
        printf("aeronave %d saiu da frente da fila\n", out->id);
    }

    // Aeronave* test = remove_aeronave_mutex_priority(mp);
    // if(test != NULL){
    //     printf("ERRO!\n");
    // }

    destroy_mutex_priority(mp);
    for(int i = 0; i < n; i++){
        free(aeronaves[i]);
    }
    free(aeronaves);
}