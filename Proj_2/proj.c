#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdarg.h>


#define FIFO_NAME "/tmp/s22664_fifo"
#define BUFFER_SIZE 512
#ifdef debug
#define PRINTF(...) printf(__VA_ARGS__)
#define SLEEP(x) sleep(x)
#else
#define PRINTF(...)
#define SLEEP(x)
#endif

bool read_files = true;
pthread_mutex_t read_lock = PTHREAD_MUTEX_INITIALIZER;

char *format_string(const char *format, ...) {
    size_t buffer_size = 256;
    char *buffer = malloc(buffer_size);
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, buffer_size, format, args);
    va_end(args);



    return buffer;
}

void create_fifo() {
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
}

void write_to_fifo(const char *message) {
    int fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        fd = open(FIFO_NAME, O_WRONLY);
    }
    write(fd, message, strlen(message));
    close(fd);
}

void *read_from_fifo() {
    char buffer[BUFFER_SIZE];
    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        fd = open(FIFO_NAME, O_RDONLY);
    }
    while (1) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            PRINTF("Korzen odebral przez FIFO: %s\n", buffer);
        }
        pthread_mutex_lock(&read_lock);
        if(read_files){
            pthread_mutex_unlock(&read_lock);
        }else{
            pthread_mutex_unlock(&read_lock);
            break;
        }
    }
    close(fd);
    pthread_exit(NULL);
}

struct msgContents
{
    long mtype;
    int number;
} msg;

int calc_key(int id)
{
    return 22664 + id;
}

int init_msgq(int id)
{
    int key = calc_key(id);
    int msgcrt = msgget(key, IPC_CREAT | 0666);
    while (msgcrt == -1)
    {
        int msgcrt = msgget(key, IPC_CREAT | 0666);
    }
    PRINTF("Zainicjalizowano kolejkę %d, %d\n", id, msgcrt);
    return msgcrt;
}

void close_msgq(int msgqId)
{
    msgctl(msgqId, IPC_RMID, NULL);
    PRINTF("Usunięto kolejkę %d\n", msgqId);
}

int calc_node_messages_limit(int nid, int levels)
{
    int current_lvl = (nid < 4) ? 1 : 2;
    return (pow(3, levels - current_lvl) * 10);
}


void root(int levels)
{

    PRINTF("Korzen zyje\n");
    create_fifo();
    pthread_t reader_thread;
    pthread_mutex_init(&read_lock, NULL);

    pthread_create(&reader_thread, NULL, read_from_fifo, NULL);


    int messageQueIds[3];
    int totalMessages = 0;
    int messagesToReceive = (pow(3, levels) * 10);
    for (int i = 0; i < 3; i++)
    {
        messageQueIds[i] = init_msgq(i + 1);
    }
    while (totalMessages < messagesToReceive)
    {
        SLEEP(1);
        for (int i = 0; i < 3; i++)
        {
            msgrcv(messageQueIds[i], &msg, sizeof(msg), 1L, 0);
            totalMessages++;
            PRINTF("Korzen odebral wiadomosc: %d\n", msg.number); 
        }
    }
    for (int i = 0; i < 3; i++)
    {
        close_msgq(messageQueIds[i]);
    }
    SLEEP(1);
    pthread_mutex_lock(&read_lock);
        read_files = false;
    pthread_mutex_unlock(&read_lock);
    unlink(FIFO_NAME);
    PRINTF("Korzen konczy prace\n");
}

void node(int id, int levels)
{
    int childrenIds[3];
    int childrenMsgQueIds[3];
    int parentQueId = init_msgq(id);
    PRINTF("node %d idq: %d\n", id, parentQueId);
    int totalMessages = 0;
    int messagesToReceive = calc_node_messages_limit(id, levels);

    // Calculate the children IDs
    for (int i = 0; i < 3; i++)
    {
        int child_id = (id) * 3 + i + 1;
        childrenIds[i] = child_id;
        childrenMsgQueIds[i] = init_msgq(child_id);
    }
    while (totalMessages < messagesToReceive)
    {
        SLEEP(1);
        for (int i = 0; i < 3; i++)
        {
            msgrcv(childrenMsgQueIds[i], &msg, sizeof(msg), 1L, 0);
            totalMessages++;
            PRINTF("Node %d odbiera wiadomosc: %d\n", id, msg.number);
            msgsnd(parentQueId, &msg, sizeof(msg), 0);
        }
    }
    for (int i = 0; i < 3; i++)
    {
        close_msgq(childrenMsgQueIds[i]);
    }
    write_to_fifo(format_string("Node %d konczy prace, odebranych wiadomosci: %d\n", id, totalMessages));
}

void leaf(int id)
{
    int parentQueId = init_msgq(id);
    msg.mtype = 1;
    PRINTF("Lisc %d idq: %d\n", id, parentQueId);
    for (int i = 0; i < 10; i++)
    {
        SLEEP(1);
        msg.number = i;
        msgsnd(parentQueId, &msg, sizeof(msg), 0);
        PRINTF("Lisc %d wysyla: %d\n", id, i);
    }
    write_to_fifo(format_string("Lisc %d spadl\n", id));
}

int main(int argc, char *argv[])
{   
    int levels;
    if (argc != 3)
    {
        levels = 2;
    }
    else
    {
        levels = atoi(argv[2]);
    }
    int id = atoi(argv[1]);
    int leaf_after = levels == 2 ? 3 : 12;

    if (id == 0)
    {
        root(levels);
    }
    else if (id > leaf_after || levels == 1)
    {
        leaf(id);
    }
    else
    {
        node(id, levels);
    }
    return 0;
}