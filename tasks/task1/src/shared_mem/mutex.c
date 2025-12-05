#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NumThreads      4 

volatile int     var1;
volatile int     var2;
pthread_mutex_t  my_mutex = PTHREAD_MUTEX_INITIALIZER; // 1. Объявляем мьютекс

void    *update_thread (void *);
char    *progname = "mutex";

int main ()
{
    pthread_t           threadID [NumThreads];
    int                 i;

    setvbuf (stdout, NULL, _IOLBF, 0);
    var1 = var2 = 0; 
    printf ("%s:  starting; creating threads\n", progname);

    // Запускаем потоки с обычными настройками (NULL вместо attrib)
    for (i = 0; i < NumThreads; i++) {
        pthread_create (&threadID [i], NULL, &update_thread, (void *)(long)i);
    }

    sleep (20); // Ждем 20 секунд
    printf ("%s:  stopping; cancelling threads\n", progname);
    for (i = 0; i < NumThreads; i++) {
      pthread_cancel (threadID [i]);
    }
    printf ("%s:  all done, var1 is %d, var2 is %d\n", progname, var1, var2);
    fflush (stdout);
    sleep (1);
    exit (0);
}

void do_work()
{
    static int var3;
    var3++;
    if ( !(var3 % 10000000) ) 
        printf ("%s: thread %lu did some work\n", progname, (unsigned long)pthread_self());
}

void *update_thread (void *i)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1) {
        // 2. Блокируем доступ (закрываем дверь)
        pthread_mutex_lock(&my_mutex);

        // Критическая секция (никто другой сюда не залезет)
        if (var1 != var2) {
            printf ("%s:  thread %ld, var1 (%d) is not equal to var2 (%d)!\n",
                    progname, (long) i, var1, var2);
            var1 = var2;
        }
        do_work(); 
        var1++;
        var2++;

        // 3. Разблокируем доступ (открываем дверь)
        pthread_mutex_unlock(&my_mutex);
    }
    return (NULL);
}