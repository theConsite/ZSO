#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#ifdef debug
#define PRINTF(...) printf(__VA_ARGS__)
#define SLEEP(x) sleep(x)
// #define SLEEP(x)
#else
#define PRINTF(...)
#define SLEEP(x)
#endif

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
    int messageQueIds[3];
    int totalMessages = 0;
    int messagesToReceive = levels * 30;
    for (int i = 0; i < 3; i++)
    {
        messageQueIds[i] = init_msgq(i + 1);
    }
    while (totalMessages < messagesToReceive)
    {
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
    PRINTF("Korzen konczy prace\n");
}

void node(int id, int levels)
{
    int childrenIds[3];
    int childrenMsgQueIds[3];
    int parentQueId = init_msgq(id);
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
    PRINTF("Node %d konczy prace\n", id);
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
    PRINTF("Lisc %d spadl\n", id);
}

int main(int argc, char *argv[])
{
    int levels = 0;
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