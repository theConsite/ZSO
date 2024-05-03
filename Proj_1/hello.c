#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_TABLES 10
#define NUM_WAITERS 4
#define SEATS_PER_TABLE 6
#define MAX_CLIENTS 200

typedef struct {
    int num_clients;
    pthread_t client_threads[6];
    int group_id;
    int table_id;
    pthread_mutex_t lock;
    int status; //0-utworzona 1-oczekuje 2-usadzona 3-wychodzi, 4-wypuszczona
    pthread_cond_t is_let_go;
} Group;

typedef struct {
    int occupied_seats;
    pthread_mutex_t lock;
} Table;

Table tables[NUM_TABLES];
Group groups[MAX_CLIENTS];  // Grup może być mniej niż maksymalnie klientów
pthread_t payment_queue[MAX_CLIENTS];
int group_count = 0;
pthread_mutex_t groups_lock = PTHREAD_MUTEX_INITIALIZER;

void* waiter_function(void* arg) {
    int waiter_id = *(int*)arg;
    while (true) {
        for (int i = 0; i < group_count; i++) {
            pthread_mutex_lock(&groups[i].lock);
            if (groups[i].status == 1) {
                for (int j = 0; j < NUM_TABLES; j++) {
                    pthread_mutex_lock(&tables[j].lock);
                    if (tables[j].occupied_seats + groups[i].num_clients <= SEATS_PER_TABLE) {
                        tables[j].occupied_seats += groups[i].num_clients;
                        groups[i].table_id = j;
                        groups[i].status = 2;
                        printf("Kelner %d usadził grupe %d(%d klientów) przy stoliku %d. Łącznie osób przy stoliku: %d\n", waiter_id, groups[i].group_id, groups[i].num_clients, j, tables[j].occupied_seats);
                        pthread_mutex_unlock(&tables[j].lock);
                        break;
                    }
                    pthread_mutex_unlock(&tables[j].lock);
                }
            }else if(groups[i].status == 3){
                groups[i].status = 4;
                pthread_cond_signal(&groups[i].is_let_go);
            }else if(groups[i].status == 4 && groups[i].table_id != 10){
                printf("Stolik %d\n", groups[i].table_id);
                pthread_mutex_lock(&tables[groups[i].table_id].lock);
                tables[groups[i].table_id].occupied_seats -= groups[i].num_clients;
                groups[i].table_id = 10;
                pthread_mutex_unlock(&tables[groups[i].table_id].lock);
            }
            pthread_mutex_unlock(&groups[i].lock);
        }
    }

    return NULL;
}

void* client_function(void* arg) {
    int group_id = *(int*)arg;
    pthread_mutex_lock(&groups[group_id].lock);
    while (groups[group_id].status == 0) {
        groups[group_id].status = 1;
    }
    pthread_mutex_unlock(&groups[group_id].lock);
    while (groups[group_id].status == 1) {
        ;
    }
    pthread_mutex_lock(&groups[group_id].lock);
    while (groups[group_id].status == 2) {
        sleep(1);
        if(groups[group_id].status == 2){
            groups[group_id].status =3;
        }
    }
    pthread_mutex_unlock(&groups[group_id].lock);
    pthread_mutex_lock(&groups[group_id].lock);
    while (groups[group_id].status == 3) {
        pthread_cond_wait(&groups[group_id].is_let_go, &groups[group_id].lock);
        printf("Do widzenia, grupa %d\n", group_id);
        break;
    }
    pthread_mutex_unlock(&groups[group_id].lock);
    return NULL;
}

int main() {
    pthread_t waiters[NUM_WAITERS];
    int waiter_ids[NUM_WAITERS];
    int client_ids[MAX_CLIENTS];
    int current_client = 0;

    // Inicjalizuj stoliki
    for (int i = 0; i < NUM_TABLES; i++) {
        pthread_mutex_init(&tables[i].lock, NULL);
    }

    // Utwórz grupy
    for (int i = 0; i < MAX_CLIENTS; i += 6) {
        int group_size = (MAX_CLIENTS - i < 6) ? (MAX_CLIENTS - i) : (rand() % 6 + 1);
        groups[group_count].num_clients = group_size;
        groups[group_count].group_id = group_count;
        groups[group_count].status = 0;
        pthread_mutex_init(&groups[group_count].lock, NULL);

        for (int j = 0; j < group_size; j++) {
            client_ids[current_client] = group_count;
            pthread_create(&groups[group_count].client_threads[j], NULL, client_function, &client_ids[current_client]);
            current_client++;
        }
        group_count++;
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

    return 0;
}