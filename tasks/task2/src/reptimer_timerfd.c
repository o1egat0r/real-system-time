/*
 * Демонстрация периодического таймера с использованием timerfd.
 *
 * Цель: Показать способ создания периодических задач через файловый дескриптор.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/timerfd.h>
#endif

#ifdef __linux__
int main(void) {
    int tfd;
    struct itimerspec its;
    struct timespec now;
    uint64_t expirations;
    const int iterations_to_run = 5;

    setvbuf(stdout, NULL, _IOLBF, 0);

    // 1. Создаем таймер (CLOCK_MONOTONIC, чтобы не зависеть от перевода часов)
    tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (tfd == -1) {
        perror("timerfd_create failed");
        return EXIT_FAILURE;
    }

    // 2. Настройка времени: первое срабатывание через 5 секунд
    its.it_value.tv_sec = 5;
    its.it_value.tv_nsec = 0;

    // 3. Настройка периода: каждые 1.5 секунды (1 сек + 500 млн наносек)
    its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = 500000000; 

    // Запускаем таймер
    if (timerfd_settime(tfd, 0, &its, NULL) == -1) {
        perror("timerfd_settime failed");
        close(tfd);
        return EXIT_FAILURE;
    }

    printf("Timer started. Initial wait: 5s, then period: 1.5s.\n");
    printf("Waiting for %d expirations...\n", iterations_to_run);

    for (int i = 0; i < iterations_to_run; ++i) {
        // 4. Ожидание события. read() заблокируется, пока таймер не "тикнет".
        // Он вернет количество срабатываний (обычно 1, если мы успеваем обрабатывать).
        ssize_t rd = read(tfd, &expirations, sizeof(expirations));
        if (rd < 0) {
            if (errno == EINTR) { 
                --i;
                continue;
            }
            perror("read(timerfd) failed");
            close(tfd);
            return EXIT_FAILURE;
        }

        // Получаем текущее время для лога
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            perror("clock_gettime failed");
            close(tfd);
            return EXIT_FAILURE;
        }
        printf("Timer expired! Timestamp: [%ld.%09ld], expirations: %" PRIu64 "\n",
               now.tv_sec, now.tv_nsec, expirations);
    }

    close(tfd);
    
    /* 
     * --- ОТВЕТ НА ВОПРОС ЗАДАНИЯ (Comparison) ---
     * 
     * clock_nanosleep vs timerfd:
     * 
     * 1. clock_nanosleep:
     *    - Блокирует весь поток. Поток не может делать ничего другого, пока спит.
     *    - Просто использовать для простых циклов "while(1) { work(); sleep(); }".
     * 
     * 2. timerfd:
     *    - Превращает время в ФАЙЛОВЫЙ ДЕСКРИПТОР.
     *    - ГЛАВНОЕ ПРЕИМУЩЕСТВО: Интеграция с epoll/poll/select.
     *    - Позволяет реализовать Event Loop: один поток может ждать и таймера,
     *      и прихода данных по сети, и нажатия кнопки одновременно.
     *      Пример: epoll_wait() разбудит нас, если сработает таймер ИЛИ придут данные.
     *      С clock_nanosleep так сделать нельзя (надо создавать много потоков).
     */
     
    return EXIT_SUCCESS;
}
#else
int main(void) {
    printf("Linux only.\n");
    return 0;
}
#endif