
#include <stdio.h>
#include <unistd.h>

int main() {
    fork();
    int test = fork();
    printf("%d\n",test);

    return 0;
}