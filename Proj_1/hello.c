#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_TABLES 10
#define NUM_WAITERS 4
#define SEATS_PER_TABLE 6
#define MAX_CLIENTS 50

typedef struct {
    bool is_mixed_gender;
    int num_clients;
    pthread_t client_threads[6];
    int group_id;
    int table_id;
    pthread_mutex_t lock;
    int status; //1-oczekuje 2-usadzona 3-wypuszczona
    pthread_cond_t allow_entry;
} Group;

typedef struct {
    bool mixed_gender;
    int occupied_seats;
    int paid;
    pthread_mutex_t lock;
    pthread_cond_t is_let_go;
} Table;

pthread_mutex_t cash_lock;
pthread_cond_t cash;

Table tables[NUM_TABLES];
Group groups[MAX_CLIENTS];  // Grup może być mniej niż maksymalnie klientów
pthread_t payment_queue[MAX_CLIENTS];
int group_count = 0;
pthread_mutex_t groups_lock = PTHREAD_MUTEX_INITIALIZER;

void* waiter_function(void* arg) {
    int waiter_id = *(int*)arg;
    while (true) {
        for (int i = 0; i < group_count; i++) {
            if (groups[i].status == 1) {
                for (int j = 0; j < NUM_TABLES; j++) {
                    if (tables[j].occupied_seats + groups[i].num_clients <= SEATS_PER_TABLE && !(tables[j].occupied_seats == 2 && tables[j].mixed_gender)) {
                        pthread_mutex_lock(&tables[j].lock);
                        tables[j].occupied_seats += groups[i].num_clients;
                        if(groups[i].is_mixed_gender){
                            tables[j].mixed_gender = true;
                        }
                        pthread_mutex_unlock(&tables[j].lock);
                        pthread_mutex_lock(&groups[i].lock);
                        groups[i].table_id = j;
                        groups[i].status = 2;
                        pthread_mutex_unlock(&groups[i].lock);
                        pthread_cond_broadcast(&groups[i].allow_entry);
                        printf("Kelner %d usadził grupe %d(%d klientów) przy stoliku %d. Łącznie osób przy stoliku: %d\n", waiter_id, groups[i].group_id, groups[i].num_clients, j, tables[j].occupied_seats);
                        break;
                    } 
                }
            }else if (groups[i].status == 2 && tables[groups[i].table_id].paid == tables[groups[i].table_id].occupied_seats) {
                pthread_mutex_lock(&tables[groups[i].table_id].lock);
                    tables[groups[i].table_id].paid = 0;
                    tables[groups[i].table_id].mixed_gender = false;
                pthread_mutex_unlock(&tables[groups[i].table_id].lock);
                pthread_cond_broadcast(&tables[groups[i].table_id].is_let_go);
                printf("Kelner wypuścił klientów z stolika %d \n",groups[i].table_id);
            }
        }
        if(waiter_id == 1){
                sleep(1);
                printf("Kelner idzie do kasy\n");
                pthread_cond_broadcast(&cash);
            }
    }

    return NULL;
}

void* client_function(void* arg) {
    int group_id = *(int*)arg;
    if(groups[group_id].status == 1) {
        pthread_mutex_lock(&groups[group_id].lock);
        pthread_cond_wait(&groups[group_id].allow_entry, &groups[group_id].lock);
        pthread_mutex_unlock(&groups[group_id].lock);
        printf("Klient z grupy %d wpuszczony\n", group_id);
    }
    sleep(5);
    printf("Klient z grupy %d idzie zaplacic\n", group_id);
    pthread_mutex_lock(&cash_lock);
    pthread_cond_wait(&cash, &cash_lock);
    pthread_mutex_unlock(&cash_lock);
    pthread_mutex_lock(&tables[groups[group_id].table_id].lock);
    tables[groups[group_id].table_id].paid++;
    pthread_mutex_unlock(&tables[groups[group_id].table_id].lock);
    printf("Klient z grupy %d zaplacil ,%d %d\n", group_id, tables[groups[group_id].table_id].paid, tables[groups[group_id].table_id].occupied_seats);
    pthread_mutex_lock(&tables[groups[group_id].table_id].lock);
    pthread_cond_wait(&tables[groups[group_id].table_id].is_let_go, &groups[group_id].lock);
    pthread_mutex_unlock(&tables[groups[group_id].table_id].lock);
    printf("Klient z grupy %d wypuszczony\n", group_id);
    if(groups[group_id].status == 2){
        pthread_mutex_lock(&tables[groups[group_id].table_id].lock);
        tables[groups[group_id].table_id].occupied_seats =- groups[group_id].num_clients;
        pthread_mutex_unlock(&tables[groups[group_id].table_id].lock);
        pthread_mutex_lock(&groups[group_id].lock);
        groups[group_id].status = 3;
        groups[group_id].table_id = -1;
        pthread_mutex_unlock(&groups[group_id].lock);
    }
    return NULL;
}

int main() {
    pthread_t waiters[NUM_WAITERS];
    int waiter_ids[NUM_WAITERS];
    int client_ids[MAX_CLIENTS];
    int current_client = 0;

    pthread_mutex_init(&cash_lock, NULL);

    // Inicjalizuj stoliki
    for (int i = 0; i < NUM_TABLES; i++) {
        pthread_mutex_init(&tables[i].lock, NULL);
    }

    // Utwórz grupy
    for (int i = 0; i < MAX_CLIENTS; i += 6) {
        int group_size = (MAX_CLIENTS - i < 6) ? (MAX_CLIENTS - i) : (rand() % 6 + 1);
        groups[group_count].is_mixed_gender = (rand()%2 == 0) ? true : false;
        groups[group_count].num_clients = group_size;
        groups[group_count].group_id = group_count;
        groups[group_count].status = 1;
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