/*
 *  Демонстрация POSIX условных переменных на примере "Производитель и потребитель".
 */

#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>

// mutex и условная переменная

pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;
volatile int        state = 0;    // переменная состояния
volatile int        product = 0;  // вывод производителя
void    *producer (void *);
void    *consumer (void *);
void    do_producer_work (void);
void    do_consumer_work (void);
char    *progname = "prodcons";

int main ()
{
  pthread_t t_prod, t_cons; // 1. Объявляем переменные для ID потоков

  setvbuf (stdout, NULL, _IOLBF, 0);
  
  // 2. Передаем адреса этих переменных (&t_cons, &t_prod)
  pthread_create (&t_cons, NULL, consumer, NULL);
  pthread_create (&t_prod, NULL, producer, NULL);
  
  sleep (20);     // Позволим потокам выполнить "работу"
  printf ("%s:  main, exiting\n", progname);
  exit(0);
}

// Производитель
void *producer (void *arg)
{
  while (1) {
    pthread_mutex_lock (&mutex);
    while (state == 1) {
      pthread_cond_wait (&cond, &mutex);
    }
    printf ("%s:  produced %d, state %d\n", progname, ++product, state);
    state = 1;
    pthread_cond_signal (&cond);
    pthread_mutex_unlock (&mutex);
    do_producer_work ();
  }
  return (NULL);
}

// Потребитель

void *consumer (void *arg)
{
  while (1) {
    pthread_mutex_lock (&mutex);
    while (state == 0) {
      pthread_cond_wait (&cond, &mutex);
    }
    printf ("%s:  consumed %d, state %d\n", progname, product, state);
    state = 0;
    pthread_cond_signal (&cond);
    pthread_mutex_unlock (&mutex);
    do_consumer_work ();
  }
  return (NULL);
}

void do_producer_work (void)
{
  usleep (100 * 1000);
}

void do_consumer_work (void)
{
  usleep (100 * 1000);
}