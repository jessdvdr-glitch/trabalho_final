#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdint.h>
#include <pthread.h>

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
    pthread_mutex_t mutex_sector; // /!\ use only pthread_mutex_try_lock() 
    Aeronave * waiting_list;
}MutexPriority;

typedef struct{
    pthread_mutex_t mutex_request;
}CentralizedControlMechanism;

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
MutexPriority create_mutex_priority(MutexPriority * mutex_priority, int max_size);
void destroy_mutex_priority(MutexPriority * mutex_priority);
int order_by_priority(Aeronave * waiting_list, int size); // max size is the number of aeronaves
void insert_aeronave_mutex_priority(MutexPriority * mutex_priority, Aeronave aeronave, int max_size);
Aeronave remove_aeronave_mutex_priority(MutexPriority * mutex_priority, int max_size);
int is_empty_mutex_priority(MutexPriority * mutex_priority);
int is_full_mutex_priority(MutexPriority * mutex_priority, int max_size);

// CentralizedControlMechanism functions
CentralizedControlMechanism create_centralized_control_mechanism();
void destroy_centralized_control_mechanism(CentralizedControlMechanism * ccm);
int control_priority(RequestSector* requests, MutexPriority * mutex_priorities, int number_aeronaves);
int prevent_deadlock(RequestSector* requests, MutexPriority * mutex_priorities, int number_aeronaves); // not shure about the paramèters 
Sector* get_next_sector(Aeronave * aeronave, Sector * sectors, int number_sectors);

#endif