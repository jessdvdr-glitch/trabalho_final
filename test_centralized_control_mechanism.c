#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "structures.h"

// Define the globals declared as extern in structures.h for the test
Sector *sectors = NULL;
Aeronave *aeronaves = NULL;
CentralizedControlMechanism *centralized_control_mechanism = NULL;

int main(void) {
    int number_aeronaves = 3;
    int number_sectors = 3;
    printf("[TEST] Starting centralized control mechanism tests\n");

    // Allocate sectors and aeronaves globals
    sectors = malloc(sizeof(Sector) * number_sectors);
    aeronaves = malloc(sizeof(Aeronave) * number_aeronaves);
    if (!sectors || !aeronaves) {
        printf("[TEST][ERROR] Failed to allocate globals\n");
        return 1;
    }
    for (int i = 0; i < number_sectors; ++i) sectors[i].id = i;
    for (int i = 0; i < number_aeronaves; ++i) { aeronaves[i].id = i; aeronaves[i].priority = i; }

    // Create centralized control mechanism
    centralized_control_mechanism = create_centralized_control_mechanism(number_aeronaves);
    if (!centralized_control_mechanism) {
        printf("[TEST][FAIL] create_centralized_control_mechanism returned NULL\n");
        free(sectors); free(aeronaves);
        return 1;
    }
    printf("[TEST][OK] create_centralized_control_mechanism returned non-NULL\n");

    // Test 1: control_priority with NULL request should return an error (in this implementation returns (Sector*)-1)
    Sector *res_null = control_priority(NULL, centralized_control_mechanism->mutex_sections, &centralized_control_mechanism->mutex_request);
    if (res_null == NULL) {
        printf("[TEST][OK] control_priority(NULL) returned expected error value\n");
    } else {
        printf("[TEST][WARN] control_priority(NULL) returned unexpected value (pointer %p)\n", (void*)res_null);
    }

    // Prepare a request for sector 1 by aircraft 2
    RequestSector req;
    req.id_sector = 1;
    req.id_aeronave = 2;

    // Test 2: control_priority when sector is free -> should acquire and return a Sector*
    Sector *res = control_priority(&req, centralized_control_mechanism->mutex_sections, &centralized_control_mechanism->mutex_request);
    if (res != NULL && res == &sectors[req.id_sector]) {
        printf("[TEST][OK] control_priority acquired sector successfully and returned correct pointer\n");
    } else {
        printf("[TEST][FAIL] control_priority did not acquire sector as expected (returned %p)\n", (void*)res);
    }

    // Test 3: make sector busy by locking its mutex, then call control_priority -> should queue the aircraft
    pthread_mutex_lock(&centralized_control_mechanism->mutex_sections[req.id_sector]->mutex_sector);
    size_t before_queue = centralized_control_mechanism->mutex_sections[req.id_sector]->waiting_list_size;
    Sector *res_busy = control_priority(&req, centralized_control_mechanism->mutex_sections, &centralized_control_mechanism->mutex_request);
    if (res_busy == NULL) {
        size_t after_queue = centralized_control_mechanism->mutex_sections[req.id_sector]->waiting_list_size;
        if (after_queue == before_queue + 1) {
            Aeronave *queued = centralized_control_mechanism->mutex_sections[req.id_sector]->waiting_list[before_queue];
            if (queued && queued->id == req.id_aeronave) {
                printf("[TEST][OK] control_priority queued aircraft %d when sector busy\n", req.id_aeronave);
            } else {
                printf("[TEST][FAIL] queued entry mismatch (expected id %d)\n", req.id_aeronave);
            }
        } else {
            printf("[TEST][FAIL] waiting_list_size did not increase as expected (before %zu after %zu)\n", before_queue, after_queue);
        }
    } else {
        printf("[TEST][FAIL] control_priority returned non-NULL when sector was locked (returned %p)\n", (void*)res_busy);
    }
    // unlock mutex we locked
    pthread_mutex_unlock(&centralized_control_mechanism->mutex_sections[req.id_sector]->mutex_sector);

    // Cleanup
    destroy_centralized_control_mechanism(centralized_control_mechanism);
    free(sectors);
    free(aeronaves);

    printf("[TEST] Finished tests\n");
    return 0;
}
