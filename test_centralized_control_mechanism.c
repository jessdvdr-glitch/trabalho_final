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
    printf("[TEST] Starting centralized control mechanism tests with request queue\n");

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
    printf("[TEST][OK] Request queue size: %d\n", centralized_control_mechanism->request_queue_size);

    // Test 1: Test enqueue_request - add multiple requests to queue
    printf("\n[TEST] Test 1: Enqueue multiple requests\n");
    RequestSector req1, req2, req3;
    req1.id_sector = 0; req1.id_aeronave = 0;
    req2.id_sector = 1; req2.id_aeronave = 1;
    req3.id_sector = 2; req3.id_aeronave = 2;
    
    if (enqueue_request(centralized_control_mechanism, &req1) == 0) {
        printf("[TEST][OK] Enqueued request 1\n");
    } else {
        printf("[TEST][FAIL] Failed to enqueue request 1\n");
    }
    
    if (enqueue_request(centralized_control_mechanism, &req2) == 0) {
        printf("[TEST][OK] Enqueued request 2\n");
    } else {
        printf("[TEST][FAIL] Failed to enqueue request 2\n");
    }
    
    if (enqueue_request(centralized_control_mechanism, &req3) == 0) {
        printf("[TEST][OK] Enqueued request 3\n");
    } else {
        printf("[TEST][FAIL] Failed to enqueue request 3\n");
    }
    
    if (centralized_control_mechanism->request_queue_count == 3) {
        printf("[TEST][OK] Queue count is correct: %d\n", centralized_control_mechanism->request_queue_count);
    } else {
        printf("[TEST][FAIL] Queue count is incorrect: %d (expected 3)\n", centralized_control_mechanism->request_queue_count);
    }

    // Test 2: Test dequeue_request - FIFO order
    printf("\n[TEST] Test 2: Dequeue requests in FIFO order\n");
    RequestSector *dequeued1 = dequeue_request(centralized_control_mechanism);
    if (dequeued1 && dequeued1->id_aeronave == 0 && dequeued1->id_sector == 0) {
        printf("[TEST][OK] Dequeued first request correctly (Aircraft 0, Sector 0)\n");
    } else {
        printf("[TEST][FAIL] Dequeued first request incorrect\n");
    }
    
    RequestSector *dequeued2 = dequeue_request(centralized_control_mechanism);
    if (dequeued2 && dequeued2->id_aeronave == 1 && dequeued2->id_sector == 1) {
        printf("[TEST][OK] Dequeued second request correctly (Aircraft 1, Sector 1)\n");
    } else {
        printf("[TEST][FAIL] Dequeued second request incorrect\n");
    }
    
    if (centralized_control_mechanism->request_queue_count == 1) {
        printf("[TEST][OK] Queue count after 2 dequeues: %d\n", centralized_control_mechanism->request_queue_count);
    } else {
        printf("[TEST][FAIL] Queue count is incorrect: %d (expected 1)\n", centralized_control_mechanism->request_queue_count);
    }

    // Test 3: Test empty queue
    printf("\n[TEST] Test 3: Test empty queue behavior\n");
    dequeue_request(centralized_control_mechanism); // Remove last element
    RequestSector *empty_result = dequeue_request(centralized_control_mechanism); // Try to dequeue from empty queue
    if (empty_result == NULL) {
        printf("[TEST][OK] Dequeue from empty queue returned NULL\n");
    } else {
        printf("[TEST][FAIL] Dequeue from empty queue returned non-NULL\n");
    }
    
    if (is_request_queue_empty(centralized_control_mechanism)) {
        printf("[TEST][OK] Queue is empty as expected\n");
    } else {
        printf("[TEST][FAIL] Queue should be empty but isn't\n");
    }

    // Test 4: Test control_priority with dequeued request
    printf("\n[TEST] Test 4: Test control_priority with requests from queue\n");
    RequestSector req_for_priority;
    req_for_priority.id_sector = 1;
    req_for_priority.id_aeronave = 1;
    
    Sector *res = control_priority(&req_for_priority, centralized_control_mechanism->mutex_sections, &centralized_control_mechanism->mutex_request);
    if (res != NULL && res == &sectors[req_for_priority.id_sector]) {
        printf("[TEST][OK] control_priority acquired sector successfully\n");
    } else {
        printf("[TEST][FAIL] control_priority did not acquire sector\n");
    }

    // Cleanup
    destroy_centralized_control_mechanism(centralized_control_mechanism);
    free(sectors);
    free(aeronaves);

    printf("\n[TEST] All tests completed\n");
    return 0;
}

