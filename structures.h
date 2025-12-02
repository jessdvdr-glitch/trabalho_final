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
    RequestSector* request;          /* a single RequestSector object (may be NULL) */
    pthread_mutex_t mutex_request;   /* mutex to protect `request` : one request at a time */
}CentralizedControlMechanism;

// global variables
extern Sector * sectors;
extern Aeronave * aeronaves;
extern CentralizedControlMechanism * centralized_control_mechanism;

// Sectors list fonctions 
Sector create_sector(int number_sectors);
void destroy_sectors(Sector * sectors); 
int insert_sector(Sector * sectors, Sector sector, int number_sectors);
Sector remove_sector(Sector * sectors, int number_sectors, int id_sector);
int is_empty_sectors(Sector * sectors, int number_sectors);
int is_full_sectors(Sector * sectors, int number_sectors);

// Aeronave functions
Aeronave create_aeronave(int number_aeronaves);
void destroy_aeronaves(Aeronave * aeronaves);
int request_sector(Aeronave * aeronave, int id_sector);
int wait_sector(Aeronave * aeronave);
int acquire_sector(Aeronave * aeronave, Sector * sector);
Sector* release_sector(Aeronave * aeronave);
int repeat(Aeronave * aeronave);

//RequestSector
RequestSector create_request(int number_requests);
void destroy_requests(RequestSector * requests);

// Sector MutexPriority functions
MutexPriority* create_mutex_priority(int max_size, int id);
void destroy_mutex_priority(MutexPriority * mutex_priority);
int order_list_by_priority(MutexPriority * mutex_priority); // max size is the number of aeronaves
void insert_aeronave_mutex_priority(MutexPriority * mutex_priority, Aeronave * aeronave);
Aeronave* remove_aeronave_mutex_priority(MutexPriority * mutex_priority);
int is_empty_mutex_priority(MutexPriority * mutex_priority);
int is_full_mutex_priority(MutexPriority * mutex_priority);

// CentralizedControlMechanism functions
CentralizedControlMechanism* create_centralized_control_mechanism(int aeronaves_number);
void destroy_centralized_control_mechanism(CentralizedControlMechanism * ccm);
Sector* control_priority(RequestSector* request, MutexPriority ** mutex_priorities, pthread_mutex_t * mutex_request);
int prevent_deadlock(RequestSector* requests, MutexPriority * mutex_priorities, int number_aeronaves); // not shure about the paramèters 

#endif