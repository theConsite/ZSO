#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_TABLES 10
#define NUM_WAITERS 4
#define SEATS_PER_TABLE 6
#define MAX_CLIENTS 66

#ifdef debug
#define PRINTF(...) printf(__VA_ARGS__)
#define SLEEP(x) sleep(x)
// #define SLEEP(x)
#else
#define PRINTF(...)
#define SLEEP(x)
#endif

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
    int ready_to_pay;
    bool is_exiting;
    pthread_mutex_t lock;
    pthread_cond_t wait_for_payment;
} Table;

Table tables[NUM_TABLES];
Group groups[MAX_CLIENTS];

int group_count = 0;
pthread_mutex_t group_count_lock = PTHREAD_MUTEX_INITIALIZER;

bool can_add_to_table(Table* table, Group* group, int waiterId) {
    bool can_add = table->occupied_seats + group->num_clients <= SEATS_PER_TABLE &&
                   !(table->occupied_seats == 2 && table->mixed_gender) &&
                   table->waiter_id == waiterId;
    return can_add;
}

bool is_assigned_to_table(Table* table, int waiterId) {
    bool assigned = table->waiter_id == waiterId;
    return assigned;
}

bool are_clients_ready_to_pay(Table* table) {
    bool ready = table->ready_to_pay == table->occupied_seats && table->occupied_seats != 0;
    return ready;
}

bool did_all_leave(Table* table) {
    bool free = table->occupied_seats == 0;
    return free;
}

bool is_table_assigned(Group* group) {
    return group->table_id != -1;
}

bool are_all_tables_serviced(int clientsAtTables){
    return clientsAtTables == 0;
}

void *waiter_function(void *arg) {
    int waiter_id = *(int *)arg;
    while (true) {
        bool no_groups_waiting = true;
        
        pthread_mutex_lock(&group_count_lock);
        int local_group_count = group_count;
        pthread_mutex_unlock(&group_count_lock);

        for (int i = 0; i < local_group_count; i++) {
            pthread_mutex_lock(&groups[i].lock);
            if (groups[i].is_waiting) {
                for (int j = 0; j < NUM_TABLES; j++) {
                    pthread_mutex_lock(&tables[j].lock);
                    if (can_add_to_table(&tables[j], &groups[i], waiter_id)) {

                        no_groups_waiting = false;
                        tables[j].occupied_seats += groups[i].num_clients;

                        if (groups[i].is_mixed_gender) {
                            tables[j].mixed_gender = true;
                        }
                        groups[i].table_id = j;
                        groups[i].is_waiting = false;
                        PRINTF("Kelner %d usadził grupe %d(%d klientów) przy stoliku %d. Łącznie osób przy stoliku: %d\n", waiter_id, groups[i].group_id, groups[i].num_clients, j, tables[j].occupied_seats);
                        pthread_cond_broadcast(&groups[i].allow_entry);
                        pthread_mutex_unlock(&tables[j].lock);
                        break;
                    }
                    pthread_mutex_unlock(&tables[j].lock);
                }
            }
            pthread_mutex_unlock(&groups[i].lock);
        }

        int clients_at_tables = 0;
        for (int j = 0; j < NUM_TABLES; j++) {
            pthread_mutex_lock(&tables[j].lock);
            if (is_assigned_to_table(&tables[j], waiter_id)) {
                if (are_clients_ready_to_pay(&tables[j])) {
                    pthread_cond_broadcast(&tables[j].wait_for_payment);
                    tables[j].is_exiting = true;
                    tables[j].mixed_gender = false;
                    tables[j].ready_to_pay = 0;
                    tables[j].is_exiting = false;
                    PRINTF("Stolik %d zwolniony\n", j);
                }
                clients_at_tables += tables[j].occupied_seats;
            }
            pthread_mutex_unlock(&tables[j].lock);
        }
        if (no_groups_waiting &&  are_all_tables_serviced(clients_at_tables)) {
            PRINTF("Kelner %d kończy zmiane\n",waiter_id);
            break;
        }
    }
    return NULL;
}


void *client_function(void *arg) {
    int group_id = *(int *)arg;
    int table_id = -1;

    pthread_mutex_lock(&groups[group_id].lock);
    if(!is_table_assigned(&groups[group_id])){
        pthread_cond_wait(&groups[group_id].allow_entry, &groups[group_id].lock);
    }
    table_id = groups[group_id].table_id;
    pthread_mutex_unlock(&groups[group_id].lock);

    SLEEP(1);

    pthread_mutex_lock(&tables[table_id].lock);
    tables[table_id].ready_to_pay += 1;
    pthread_cond_wait(&tables[table_id].wait_for_payment,&tables[table_id].lock);
    tables[table_id].occupied_seats -= 1;
    if(did_all_leave(&tables[table_id])){
        PRINTF("Stolik %d wychodzi \n", table_id);
    }
    pthread_mutex_unlock(&tables[table_id].lock);
    

    return NULL;
}

void projekt_zso(int runcount) {
    pthread_t waiters[NUM_WAITERS];
    int waiter_ids[NUM_WAITERS];
    int client_ids[MAX_CLIENTS];
    int current_client = 0;

    pthread_mutex_lock(&group_count_lock);
    group_count = 0;
    pthread_mutex_unlock(&group_count_lock);

    // Utwórz stoliki
    for (int i = 0; i < NUM_TABLES; i++) {
        pthread_mutex_init(&tables[i].lock, NULL);
        tables[i].waiter_id = i % NUM_WAITERS;
        tables[i].ready_to_pay = 0;
        tables[i].occupied_seats = 0;
        tables[i].mixed_gender = false;
        pthread_cond_init(&tables[i].wait_for_payment, NULL);
    }

    // Utwórz grupy
    for (int i = 0; i < MAX_CLIENTS;) {
        int group_size = (MAX_CLIENTS - i < 4) ? (MAX_CLIENTS - i) : (rand() % 4 + 1);
        pthread_mutex_lock(&group_count_lock);
        int group_id = group_count++;
        pthread_mutex_unlock(&group_count_lock);

        groups[group_id].is_mixed_gender = (rand() % 2 == 0);
        groups[group_id].num_clients = 0;
        groups[group_id].group_id = group_id;
        groups[group_id].table_id = -1;
        groups[group_id].is_waiting = true;
        pthread_mutex_init(&groups[group_id].lock, NULL);
        pthread_cond_init(&groups[group_id].allow_entry, NULL);

        for (int j = 0; j < group_size; j++) {
            client_ids[current_client] = group_id;
            pthread_create(&groups[group_id].client_threads[j], NULL, client_function, &client_ids[current_client]);
            current_client++;
            groups[group_id].num_clients += 1;
        }
        i += group_size;
    }

    // Utwórz kelnerów
    for (int i = 0; i < NUM_WAITERS; i++) {
        waiter_ids[i] = i;
        pthread_create(&waiters[i], NULL, waiter_function, &waiter_ids[i]);
    }

    // Łącz klientów
    for (int i = 0; i < group_count; i++) {
        for (int j = 0; j < groups[i].num_clients; j++) {
            pthread_join(groups[i].client_threads[j], NULL);
        }
    }

    // Łącz kelnerów
    for (int i = 0; i < NUM_WAITERS; i++) {
        pthread_join(waiters[i], NULL);
    }
    
    PRINTF("Scenariusz %d zakończony\n", runcount);
}

int main() {
    for (int i = 0; i < 10; i++) {
        projekt_zso(i + 1);
    }

    return 0;
}