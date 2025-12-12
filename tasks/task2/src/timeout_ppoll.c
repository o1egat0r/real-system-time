/*
 * Демонстрация ppoll() для ожидания с атомарной разблокировкой сигналов.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Флаг, который будет (безопасно) установлен в обработчике сигнала.
static volatile sig_atomic_t signal_received = 0;

static void signal_handler(int signum) {
    (void)signum;
    signal_received = 1;
    write(STDOUT_FILENO, "Signal handler executed!\n", 25);
}

// Поток, посылающий сигнал
static void *thread_sender(void *arg) {
    // Получаем ID главного потока из аргумента
    pthread_t main_thread_id = *(pthread_t*)arg;
    
    printf("[SENDER] sleeping for 1 second...\n");
    sleep(1);
    printf("[SENDER] sending SIGUSR1 to main thread...\n");
    
    // Посылаем сигнал главному потоку
    pthread_kill(main_thread_id, SIGUSR1);
    return NULL;
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    // 1. Настройка обработчика сигнала
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    // 2. Блокируем SIGUSR1.
    sigset_t blocked_mask, original_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &blocked_mask, &original_mask) != 0) {
        perror("pthread_sigmask");
        return EXIT_FAILURE;
    }
    printf("Main thread blocked SIGUSR1.\n");

    // 3. Создаем pipe
    int fds[2];
    if (pipe(fds) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }
    struct pollfd pfd = {.fd = fds[0], .events = POLLIN};

    // 4. Создаем поток, передавая ему ID текущего (главного) потока
    pthread_t main_tid = pthread_self(); // Узнаем свой ID
    pthread_t tid;
    // Передаем адрес переменной main_tid
    if (pthread_create(&tid, NULL, thread_sender, &main_tid) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    // 5. Вызываем ppoll().
    printf("Calling ppoll() with unblocked signal mask, waiting for signal...\n");
    struct timespec timeout = {.tv_sec = 5, .tv_nsec = 0};

    int rc = ppoll(&pfd, 1, &timeout, &original_mask);

    // 6. Анализ результата
    if (rc == -1) {
        if (errno == EINTR) {
            printf("ppoll was correctly interrupted by a signal.\n");
        } else {
            perror("ppoll failed");
        }
    } else if (rc == 0) {
        printf("ppoll timed out, which is unexpected.\n");
    } else {
        printf("ppoll detected data, which is unexpected.\n");
    }

    if (signal_received) {
        printf("Verified that the signal handler was executed.\n");
    } else {
        printf("Warning: signal handler was not executed.\n");
    }

    pthread_join(tid, NULL);
    close(fds[0]);
    close(fds[1]);

    /* 
     * [ADDED] --- Сравнение стратегий ожидания (для отчета) ---
     * 
     * 1. poll (timeout_poll.c):
     *    - Ждет на файловых дескрипторах.
     *    - Минус: Race Condition с сигналами.
     * 
     * 2. ppoll (этот файл):
     *    - Решение: Атомарно меняет маску сигналов и ждет.
     *    - Применение: Главный цикл (Main Loop) с обработкой сигналов.
     * 
     * 3. condvar (timeout_condvar.c):
     *    - Ждет изменения переменной в памяти (межпоточное).
     * 
     * 4. mq (timeout_mq.c):
     *    - Ждет сообщения в очереди (межпроцессное).
     */

    return 0;
}