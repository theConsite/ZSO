#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#define NUM_DATA 10
#define NUM_CHILDREN 3
#define PIPE_NAME "/tmp/stat_pipe"

typedef struct {
    long mtype;
    int data;
} message_t;

void leaf_process(int msgid, int id) {
    message_t msg;
    for (int i = 0; i < NUM_DATA; i++) {
        msg.mtype = 1;
        msg.data = rand() % 100; // Produkuj losowe dane
        msgsnd(msgid, &msg, sizeof(int), 0);
    }

    // Oczekiwanie na sygnał
    pause();

    // Wysłanie statystyk do named pipe
    int pipe_fd = open(PIPE_NAME, O_WRONLY);
    dprintf(pipe_fd, "Leaf %d sent %d messages\n", id, NUM_DATA);
    close(pipe_fd);

    exit(0);
}

void node_process(int msgid, int children_msgids[], int id) {
    int total_data = 0;
    message_t msg;

    for (int i = 0; i < NUM_CHILDREN; i++) {
        for (int j = 0; j < NUM_DATA; j++) {
            msgrcv(children_msgids[i], &msg, sizeof(int), 0, 0);
            total_data += msg.data;
            msgsnd(msgid, &msg, sizeof(int), 0);
        }
    }

    // Oczekiwanie na sygnał
    pause();

    // Wysłanie statystyk do named pipe
    int pipe_fd = open(PIPE_NAME, O_WRONLY);
    dprintf(pipe_fd, "Node %d received %d messages, total data: %d\n", id, NUM_CHILDREN * NUM_DATA, total_data);
    close(pipe_fd);

    exit(0);
}

void root_process(int msgid) {
    int total_data = 0;
    message_t msg;

    for (int i = 0; i < NUM_CHILDREN * NUM_DATA; i++) {
        msgrcv(msgid, &msg, sizeof(int), 0, 0);
        total_data += msg.data;
    }

    printf("Root received total data: %d\n", total_data);

    // Wysłanie sygnału do wszystkich procesów
    for (int i = 0; i < NUM_CHILDREN + 1; i++) {
        kill(0, SIGUSR1);
    }

    // Odczytanie statystyk z named pipe
    int pipe_fd = open(PIPE_NAME, O_RDONLY);
    char buffer[256];
    int bytes_read;
    while ((bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    close(pipe_fd);

    exit(0);
}

int main() {
    int msgid_root, msgid_nodes[NUM_CHILDREN], msgid_leaves[NUM_CHILDREN][NUM_CHILDREN];
    pid_t pids[NUM_CHILDREN + 1];
    key_t key = ftok("progfile", 65);

    msgid_root = msgget(key, 0666 | IPC_CREAT);

    // Tworzenie kolejek dla węzłów
    for (int i = 0; i < NUM_CHILDREN; i++) {
        key = ftok("progfile", 66 + i);
        msgid_nodes[i] = msgget(key, 0666 | IPC_CREAT);

        for (int j = 0; j < NUM_CHILDREN; j++) {
            key = ftok("progfile", 70 + i * NUM_CHILDREN + j);
            msgid_leaves[i][j] = msgget(key, 0666 | IPC_CREAT);
        }
    }

    mkfifo(PIPE_NAME, 0666);

    // Tworzenie procesów liści
    for (int i = 0; i < NUM_CHILDREN; i++) {
        for (int j = 0; j < NUM_CHILDREN; j++) {
            if ((pids[i] = fork()) == 0) {
                leaf_process(msgid_leaves[i][j], i * NUM_CHILDREN + j);
            }
        }
    }

    // Tworzenie procesów węzłów
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if ((pids[i] = fork()) == 0) {
            node_process(msgid_nodes[i], msgid_leaves[i], i);
        }
    }

    // Proces korzenia
    if ((pids[NUM_CHILDREN] = fork()) == 0) {
        root_process(msgid_root);
    }

    // Czekanie na zakończenie wszystkich procesów
    for (int i = 0; i < NUM_CHILDREN + 1; i++) {
        wait(NULL);
    }

    // Usuwanie kolejek komunikatów
    msgctl(msgid_root, IPC_RMID, NULL);
    for (int i = 0; i < NUM_CHILDREN; i++) {
        msgctl(msgid_nodes[i], IPC_RMID, NULL);
        for (int j = 0; j < NUM_CHILDREN; j++) {
            msgctl(msgid_leaves[i][j], IPC_RMID, NULL);
        }
    }

    unlink(PIPE_NAME);

    return 0;
}