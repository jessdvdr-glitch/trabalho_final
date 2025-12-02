#include "structures.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // usleep
#include <string.h> // (may be useful)
#include <errno.h>   // EBUSY for pthread_mutex_trylock return

// Sector functions
// This function creates an individual sector with a given id
Sector create_sector(int id) {
    Sector s;
    s.id = id;
    return s;
}

// Frees the dynamically allocated memory for the sectors array
void destroy_sectors(Sector * sectors) {
    if (sectors != NULL) {
        free(sectors);
    }
}

// Inserts a specific sector in the list at index sector.id
// Returns 0 on success, -1 on error
int insert_sector(Sector * sectors, Sector sector) {
    if (sectors == NULL) {
        printf("[INSERT_SECTOR] Error: sectors array is NULL\n");
        return -1;
    }
    
    // The sector is inserted directly at the index corresponding to its ID
    sectors[sector.id] = sector;
    printf("[INSERT_SECTOR] Sector %d inserted successfully\n", sector.id);
    return 0;
}

// Removes a sector whose id is passed as parameter and returns it
// Returns a sector with id = -1 if error
Sector remove_sector(Sector * sectors, int number_sectors, int id_sector) {
    Sector s = {-1}; // Invalid sector by default
    
    // Check that the list is not empty
    if (is_empty_sectors(sectors, number_sectors)) {
        printf("[REMOVE_SECTOR] Error: sectors list is empty\n");
        return s;
    }
    
    if (sectors == NULL) {
        printf("[REMOVE_SECTOR] Error: sectors array is NULL\n");
        return s;
    }
    
    // Check that the id is valid
    if (id_sector < 0 || id_sector >= number_sectors) {
        printf("[REMOVE_SECTOR] Error: invalid sector id %d\n", id_sector);
        return s;
    }
    
    // Get the sector before marking it as invalid
    s = sectors[id_sector];
    
    // Mark the sector as removed by setting its id to -1
    sectors[id_sector].id = -1;
    
    printf("[REMOVE_SECTOR] Sector %d removed successfully\n", id_sector);
    return s;
}

// Checks if the sectors list is empty (all sectors have id = -1)
// Returns 1 if empty, 0 otherwise
int is_empty_sectors(Sector * sectors, int number_sectors) {
    if (sectors == NULL) {
        return 1; // Considered empty if NULL
    }
    
    // Iterate through all sectors to check if there is at least one valid
    for (int i = 0; i < number_sectors; i++) {
        if (sectors[i].id != -1) {
            return 0; // At least one valid sector found
        }
    }
    
    return 1; // All sectors are invalid (id = -1)
}

// Checks if the sectors list is full (all sectors have a valid id >= 0)
// Returns 1 if full, 0 otherwise
int is_full_sectors(Sector * sectors, int number_sectors) {
    if (sectors == NULL) {
        return 0; // Considered not full if NULL
    }
    
    // Iterate through all sectors to check if they are all valid
    for (int i = 0; i < number_sectors; i++) {
        if (sectors[i].id == -1) {
            return 0; // At least one invalid sector found
        }
    }
    
    return 1; // All sectors are valid
}

extern Sector *sectors;
extern Aeronave *aeronaves;
extern CentralizedControlMechanism *centralized_control_mechanism;

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
    // NAO PRECISA DO MUTEX !! JA USADO NO ENQUEUE_REQUEST FONCTION
    // insert a struct request in the request queue with the focntion int enqueue_request(CentralizedControlMechanism * ccm, RequestSector * request);
    // wait until the request is processed by the centralized control mechanism thread
    RequestSector req;
    req.id_aeronave = aeronave->id;
    req.id_sector   = id_sector;
    return enqueue_request(centralized_control_mechanism, &req);
}

// if the response of the request is NULL, the aeronave must wait
int wait_sector(Aeronave * aeronave) {
    aeronave->aguardar = 1;
    while (aeronave->aguardar) {
        // usleep(1000);
    }
    return 0;
}

// if the response of the request is a Sector*, the aeronave can acquire it
int acquire_sector(Aeronave * aeronave, Sector * sector) {
    if (!centralized_control_mechanism || !sector) return 0;
    int sid = sector->id;
    if (sid < 0 || sid >= centralized_control_mechanism->num_mutex_sections) return 0;

    MutexPriority *mp = centralized_control_mechanism->mutex_sections[sid];
    int rc = pthread_mutex_trylock(&mp->mutex_sector);
    if (rc == 0) {
        aeronave->current_sector = sector;
        return 1;
    }
    return 0;
}

Sector* release_sector(Aeronave * aeronave) {
    if (!centralized_control_mechanism || !aeronave || !aeronave->current_sector) return NULL;
    int sid = aeronave->current_sector->id;
    if (sid < 0 || sid >= centralized_control_mechanism->num_mutex_sections) return NULL;

    MutexPriority *mp = centralized_control_mechanism->mutex_sections[sid];
    pthread_mutex_unlock(&mp->mutex_sector);
    Sector *released = aeronave->current_sector;
    aeronave->current_sector = NULL;
    return released; // Placeholder
}

int repeat(Aeronave * aeronave) {
    if (!aeronave || !aeronave->rota) return 0;
    int next = aeronave->rota[aeronave->current_index_rota];
    return (next >= 0);
}

// Pequena função interna para obter o próximo setor da rota (terminada com -1)
static int aeronave_next_sector_id(Aeronave *a) {
    if (!a || !a->rota) return -1;
    return a->rota[a->current_index_rota];
}

// "Init + run": prepara estado e executa a rota completa da aeronave
void init_aeronave(Aeronave * aeronave) {
    if (!aeronave) return;

    if (aeronave->current_index_rota < 0) aeronave->current_index_rota = 0;

    while (repeat(aeronave)) {
        int next_id = aeronave_next_sector_id(aeronave);
        if (next_id < 0) break;

        // Request access to the next sector
        if (request_sector(aeronave, next_id) < 0) {
            sleep(1);
            continue;
        }

        // Wait CCM authorization 
        wait_sector(aeronave);

        // Acquire (trylock) after authorization it should be available
        while (!acquire_sector(aeronave, &sectors[next_id])) {
            sleep(1);
        }

        // Simulate using the sector
        sleep(2);

        // Release and advance to next waypoint
        release_sector(aeronave);
        aeronave->current_index_rota++;
    }
}

RequestSector create_request(int number_sectors, int number_aeronaves) {
    RequestSector r;
    r.id_sector = number_sectors;
    r.id_aeronave = number_aeronaves;
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
CentralizedControlMechanism* create_centralized_control_mechanism(int sections_number) {
  CentralizedControlMechanism *ccm = malloc(sizeof(CentralizedControlMechanism));
  if (!ccm) return NULL;

  ccm->num_mutex_sections = sections_number;
  ccm->mutex_sections = calloc(sections_number, sizeof(MutexPriority*));
  if (!ccm->mutex_sections) {
    free(ccm);
    return NULL;
  }

  for (int i = 0; i < sections_number; ++i) {
    ccm->mutex_sections[i] = create_mutex_priority(sections_number, i);
    if (!ccm->mutex_sections[i]) {
      for (int j = 0; j < i; ++j) destroy_mutex_priority(ccm->mutex_sections[j]);
      free(ccm->mutex_sections);
      free(ccm);
      return NULL;
    }
  }

  // Initialize request queue with large capacity
  ccm->request_queue_size = sections_number * 10; // Large queue capacity
  ccm->request_queue = calloc(ccm->request_queue_size, sizeof(RequestSector));
  if (!ccm->request_queue) {
    for (int i = 0; i < sections_number; ++i) destroy_mutex_priority(ccm->mutex_sections[i]);
    free(ccm->mutex_sections);
    free(ccm);
    return NULL;
  }
  ccm->request_queue_front = 0;
  ccm->request_queue_rear = 0;
  ccm->request_queue_count = 0;

  if (pthread_mutex_init(&ccm->mutex_request, NULL) != 0) {
    for (int i = 0; i < sections_number; ++i) destroy_mutex_priority(ccm->mutex_sections[i]);
    free(ccm->mutex_sections);
    free(ccm->request_queue);
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
  free(ccm->request_queue);
  pthread_mutex_destroy(&ccm->mutex_request);
  free(ccm);
}

// Enqueue a request to the back of the queue
int enqueue_request(CentralizedControlMechanism * ccm, RequestSector * request) {
    if (!ccm || !request) return -1;
    
    pthread_mutex_lock(&ccm->mutex_request);
    
    // Check if queue is full
    if (ccm->request_queue_count >= ccm->request_queue_size) {
        printf("[ENQUEUE] Error: Request queue is full. Cannot enqueue request.\n");
        pthread_mutex_unlock(&ccm->mutex_request);
        return -1;
    }
    
    // Add request to the rear of the queue
    ccm->request_queue[ccm->request_queue_rear] = *request;
    ccm->request_queue_rear = (ccm->request_queue_rear + 1) % ccm->request_queue_size;
    ccm->request_queue_count++;
    
    printf("[ENQUEUE] Request queued. Aircraft %d for Sector %d. Queue size: %d\n",
           request->id_aeronave, request->id_sector, ccm->request_queue_count);
    
    pthread_mutex_unlock(&ccm->mutex_request);
    return 0;
}

// Dequeue a request from the front of the queue
RequestSector* dequeue_request(CentralizedControlMechanism * ccm) {
    if (!ccm) return NULL;
    
    pthread_mutex_lock(&ccm->mutex_request);
    
    // Check if queue is empty
    if (ccm->request_queue_count == 0) {
        pthread_mutex_unlock(&ccm->mutex_request);
        return NULL;
    }
    
    // Get request from front of queue
    RequestSector * request = &ccm->request_queue[ccm->request_queue_front];
    ccm->request_queue_front = (ccm->request_queue_front + 1) % ccm->request_queue_size;
    ccm->request_queue_count--;
    
    printf("[DEQUEUE] Request dequeued. Aircraft %d for Sector %d. Remaining: %d\n",
           request->id_aeronave, request->id_sector, ccm->request_queue_count);
    
    pthread_mutex_unlock(&ccm->mutex_request);
    return request;
}

// Check if queue is empty (must be called with mutex locked)
int is_request_queue_empty(CentralizedControlMechanism * ccm) {
    return ccm->request_queue_count == 0 ? 1 : 0;
}


Sector* control_priority(RequestSector* request, MutexPriority ** mutex_priorities, 
                         pthread_mutex_t * mutex_request) {
    (void)mutex_request; // Mark as intentionally unused

    if (request == NULL) {
        printf("[CONTROL_PRIORITY] Error: request pointer is NULL. Exiting function.\n");
        return NULL;
    }

    int lock_result = pthread_mutex_trylock(&mutex_priorities[request->id_sector]->mutex_sector);

    if (lock_result == 0) {
        // Sector is FREE: mutex acquired successfully (probe only)
        printf("[CONTROL_PRIORITY] Aircraft %d acquired sector %d (lock successful).\n",
               request->id_aeronave, request->id_sector);

        // Immediately unlock (CCM must NOT keep the mutex locked)
        pthread_mutex_unlock(&mutex_priorities[request->id_sector]->mutex_sector);

        // Wake the aircraft; it will perform the actual acquire_sector() trylock
        aeronaves[request->id_aeronave].aguardar = 0;

        // Informative pointer returned
        return &sectors[request->id_sector];
    } 
    else if (lock_result == EBUSY) {
        printf("[CONTROL_PRIORITY] Sector %d is busy. Adding aircraft %d to waiting list.\n", 
               request->id_sector, request->id_aeronave);

        insert_aeronave_mutex_priority(
            mutex_priorities[request->id_sector], &aeronaves[request->id_aeronave]
        );

        printf("[CONTROL_PRIORITY] Aircraft %d added to waiting list for sector %d.\n", 
               request->id_aeronave, request->id_sector);

        return NULL;
    } 
    else {
        printf("[CONTROL_PRIORITY] Error: pthread_mutex_trylock failed with code %d.\n", lock_result);
        return NULL;
    }
}
