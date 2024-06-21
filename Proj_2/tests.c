#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct msgContents{
    long mtype;
    int number;
} msg;

void createFormattedString(int input, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "s22664msgq_%d", input);
}

unsigned int hash_string(const char *str) {
    unsigned int hash = 0;
    while (*str) {
        hash = (hash << 5) + *str++;
    }
    return hash;
}

void root(){
    int msgQueues[3];
    char buffer[20];
    for(int i = 0; i<3; i++){
        createFormattedString(i, buffer, sizeof(buffer));
        int key = hash_string(buffer);
        int msgcrt = msgget(key, IPC_CREAT | 0666);
        while(msgcrt == -1){
            int msgcrt = msgget(key, IPC_CREAT | 0666);
        }
        msgQueues[i] = msgcrt;
        printf("Utworzono kolejkę %s, %d\n", buffer, msgcrt);
    }
}

void node(int id){
    int childrenIds[3];
    int msgQueues[3];
    char buffer[20];

    // Calculate the children IDs
    for (int i = 0; i < 3; i++) {
        childrenIds[i] = id * 3 + i;
        createFormattedString(childrenIds[i], buffer, sizeof(buffer));
        int key = hash_string(buffer);
        int msgcrt = msgget(key, IPC_CREAT | 0666);
        while(msgcrt == -1){
            int msgcrt = msgget(key, IPC_CREAT | 0666);
        }
        msgQueues[i] = msgcrt;
        printf("Utworzono kolejkę %s\n", buffer);
    }
}

void leaf(int id){
    printf("leaf\n");
}

int main(int argc, char *argv[]) {
    int levels = 0;
    if (argc != 3) {
        levels = 2;
    }else{
        levels = atoi(argv[2]);
    }
    int id = atoi(argv[1]);
    int leaf_after = levels == 2?3:12;
    
    if(id == 0){
        root();
    }else if( id > leaf_after || levels == 1){
        leaf(id);
    }else{
        node(id);
    }
    return 0;
}