#include "structures.h"
#include <stdio.h>
#include <stdlib.h>

// Sector functions
Sector create_sector(int id) {
    Sector s;
    s.id = id;
    return s;
}

void destroy_sectors(Sector * sectors) {
    if (sectors) free(sectors);
}

int insert_sector(Sector * sectors, Sector sector, int number_sectors) {
    (void)sectors; (void)sector; (void)number_sectors;
    return 0; // Placeholder
}

Sector remove_sector(Sector * sectors, int number_sectors, int id_sector) {
    (void)sectors; (void)number_sectors; (void)id_sector;
    Sector s = {-1};
    return s; // Placeholder
}

int is_empty_sectors(Sector * sectors, int number_sectors) {
    (void)sectors; (void)number_sectors;
    return 0; // Placeholder
}

int is_full_sectors(Sector * sectors, int number_sectors) {
    (void)sectors; (void)number_sectors;
    return 0; // Placeholder
}

// Aeronave functions
Aeronave create_aeronave(int id) {
    Aeronave a;
    a.id = id;
    a.priority = 0;
    a.rota = NULL;
    a.current_index_rota = 0;
    a.current_sector = NULL;
    a.aguardar = 0;
    return a;
}

void destroy_aeronaves(Aeronave * aeronaves) {
    if (aeronaves) free(aeronaves);
}

int request_sector(Aeronave * aeronave, int id_sector) {
    (void)aeronave; (void)id_sector;
    // 1 lock the global mutex mutex_request in the struct CentralizedControlMechanism
    // 2 set the request field in CentralizedControlMechanism to the desired request (id_sector, id_aeronave)
    // wait until the request is processed by the centralized control mechanism thread
    // unlock the mutex_request when you received the response
    return 0; // Placeholder
}

// if the response of the request is NULL, the aeronave must wait
int wait_sector(Aeronave * aeronave) {
    (void)aeronave;
    return 0; // Placeholder
}

// if the response of the request is a Sector*, the aeronave can acquire it
int acquire_sector(Aeronave * aeronave, Sector * sector) {
    (void)aeronave; (void)sector;
    return 0; // Placeholder
}

Sector* release_sector(Aeronave * aeronave) {
    (void)aeronave;
    return NULL; // Placeholder
}

int repeat(Aeronave * aeronave) {
    (void)aeronave;
    return 0; // Placeholder
}

RequestSector create_request(int number_requests) {
    (void)number_requests;
    RequestSector r;
    r.id_sector = 0;
    r.id_aeronave = 0;
    return r; // Placeholder
}

void destroy_requests(RequestSector * requests) {
    if (requests) free(requests);
}

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
    (void)mutex_priority; // Mark as intentionally unused
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


// Sector CentralizedControlMechanism functions
CentralizedControlMechanism* create_centralized_control_mechanism(int aeronaves_number) {
  CentralizedControlMechanism *ccm = malloc(sizeof(CentralizedControlMechanism));
  if (!ccm) return NULL;

  ccm->num_mutex_sections = aeronaves_number;
  ccm->mutex_sections = calloc(aeronaves_number, sizeof(MutexPriority*));
  if (!ccm->mutex_sections) {
    free(ccm);
    return NULL;
  }

  for (int i = 0; i < aeronaves_number; ++i) {
    ccm->mutex_sections[i] = create_mutex_priority(aeronaves_number, i);
    if (!ccm->mutex_sections[i]) {
      for (int j = 0; j < i; ++j) destroy_mutex_priority(ccm->mutex_sections[j]);
      free(ccm->mutex_sections);
      free(ccm);
      return NULL;
    }
  }

  ccm->request = NULL;
  if (pthread_mutex_init(&ccm->mutex_request, NULL) != 0) {
    for (int i = 0; i < aeronaves_number; ++i) destroy_mutex_priority(ccm->mutex_sections[i]);
    free(ccm->mutex_sections);
    free(ccm);
    return NULL;
  }

  return ccm;
}

void destroy_centralized_control_mechanism(CentralizedControlMechanism * ccm) {
  if (!ccm) return;
  for (int i = 0; i < ccm->num_mutex_sections; ++i) {
    destroy_mutex_priority(ccm->mutex_sections[i]);
  }
  free(ccm->mutex_sections);
  if (ccm->request) free(ccm->request);
  pthread_mutex_destroy(&ccm->mutex_request);
  free(ccm);
}


Sector* control_priority(RequestSector* request, MutexPriority ** mutex_priorities, 
                     pthread_mutex_t * mutex_request) {
    // Check if request pointer is NULL
    if (request == NULL) {
        printf("[CONTROL_PRIORITY] Error: request pointer is NULL. Exiting function.\n");
        return NULL;
    }

    // Attempt to lock the sector mutex using trylock (non-blocking)
    int lock_result = pthread_mutex_trylock(&mutex_priorities[request->id_sector]->mutex_sector);

    if (lock_result == 0) {
        // Sector is FREE: mutex acquired successfully
        printf("[CONTROL_PRIORITY] Aircraft %d acquired sector %d (lock successful).\n", 
               request->id_aeronave, request->id_sector);
        Sector * acquired_sector = &sectors[request->id_sector];         
        return acquired_sector; // send the new sector to the aeronave
    } else if (lock_result == EBUSY) {
        // Sector is BUSY: mutex could not be acquired
        printf("[CONTROL_PRIORITY] Sector %d is busy. Adding aircraft %d to waiting list.\n", 
               request->id_sector, request->id_aeronave);
        
        // Add the aircraft to the waiting list of this sector
        insert_aeronave_mutex_priority(mutex_priorities[request->id_sector], 
                                       &aeronaves[request->id_aeronave]);
        
        printf("[CONTROL_PRIORITY] Aircraft %d added to waiting list for sector %d.\n", 
               request->id_aeronave, request->id_sector);
        return NULL; // section not available : aeronave must wait
    } else {
        // Unexpected error from pthread_mutex_trylock
        printf("[CONTROL_PRIORITY] Error: pthread_mutex_trylock failed with code %d.\n", lock_result);
        return NULL;
    }
}
