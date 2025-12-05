/*
 *  Решение задачи с 4 состояниями (State Machine)
 *  State 0 -> State 1
 *  State 1 -> State 2 (если счетчик четный)
 *  State 1 -> State 3 (если счетчик нечетный)
 *  State 2 -> State 0
 *  State 3 -> State 0
 */ 

 #include <stdio.h>
 #include <unistd.h>
 #include <pthread.h>
 #include <sched.h>
 
 volatile int        state;          // Текущее состояние
 pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;
 pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;
 
 // Объявляем функции для 4 потоков
 void    *state_0 (void *);
 void    *state_1 (void *);
 void    *state_2 (void *);
 void    *state_3 (void *);
 
 char    *progname = "condvar";
 
 int main ()
 {
     setvbuf (stdout, NULL, _IOLBF, 0);
     state = 0;
     pthread_t t0, t1, t2, t3;
     
     // Создаем 4 потока для каждого состояния
     pthread_create (&t0, NULL, state_0, NULL);
     pthread_create (&t1, NULL, state_1, NULL);
     pthread_create (&t2, NULL, state_2, NULL);
     pthread_create (&t3, NULL, state_3, NULL);
     
     sleep (20); // Работаем 20 секунд
     printf ("%s:  main, exiting\n", progname);
     return 0;
 }
 
 /* Состояние 0: Переключает всегда на 1 */
 void *state_0 (void *arg)
 {
     while (1) {
         pthread_mutex_lock (&mutex);
         while (state != 0) {
             pthread_cond_wait (&cond, &mutex);
         }
         
         printf ("%s:  transit 0 -> 1\n", progname);
         state = 1;
         
         usleep(200 * 1000); // Небольшая задержка для читаемости
         
         pthread_cond_broadcast (&cond); // Будим остальные потоки
         pthread_mutex_unlock (&mutex);
     }
     return (NULL);
 }
 
 /* Состояние 1: Распределяет на 2 или 3 */
 void *state_1 (void *arg)
 {
     static int counter = 0; // Внутренний счетчик
     while (1) {
         pthread_mutex_lock (&mutex);
         while (state != 1) {
             pthread_cond_wait (&cond, &mutex);
         }
         
         if (counter % 2 == 0) {
             printf ("%s:  transit 1 -> 2 (even)\n", progname);
             state = 2;
         } else {
             printf ("%s:  transit 1 -> 3 (odd)\n", progname);
             state = 3;
         }
         counter++;
         
         pthread_cond_broadcast (&cond);
         pthread_mutex_unlock (&mutex);
     }
     return (NULL);
 }
 
 /* Состояние 2: Возвращает в 0 */
 void *state_2 (void *arg)
 {
     while (1) {
         pthread_mutex_lock (&mutex);
         while (state != 2) {
             pthread_cond_wait (&cond, &mutex);
         }
         
         printf ("%s:  transit 2 -> 0\n", progname);
         state = 0;
         
         pthread_cond_broadcast (&cond);
         pthread_mutex_unlock (&mutex);
     }
     return (NULL);
 }
 
 /* Состояние 3: Возвращает в 0 */
 void *state_3 (void *arg)
 {
     while (1) {
         pthread_mutex_lock (&mutex);
         while (state != 3) {
             pthread_cond_wait (&cond, &mutex);
         }
         
         printf ("%s:  transit 3 -> 0\n", progname);
         state = 0;
         
         pthread_cond_broadcast (&cond);
         pthread_mutex_unlock (&mutex);
     }
     return (NULL);
 }