#define _DEFAULT_SOURCE  // Enable usleep and other POSIX features
#include "structures.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // usleep
#include <string.h> // (may be useful)
#include <errno.h>   // EBUSY for pthread_mutex_trylock return
#include <time.h>  //sleep for random time

// variables for treating the deadlocks
extern Sector * aux_sector;
extern pthread_mutex_t aux_mutex;
extern int aux_var;

extern Sector **sectors;
extern Aeronave **aeronaves;
extern CentralizedControlMechanism *centralized_control_mechanism;


// Sector functions
Sector* create_sector(int id) {
    Sector* s = malloc(sizeof(Sector));
    s->id = id;
    s->busy = 0;
    s->id_aeronave_occupying = -1;
    return s;
}

void destroy_sector(Sector * sector) {
    if (sector) free(sector);
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
    Sector s = {-1, -1, -1}; // Invalid sector by default
    
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

// Aeronave functions
Aeronave* create_aeronave(int id, int priority, int tam_rota) {
    Aeronave* a = malloc(sizeof(Aeronave));
    if (!a) return NULL;
    
    a->id = id;
    a->priority = priority;
    a->tam_rota = tam_rota;
    a->current_index_rota = 0;
    a->aguardar = 0;
    
    // CRITICAL: Allocate memory for the rota array
    a->rota = malloc(sizeof(int) * tam_rota);
    if (!a->rota) {
        free(a);
        return NULL;
    }
    
    int num_sectors = centralized_control_mechanism->num_mutex_sections;
    a->rota[0] = rand() % num_sectors; // random starting sector
    int next;
    int i = 1;
    while(i < tam_rota){
        next = rand() % num_sectors;
        if(next != a->rota[i-1]){ // selects a random route, and two consecutive sectors have to be different
            a->rota[i] = next;
            i++;
        }
    }
    a->current_sector = NULL; //starting sector has to be undefined, because it has to wait for the permission of control 
    printf("Aeronave %d started, priority level: %d\n", a->id, a->priority);           // updated variable name
    printf("Route size: %d\n", a->tam_rota);
    for(int i = 0; i < a->tam_rota; i++){
            printf("%d -> ", a->rota[i]);
    }
    printf("\n");
    printf("\n");

    return a;
}

void destroy_aeronave(Aeronave * aeronave) {
    if (aeronave) {
        if (aeronave->rota) {
            free(aeronave->rota);  // Free the dynamically allocated rota array
        }
        free(aeronave);
    }
}

int request_sector(Aeronave * aeronave, int id_sector) {
    // NAO PRECISA DO MUTEX !! JA USADO NO ENQUEUE_REQUEST FONCTION
    // insert a struct request in the request queue with the focntion int enqueue_request(CentralizedControlMechanism * ccm, RequestSector * request);
    // wait until the request is processed by the centralized control mechanism thread
    RequestSector req;
    req.id_aeronave = aeronave->id;
    req.id_sector   = id_sector;
    req.request_type = 0;
    aeronave->aguardar = 1; // before sending request (if it requests before, ccm can change it's attribute before entering wait_sector function)
    return enqueue_request(centralized_control_mechanism, &req);
}

// if the response of the request is NULL, the aeronave must wait
int wait_sector(Aeronave * aeronave) {
    printf("\033[34m[AIRCRAFT %d] Started waiting\033[0m\n", aeronave->id);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    int result;

    result = sem_timedwait(&centralized_control_mechanism->semaphores_aeronaves[aeronave->id], &ts);
    if(result == -1){
        if(errno = ETIMEDOUT){
            flush_network(aeronave);
        }
        else{
            printf("\033[34m[AIRCRAFT %d] sem_timedwait error\033[0m\n", aeronave->id);
        }
    }
    else{
        printf("\033[34m[AIRCRAFT %d] Is free\033[0m\n", aeronave->id);
    }
    return 0;
}

// Pequena função interna para obter o próximo setor da rota (terminada com -1)
static int aeronave_next_sector_id(Aeronave *a) {
    if (!a || !a->rota) return -1;
    return a->rota[a->current_index_rota];
}


// if the system is clogged in deadlock, we "flush" it and release the network with this function
void flush_network(Aeronave * a){
    pthread_mutex_lock(&aux_mutex);
    if(aux_var == 0){
        printf("\033[34m[AIRCRAFT %d] DEADLOCK DETECTED! ENTERING BACKUP SECTOR\033[0m\n", a->id);
        aux_var = 1;
        // Request access to the backup sector
        request_sector(a, centralized_control_mechanism->num_mutex_sections); // request to enter backup
    
        // Wait CCM authorization 
        wait_sector(a);
        Sector* to_release = a->current_sector;
        
        // Acquire (trylock) after authorization it should be available
        while (!acquire_sector(a, sectors[centralized_control_mechanism->num_mutex_sections])) {
            usleep(100);
        }

        release_sector(a, to_release); // release the current one

        printf("\033[34m[AIRCRAFT %d] Entered sector backup sector (id = %d)\033[0m\n", a->id, a->current_sector->id);

        // wait a little, until the net is unclogged
        usleep(100);

    }
    pthread_mutex_unlock(&aux_mutex);
    if(a->id == sectors[centralized_control_mechanism->num_mutex_sections]->id_aeronave_occupying){ // if it has entered the backup, it asks to go to the sector it was trying to access
        int next_id = aeronave_next_sector_id(a);

        // Request access to the next sector
        request_sector(a, next_id);

    }
    // Wait CCM authorization 
    wait_sector(a);
}

// if the response of the request is a Sector*, the aeronave can acquire it
int acquire_sector(Aeronave * aeronave, Sector * sector) {
    if (!centralized_control_mechanism || !sector) return 0;
    int sid = sector->id;
    if (sid < 0 || sid >= centralized_control_mechanism->num_mutex_sections) return 0;

    MutexPriority *mp = centralized_control_mechanism->mutex_sections[sid];
    int rc = pthread_mutex_trylock(&mp->mutex_sector);
    if (rc == 0) {
        if(sector->id == centralized_control_mechanism->num_mutex_sections){
            printf("\033[34m[AIRCRAFT %d] Acquired backup sector (id = %d)\033[0m\n", aeronave->id, sector->id);
            aeronave->current_sector = sector;
        }
        else{
            printf("\033[34m[AIRCRAFT %d] Acquired sector %d\033[0m\n", aeronave->id, sector->id);
            aeronave->current_sector = sector;
            aeronave->current_index_rota++;
        }
        return 1;
    }
    return 0;
}

Sector* release_sector(Aeronave * aeronave, Sector* to_release) {
    if (!centralized_control_mechanism || !aeronave || !to_release) return NULL;
    int sid = to_release->id;
    if (sid < 0 || sid >= centralized_control_mechanism->num_mutex_sections) return NULL;

    MutexPriority *mp = centralized_control_mechanism->mutex_sections[sid];
    pthread_mutex_unlock(&mp->mutex_sector);
    // Only set current_sector to NULL if we're releasing the current sector
    if (aeronave->current_sector->id == to_release->id) {
        aeronave->current_sector = NULL;
    }
    RequestSector req;
    req.id_aeronave = aeronave->id;
    req.id_sector   = sid;
    req.request_type = 1;
    if(sid == centralized_control_mechanism->num_mutex_sections){
        printf("\033[34m[AIRCRAFT %d] Released backup sector (id = %d)\033[0m\n", aeronave->id, sid);
    }
    else{
        printf("\033[34m[AIRCRAFT %d] Released sector %d\033[0m\n", aeronave->id, sid);
    }
    enqueue_request(centralized_control_mechanism, &req); // sends a request warning that the sector is free
    return to_release;
}

int repeat(Aeronave * aeronave) {
    if (!aeronave || !aeronave->rota) return 0;
    return (aeronave->current_index_rota < aeronave->tam_rota); // if it was at the last sector, exit
}

// "Init + run": prepara estado e executa a rota completa da aeronave
void init_aeronave(Aeronave * aeronave) {
    if (!aeronave) return;

    if (aeronave->current_index_rota < 0) aeronave->current_index_rota = 0;

    while (repeat(aeronave)) {
        if(aeronave->current_sector != NULL){
            printf("\033[34m[AIRCRAFT %d] Currently at sector %d\033[0m\n", aeronave->id, aeronave->current_sector->id);
        }
        else{
            printf("\033[34m[AIRCRAFT %d] Route not started\033[0m\n", aeronave->id);
        }
        int next_id = aeronave_next_sector_id(aeronave);
        if (next_id < 0) break;

        // Request access to the next sector
        if (request_sector(aeronave, next_id) < 0) {
            sleep(1);
            continue;
        }

        // Wait CCM authorization 
        wait_sector(aeronave);
        Sector* to_release = aeronave->current_sector;
        
        // Acquire (trylock) after authorization it should be available
        while (!acquire_sector(aeronave, sectors[next_id])) {
            usleep(100);
        }

        // releases sector
        release_sector(aeronave, to_release);
        printf("\033[34m[AIRCRAFT %d] Entered sector %d\033[0m\n", aeronave->id, aeronave->current_sector->id);

        // Simulate using the sector for a random time
        usleep(1000 * (rand() % 5 + 1));

        // advance to next waypoint
        // printf("%d\n", aeronave->current_index_rota);
    }
    // Release last sector if we have one
    if (aeronave->current_sector != NULL) {
        printf("\033[34m[AIRCRAFT %d] Currently at sector %d, finishing route.\033[0m\n", aeronave->id, aeronave->current_sector->id);
        Sector* final_sector = aeronave->current_sector;
        release_sector(aeronave, final_sector);
        printf("\033[34m[AIRCRAFT %d] Left sector %d\033[0m\n", aeronave->id, final_sector->id);
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
    MutexPriority* mutex_priority = malloc(sizeof(MutexPriority));
    mutex_priority->id = id;
    mutex_priority->waiting_list = malloc(max_size * sizeof(Aeronave*));
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
    if(mutex_priority->waiting_list_size <= 0){
        return NULL;
    }
    Aeronave *out = mutex_priority->waiting_list[0]; // takes the first one
    for(int i = 0; i < mutex_priority->waiting_list_size - 1; i++){ // dislocate the next ones to the head of the queue
        mutex_priority->waiting_list[i] = mutex_priority->waiting_list[i+1];
    }
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
CentralizedControlMechanism* create_centralized_control_mechanism(int sectors_number, int aeronaves_number) {
    CentralizedControlMechanism *ccm = malloc(sizeof(CentralizedControlMechanism));
    if (!ccm) return NULL;

    ccm->num_mutex_sections = sectors_number;
    ccm->mutex_sections = malloc(sectors_number * sizeof(MutexPriority*));
    if (!ccm->mutex_sections) {
        free(ccm);
        return NULL;
    }

    for (int i = 0; i < sectors_number; ++i) {
        ccm->mutex_sections[i] = create_mutex_priority(aeronaves_number, i);
        if (!ccm->mutex_sections[i]) {
            for (int j = 0; j < i; ++j) destroy_mutex_priority(ccm->mutex_sections[j]);
            free(ccm->mutex_sections);
            free(ccm);
            return NULL;
        }
    }

    ccm->num_semaphores_aeronaves = aeronaves_number;
    ccm->semaphores_aeronaves = malloc(aeronaves_number * sizeof(sem_t));
    if (!ccm->semaphores_aeronaves) destroy_centralized_control_mechanism(ccm);
    for (int i = 0; i < aeronaves_number; ++i) {
        sem_init(&ccm->semaphores_aeronaves[i], 0, 0); // semaphore starts with zero, is useful to block a thread and let another one break it free
    }

    // Initialize request queue with large capacity
    ccm->request_queue_size = aeronaves_number; // Large queue capacity  Lucas: isn't the max number of requests == num_aeronaves?
    ccm->request_queue = malloc(ccm->request_queue_size * sizeof(RequestSector));
    if (!ccm->request_queue) {
        for (int i = 0; i < sectors_number; ++i) destroy_mutex_priority(ccm->mutex_sections[i]);
        free(ccm->mutex_sections);
        free(ccm);
        return NULL;
    }
    ccm->request_queue_front = 0;
    ccm->request_queue_rear = 0;
    ccm->request_queue_count = 0;

    if (pthread_mutex_init(&ccm->mutex_request, NULL) != 0) {
        for (int i = 0; i < sectors_number; ++i) destroy_mutex_priority(ccm->mutex_sections[i]);
        free(ccm->mutex_sections);
        free(ccm->request_queue);
        free(ccm);
        return NULL;
    }

  return ccm;
}

void destroy_centralized_control_mechanism(CentralizedControlMechanism * ccm) {
    if (!ccm) return;
    if(ccm->num_mutex_sections){
        for (int i = 0; i < ccm->num_mutex_sections; ++i) {
            destroy_mutex_priority(ccm->mutex_sections[i]);
        }
    }
    if(ccm->semaphores_aeronaves){
        for(int i = 0; i < ccm->num_semaphores_aeronaves; i++){
            sem_destroy(&ccm->semaphores_aeronaves[i]);
        }
        free(ccm->semaphores_aeronaves);
    }
    if(ccm->mutex_sections) free(ccm->mutex_sections);
    if(ccm->request_queue) free(ccm->request_queue);
    pthread_mutex_destroy(&ccm->mutex_request);
    free(ccm);
}

// Enqueue a request to the back of the queue
int enqueue_request(CentralizedControlMechanism * ccm, RequestSector * request) {
    if (!ccm || !request) return -1;
    
    pthread_mutex_lock(&ccm->mutex_request);
    
    // Check if queue is full
    if (ccm->request_queue_count >= ccm->request_queue_size) {
        printf("\033[33m[ENQUEUE] Error: Request queue is full. Cannot enqueue request.\033[0m\n");
        pthread_mutex_unlock(&ccm->mutex_request);
        return -1;
    }
    
    // Add request to the rear of the queue
    ccm->request_queue[ccm->request_queue_rear] = *request;
    ccm->request_queue_rear = (ccm->request_queue_rear + 1) % ccm->request_queue_size;
    ccm->request_queue_count++;
    if(request->request_type == 0){
        if(request->id_sector == ccm->num_mutex_sections){
            printf("\033[33m[ENQUEUE] Request queued. Aircraft %d wants to enter BACKUP Sector (id = %d). Queue size: %d\033[0m\n",
                   request->id_aeronave, request->id_sector, ccm->request_queue_count);
        }
        else{
            printf("\033[33m[ENQUEUE] Request queued. Aircraft %d wants to enter Sector %d. Queue size: %d\033[0m\n",
                   request->id_aeronave, request->id_sector, ccm->request_queue_count);
        }
    }
    else{
        if(request->id_sector == ccm->num_mutex_sections){
            printf("\033[33m[ENQUEUE] Request queued. Aircraft %d wants to leave BACKUP Sector (id = %d). Queue size: %d\033[0m\n",
               request->id_aeronave, request->id_sector, ccm->request_queue_count);
        }
        else{
            printf("\033[33m[ENQUEUE] Request queued. Aircraft %d wants to leave Sector %d. Queue size: %d\033[0m\n",
                   request->id_aeronave, request->id_sector, ccm->request_queue_count);
        }
    }
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
    
    if(request->request_type == 0){
        printf("\033[33m[DEQUEUE] Request dequeued. Aircraft %d wants to enter Sector %d. Remaining: %d\033[0m\n", request->id_aeronave, request->id_sector, ccm->request_queue_count);
    }
    else{
        printf("\033[33m[DEQUEUE] Request dequeued. Aircraft %d wants to leave Sector %d. Remaining: %d\033[0m\n", request->id_aeronave, request->id_sector, ccm->request_queue_count);
    }
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
        printf("\033[31m[CONTROL_PRIORITY] Error: request pointer is NULL. Exiting function.\033[0m\n");
        return NULL;
    }

    int is_busy = sectors[request->id_sector]->busy;

    if(request->request_type == 0){ // if it's to ask for entrance
        if (is_busy == 0) {
            // Sector is FREE: mutex acquired successfully (probe only)
            printf("\033[31m[CONTROL_PRIORITY] Aircraft %d acquired sector %d (lock successful).\033[0m\n",
                request->id_aeronave, request->id_sector);
            
            // there parameters are changed to tell the sector is busy
            sectors[request->id_sector]->busy = 1;
            sectors[request->id_sector]->id_aeronave_occupying = request->id_aeronave;
            
            // Wake the aircraft; it will perform the actual acquire_sector() trylock
            sem_post(&centralized_control_mechanism->semaphores_aeronaves[request->id_aeronave]);

            // Informative pointer returned
            return sectors[request->id_sector];
        } 
        else if (is_busy == 1) {
            printf("[CONTROL_PRIORITY] Sector %d is occupied by aircraft %d. Adding aircraft %d to waiting list.\n", 
                request->id_sector, sectors[request->id_sector]->id_aeronave_occupying, request->id_aeronave);
            
            insert_aeronave_mutex_priority(
                mutex_priorities[request->id_sector], aeronaves[request->id_aeronave]
            );

            printf("\033[31m[CONTROL_PRIORITY] Aircraft %d added to waiting list for sector %d.\033[0m\n", 
                request->id_aeronave, request->id_sector);

            return NULL;
        } 
        else {
            printf("\033[31m[CONTROL_PRIORITY] Error: pthread_mutex_trylock failed with code %d.\033[0m\n", is_busy);
            return NULL;
        }
    }
    else{ // if the request is a flag from the aeronave that has just released the sector, it dequeues it from that sector and wakes the waiting aeronave
        int id_sector = request->id_sector;
        Aeronave *released = remove_aeronave_mutex_priority(mutex_priorities[id_sector]);
        if(released != NULL){
            printf("\033[31m[CONTROL_PRIORITY] Aircraft %d released sector %d. Aircraft %d is now free to go.\033[0m\n", request->id_aeronave, id_sector, released->id);
            sem_post(&centralized_control_mechanism->semaphores_aeronaves[released->id]);
            sectors[request->id_sector]->id_aeronave_occupying = released->id;
        }
        else{
            printf("\033[31m[CONTROL_PRIORITY] Aircraft %d released sector %d.\033[0m\n", request->id_aeronave, id_sector);
            sectors[request->id_sector]->busy = 0;
            sectors[request->id_sector]->id_aeronave_occupying = -1;
        }
        if(id_sector == centralized_control_mechanism->num_mutex_sections){
            aux_var = 0;
        }
        printf("\033[34m[AIRCRAFT %d] Left sector %d\033[0m\n", request->id_aeronave, id_sector);
        return sectors[id_sector];
    }
}
