#ifndef STRUCTURES_H
#define STRUCTURES_H
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

typedef struct{
    int id;
}Sector; 

typedef struct{
    int id;
    int priority;
    int * rota;
    int current_index_rota;
    Sector * current_sector;
    int aguardar;
}Aeronave;

typedef struct{
    int id_sector;
    int id_aeronave;
}RequestSector;

typedef struct{
    int id; 
    pthread_mutex_t mutex_sector; // /!\ use only pthread_mutex_try_lock()
    int max_size;
    Aeronave ** waiting_list; // pointer to pointer, because it's an array for the pointers to Aeronaves
    int waiting_list_size;
}MutexPriority;

typedef struct{
    MutexPriority ** mutex_sections; /* array of pointers to MutexPriority (one per sector) */
    int num_mutex_sections;          /* number of entries in mutex_sections */
    RequestSector * request_queue;   /* array of RequestSector objects acting as a FIFO queue */
    int request_queue_size;          /* maximum size of the request queue */
    int request_queue_front;         /* index of the front element (where we dequeue) */
    int request_queue_rear;          /* index of the rear element (where we enqueue) */
    int request_queue_count;         /* current number of requests in the queue */
    pthread_mutex_t mutex_request;   /* mutex to protect `request_queue` : one request at a time */
}CentralizedControlMechanism;

// global variables
extern Sector * sectors;
extern Aeronave * aeronaves;
extern CentralizedControlMechanism * centralized_control_mechanism;

// Sectors list fonctions 
Sector create_sector(int number_sectors);
void destroy_sectors(Sector * sectors); 
int insert_sector(Sector * sectors, Sector sector);
Sector remove_sector(Sector * sectors, int number_sectors, int id_sector);
int is_empty_sectors(Sector * sectors, int number_sectors);
int is_full_sectors(Sector * sectors, int number_sectors);

// Aeronave functions
Aeronave create_aeronave(int number_aeronaves);
void init_aeronave(Aeronave * aeronave);
void destroy_aeronaves(Aeronave * aeronaves);
int request_sector(Aeronave * aeronave, int id_sector);
int wait_sector(Aeronave * aeronave);
int acquire_sector(Aeronave * aeronave, Sector * sector);
Sector* release_sector(Aeronave * aeronave);
int repeat(Aeronave * aeronave);

//RequestSector
RequestSector create_request(int number_requests);
void destroy_requests(RequestSector * requests);

// Sector MutexPriority functions (DONE)
MutexPriority* create_mutex_priority(int max_size, int id);
void destroy_mutex_priority(MutexPriority * mutex_priority);
int order_list_by_priority(MutexPriority * mutex_priority); // max size is the number of aeronaves
void insert_aeronave_mutex_priority(MutexPriority * mutex_priority, Aeronave * aeronave);
Aeronave* remove_aeronave_mutex_priority(MutexPriority * mutex_priority);
int is_empty_mutex_priority(MutexPriority * mutex_priority);
int is_full_mutex_priority(MutexPriority * mutex_priority);

// CentralizedControlMechanism functions

int prevent_deadlock(RequestSector* requests, MutexPriority * mutex_priorities, int number_aeronaves); // not sure about the paramèters 
// thought of smth like this: if an aeronave tries to acquire the access to the next sector, there will be a timeout
// if it times out, it stops trying to acquire the next sector, waits a little bit (important to be a random time) and then
// tries again
Sector* get_next_sector(Aeronave * aeronave, Sector * sectors, int number_sectors);
CentralizedControlMechanism* create_centralized_control_mechanism(int sections_number);
void destroy_centralized_control_mechanism(CentralizedControlMechanism * ccm);
int enqueue_request(CentralizedControlMechanism * ccm, RequestSector * request);
RequestSector* dequeue_request(CentralizedControlMechanism * ccm);
int is_request_queue_empty(CentralizedControlMechanism * ccm);
Sector* control_priority(RequestSector* request, MutexPriority ** mutex_priorities, pthread_mutex_t * mutex_request);

#endif