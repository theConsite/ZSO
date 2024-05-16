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
    pthread_cond_t is_let_go;
} Table;

pthread_mutex_t cash_lock;
pthread_cond_t cash;

Table tables[NUM_TABLES];
Group groups[MAX_CLIENTS];
int group_count = 0;
int clients_handled= 0;

void* waiter_function(void* arg) {
    int waiter_id = *(int*)arg;
    while (true) {
        for (int i = 0; i < group_count; i++) {
            if (groups[i].is_waiting) {
                for (int j = 0; j < NUM_TABLES; j++) {
                    if (tables[j].occupied_seats + groups[i].num_clients <= SEATS_PER_TABLE && !(tables[j].occupied_seats == 2 && tables[j].mixed_gender) && (tables[j].waiter_id == waiter_id || tables[j].waiter_id == -1) && tables[j].paid == 0) {
                        pthread_mutex_lock(&tables[j].lock);
                        tables[j].waiter_id = waiter_id;
                        tables[j].occupied_seats += groups[i].num_clients;
                        if(groups[i].is_mixed_gender){
                            tables[j].mixed_gender = true;
                        }

                        pthread_mutex_unlock(&tables[j].lock);
                        pthread_mutex_lock(&groups[i].lock);
                        groups[i].table_id = j;
                        groups[i].is_waiting = false;
                        pthread_mutex_unlock(&groups[i].lock);
                        #ifdef prints
                        printf("Kelner %d usadził grupe %d(%d klientów) przy stoliku %d. Łącznie osób przy stoliku: %d\n", waiter_id, groups[i].group_id, groups[i].num_clients, j, tables[j].occupied_seats);
                        #endif
                        pthread_cond_broadcast(&groups[i].allow_entry);
                        break;
                    } 
                }
            }
        }
        for(int i = 0; i < NUM_TABLES; i++){
            if(tables[i].waiter_id == waiter_id){
                if(tables[i].occupied_seats <= tables[i].paid && tables[i].occupied_seats > 0){
                    pthread_cond_broadcast(&tables[i].is_let_go);
                    tables[i].mixed_gender = false;
                }else if(tables[i].occupied_seats == 0){
                    #ifdef prints
                    printf("Stolik %d wolny, %d \n", i, clients_handled);
                    #endif
                    tables[i].mixed_gender = false;
                    tables[i].waiter_id = -1;
                    tables[i].paid = 0;
                }
                
            }
        }
        if(waiter_id == 0){
                
                #ifdef sleeps
                sleep(1);
                #endif
                #ifdef prints
                printf("Kelner idzie do kasy\n");
                #endif
                pthread_cond_broadcast(&cash);
            }
        if(clients_handled == MAX_CLIENTS){
            break;
        }
    }

    return NULL;
}

void* client_function(void* arg) {
    int group_id = *(int*)arg;
    int table_id = -1;
    pthread_mutex_lock(&groups[group_id].lock);
    pthread_cond_wait(&groups[group_id].allow_entry, &groups[group_id].lock);
    pthread_mutex_unlock(&groups[group_id].lock);
    table_id = groups[group_id].table_id;
    #ifdef prints
    printf("Klient z grupy %d wpuszczony do stolika %d\n", group_id,table_id);
    #endif
    #ifdef sleeps
                sleep(3);
                #endif
    #ifdef prints
    printf("Klient z stolika %d idzie zapłacić\n",table_id);
    #endif
    pthread_mutex_lock(&cash_lock);
    pthread_cond_wait(&cash, &cash_lock);
    pthread_mutex_unlock(&cash_lock);
    #ifdef prints
    printf("Klient z stolika %d zapłacił\n",table_id);
    #endif
    tables[table_id].paid ++;
    pthread_mutex_lock(&tables[table_id].lock);
    pthread_cond_wait(&tables[table_id].is_let_go, &groups[table_id].lock);
    pthread_mutex_unlock(&tables[table_id].lock);
    clients_handled++;
    tables[table_id].occupied_seats--;
    return NULL;
}

void projekt_zso(){
    pthread_t waiters[NUM_WAITERS];
    int waiter_ids[NUM_WAITERS];
    int client_ids[MAX_CLIENTS];
    int current_client = 0;

    pthread_mutex_init(&cash_lock, NULL);

    for (int i = 0; i < NUM_TABLES; i++) {
        pthread_mutex_init(&tables[i].lock, NULL);
        tables[i].waiter_id = -1;
        tables[i].paid = 0;
    }

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

    for (int i = 0; i < NUM_WAITERS; i++) {
        waiter_ids[i] = i;
        pthread_create(&waiters[i], NULL, waiter_function, &waiter_ids[i]);
    }

    for (int i = 0; i < group_count; i++) {
        for (int j = 0; j < groups[i].num_clients; j++) {
            pthread_join(groups[i].client_threads[j], NULL);
        }
    }

    for (int i = 0; i < NUM_WAITERS; i++) {
        pthread_join(waiters[i], NULL);
    }

    printf("Scenariusz zakończony\n");
}

int main() {
    projekt_zso();

    return 0;
}