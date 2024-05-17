#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_TABLES 10
#define NUM_WAITERS 4
#define SEATS_PER_TABLE 6
#define MAX_CLIENTS 100

#ifdef debug
#define PRINTF(...) printf(__VA_ARGS__)
#define SLEEP(x) sleep(x)
// #define SLEEP(x)
#else
#define PRINTF(...)
#define SLEEP(x)
#endif

typedef struct
{
    bool is_mixed_gender;
    int num_clients;
    pthread_t client_threads[4];
    int group_id;
    int table_id;
    pthread_mutex_t lock;
    pthread_cond_t allow_entry;
    bool is_waiting;
} Group;

typedef struct
{
    bool mixed_gender;
    int waiter_id;
    int occupied_seats;
    int ready_to_pay;
    pthread_mutex_t lock;
    pthread_cond_t wait_for_payment;
} Table;

Table tables[NUM_TABLES];
Group groups[MAX_CLIENTS];

int group_count = 0;

void *waiter_function(void *arg)
{
    int waiter_id = *(int *)arg;
    while (true)
    {
        bool no_groups_waiting = true;
        for (int i = 0; i < group_count; i++)
        {
            int grouplock = pthread_mutex_trylock(&groups[i].lock);
            if (grouplock == 0 && groups[i].is_waiting)
            {
                for (int j = 0; j < NUM_TABLES; j++)
                {
                    if (tables[j].occupied_seats + groups[i].num_clients <= SEATS_PER_TABLE && !(tables[j].occupied_seats == 2 && tables[j].mixed_gender) && tables[j].waiter_id == waiter_id)
                    {   

                        no_groups_waiting = false;
                        tables[j].occupied_seats += groups[i].num_clients;
                        
                        if (groups[i].is_mixed_gender)
                        {
                            tables[j].mixed_gender = true;
                        }
                        groups[i].table_id = j;
                        groups[i].is_waiting = false;
                        PRINTF("Kelner %d usadził grupe %d(%d klientów) przy stoliku %d. Łącznie osób przy stoliku: %d\n", waiter_id, groups[i].group_id, groups[i].num_clients, j, tables[j].occupied_seats);
                        pthread_cond_broadcast(&groups[i].allow_entry);
                        break;
                    }
                }
            }
            if(grouplock == 0){
                pthread_mutex_unlock(&groups[i].lock);
            }
            
        }
        int clients_at_tables = 0;
        for (int j = 0; j < NUM_TABLES; j++){
            if(tables[j].waiter_id == waiter_id){
                if (tables[j].ready_to_pay == tables[j].occupied_seats && tables[j].occupied_seats != 0){
                    pthread_cond_broadcast(&tables[j].wait_for_payment);
                    tables[j].mixed_gender = false;
                    tables[j].ready_to_pay = 0;
                    tables[j].occupied_seats = 0;
                    PRINTF("Stolik %d zwolniony\n", j);
                }else{
                    clients_at_tables += tables[j].occupied_seats;
                }
                
            }
        }
        if (no_groups_waiting && clients_at_tables == 0)
        {
            PRINTF("Kelner %d kończy zmiane\n",waiter_id);
            break;
        }
    }
    return NULL;
}

void *client_function(void *arg)
{
    int group_id = *(int *)arg;
    int table_id = -1;
    int grouplock = pthread_mutex_trylock(&groups[group_id].lock);
    while(grouplock ==-1){
        grouplock = pthread_mutex_trylock(&groups[group_id].lock);
    }
    pthread_cond_wait(&groups[group_id].allow_entry, &groups[group_id].lock);
    pthread_mutex_unlock(&groups[group_id].lock);
    table_id = groups[group_id].table_id;
    SLEEP(1);
    int tablelock = pthread_mutex_trylock(&tables[table_id].lock);
    while(tablelock ==-1){
        tablelock = pthread_mutex_trylock(&tables[table_id].lock);
    }
    tables[table_id].ready_to_pay+=1;
    pthread_cond_wait(&tables[table_id].wait_for_payment, &tables[table_id].lock);
    pthread_mutex_unlock(&tables[table_id].lock);
    return NULL;
}

void projekt_zso(int runcount)
{

    pthread_t waiters[NUM_WAITERS];
    int waiter_ids[NUM_WAITERS];
    int client_ids[MAX_CLIENTS];
    int current_client = 0;
    group_count = 0;

    // Inicjalizuj stoliki
    for (int i = 0; i < NUM_TABLES; i++)
    {
        pthread_mutex_init(&tables[i].lock, NULL);
        tables[i].waiter_id = i % NUM_WAITERS;
        tables[i].ready_to_pay = 0;
    }

    // Utwórz grupy
    for (int i = 0; i < MAX_CLIENTS;)
    {
        int group_size = (MAX_CLIENTS - i < 4) ? (MAX_CLIENTS - i) : (rand() % 4 + 1);
        groups[group_count].is_mixed_gender = (rand() % 2 == 0) ? true : false;
        groups[group_count].num_clients = group_size;
        groups[group_count].group_id = group_count;
        groups[group_count].table_id = -1;
        groups[group_count].is_waiting = true;
        pthread_mutex_init(&groups[group_count].lock, NULL);

        for (int j = 0; j < group_size; j++)
        {
            client_ids[current_client] = group_count;
            pthread_create(&groups[group_count].client_threads[j], NULL, client_function, &client_ids[current_client]);
            current_client++;
        }
        group_count++;
        i += group_size;
    }

    // Utwórz kelnerów
    for (int i = 0; i < NUM_WAITERS; i++)
    {
        waiter_ids[i] = i;
        pthread_create(&waiters[i], NULL, waiter_function, &waiter_ids[i]);
    }

    // Połącz klientów
    for (int i = 0; i < group_count; i++)
    {
        for (int j = 0; j < groups[i].num_clients; j++)
        {
            pthread_join(groups[i].client_threads[j], NULL);
        }
    }

    // Połącz kelnerów
    for (int i = 0; i < NUM_WAITERS; i++)
    {
        pthread_join(waiters[i], NULL);
    }

    PRINTF("Scenariusz %d zakończony\n", runcount);
}

int main()
{
    for (int i = 0; i < 10; i++){
        projekt_zso(i+1);
    }

    return 0;
}
