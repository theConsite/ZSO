#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
 #include <errno.h>

 #define test_errno(msg) do{if (errno) {perror(msg); exit(EXIT_FAILURE);}} while(0)

 /* prosta funkcja wykonywana w wątku */
 void* watek(void* _arg) {
      puts("Witaj w równoległym świecie!");
   return NULL;
 }
 
 #define N 500   /* liczba wątków */

 int main() {
     pthread_t id[N];
     int i;

     /* utworzenie kilku wątków wątku */
     for (i=0;i < N; i++) {
 	   errno = pthread_create(&id[i], NULL, watek, NULL);
      test_errno("Nie powiodło się pthread_create");
     }
 
     /* oczekiwanie na zakończenie wszystkich wątków */
     for (i=0;i < N; i++) {
         errno = pthread_join(id[i], NULL);
         test_errno("pthread_join");
    }

    return EXIT_SUCCESS;
}