#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_TABLES 10
#define NUM_WAITERS 4
#define SEATS_PER_TABLE 6
#define MAX_CLIENTS 100

typedef struct {
    bool is_mixed_gender;
    int num_clients;
    pthread_t client_threads[4];
    int group_id;
    int table_id;
    pthread_mutex_t lock;
    pthread_cond_t allow_entry;
    bool is_waiting;
} Group;

typedef struct {
    bool mixed_gender;
    int waiter_id;
    int occupied_seats;
    int paid;
    pthread_mutex_t lock;
    pthread_cond_t wait_for_change;
} Table;

Table tables[NUM_TABLES];
Group groups[MAX_CLIENTS];

int group_count = 0;
int clients_handled= 0;

void* waiter_function(void* arg) {
    int waiter_id = *(int*)arg;
    return NULL;
}

void* client_function(void* arg) {
    int group_id = *(int*)arg;
    int table_id = -1;
    return NULL;
}


void projekt_zso(){

        pthread_t waiters[NUM_WAITERS];
    int waiter_ids[NUM_WAITERS];
    int client_ids[MAX_CLIENTS];
    int current_client = 0;

    // Inicjalizuj stoliki
    for (int i = 0; i < NUM_TABLES; i++) {
        pthread_mutex_init(&tables[i].lock, NULL);
        tables[i].waiter_id = -1;
        tables[i].paid = 0;
    }

    // Utwórz grupy
    for (int i = 0; i < MAX_CLIENTS;) {
        int group_size = (MAX_CLIENTS - i < 4) ? (MAX_CLIENTS - i) : (rand() % 4 + 1);
        groups[group_count].is_mixed_gender = (rand()%2 == 0) ? true : false;
        groups[group_count].num_clients = group_size;
        groups[group_count].group_id = group_count;
        groups[group_count].is_waiting = true;
        pthread_mutex_init(&groups[group_count].lock, NULL);

        for (int j = 0; j < group_size; j++) {
            client_ids[current_client] = group_count;
            pthread_create(&groups[group_count].client_threads[j], NULL, client_function, &client_ids[current_client]);
            current_client++;
        }
        group_count++;
        i+=group_size;
    }

    // Utwórz kelnerów
    for (int i = 0; i < NUM_WAITERS; i++) {
        waiter_ids[i] = i;
        pthread_create(&waiters[i], NULL, waiter_function, &waiter_ids[i]);
    }

    // Połącz klientów
    for (int i = 0; i < group_count; i++) {
        for (int j = 0; j < groups[i].num_clients; j++) {
            pthread_join(groups[i].client_threads[j], NULL);
        }
    }

    // Połącz kelnerów
    for (int i = 0; i < NUM_WAITERS; i++) {
        pthread_join(waiters[i], NULL);
    }


    printf("Scenariusz zakończony\n");
}

int main() {
    projekt_zso();

    return 0;
}

