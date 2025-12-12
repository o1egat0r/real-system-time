/*
 *  POSIX clock demo with 2 ms period sampling for Linux.
 *
 *  Цели:
 *  - Показать использование CLOCK_MONOTONIC и clock_getres
 *  - Реализовать периодическую выборку с шагом 2 мс через
 *    абсолютный clock_nanosleep(TIMER_ABSTIME)
 *  - Измерить фактические дельты между сэмплами и вывести статистику
 */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BILLION 1000000000LL
#define MILLION 1000000LL
#define NUM_SAMPLES 5000 /* 5000 * 2 ms ≈ 10 секунд эксперимента */

// Вспомогательная функция: перевод timespec в наносекунды
static inline int64_t timespec_to_ns(const struct timespec *ts) {
    return (int64_t)ts->tv_sec * BILLION + (int64_t)ts->tv_nsec;
}

// Вспомогательная функция: перевод наносекунд в timespec
static inline void ns_to_timespec(int64_t ns, struct timespec *ts) {
    ts->tv_sec = (time_t)(ns / BILLION);
    ts->tv_nsec = (long)(ns % BILLION);
}

#ifdef __linux__
int main(void) {
    struct timespec res_rt = {0}, res_mono = {0};
    struct timespec t_next = {0}, now = {0};
    const int64_t period_ns = 2 * MILLION; /* Период 2 мс */
    int64_t deltas_ns[NUM_SAMPLES];
    int samples = 0;

    setvbuf(stdout, NULL, _IOLBF, 0);

    // Получаем разрешение часов (для справки)
    if (clock_getres(CLOCK_REALTIME, &res_rt) != 0) {
        fprintf(stderr, "clock_getres(CLOCK_REALTIME) failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    if (clock_getres(CLOCK_MONOTONIC, &res_mono) != 0) {
        fprintf(stderr, "clock_getres(CLOCK_MONOTONIC) failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("Resolution: REALTIME=%ld ns, MONOTONIC=%ld ns\n",
           (long)res_rt.tv_nsec, (long)res_mono.tv_nsec);

    // Инициализируем время старта
    if (clock_gettime(CLOCK_MONOTONIC, &t_next) != 0) {
        fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // [MODIFIED] Сохраняем время "предыдущего" пробуждения для расчета интервала
    int64_t start_ns = timespec_to_ns(&t_next);
    int64_t prev_wakeup_ns = start_ns;
    int64_t next_target_ns = start_ns;

    /* 
     * [COMMENT FOR LAB]
     * Почему TIMER_ABSTIME важен?
     * Если использовать относительный сон (rel_time), то:
     *   Время_цикла = Время_сна + Время_работы + Джиттер
     * Ошибка будет накапливаться с каждой итерацией (Drift). 
     * Через 1000 циклов мы отстанем существенно.
     * 
     * С TIMER_ABSTIME мы говорим: "Разбуди меня ровно в 12:00:01, потом в 12:00:02".
     * Если мы проснулись чуть позже или работали долго, следующий сон просто будет короче.
     * Ошибка НЕ накапливается.
     */

    for (samples = 0; samples < NUM_SAMPLES; ++samples) {
        // [MODIFIED] Рассчитываем следующее АБСОЛЮТНОЕ время пробуждения
        next_target_ns += period_ns; 
        ns_to_timespec(next_target_ns, &t_next);

        /* Абсолютный сон до t_next: устойчив к дрейфу */
        int rc;
        do {
            rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t_next, NULL);
        } while (rc == EINTR);
        
        if (rc != 0) {
            fprintf(stderr, "clock_nanosleep failed: %s\n", strerror(rc));
            return EXIT_FAILURE;
        }

        // Замеряем фактическое время пробуждения
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }

        int64_t now_ns = timespec_to_ns(&now);
        
        // [MODIFIED] Считаем интервал между текущим и прошлым пробуждением
        // Идеал: должно быть ровно 2 000 000 нс.
        deltas_ns[samples] = now_ns - prev_wakeup_ns; 
        
        prev_wakeup_ns = now_ns;
    }

    /* Статистика */
    int64_t min_ns = INT64_MAX, max_ns = INT64_MIN, sum_ns = 0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        if (deltas_ns[i] < min_ns) min_ns = deltas_ns[i];
        if (deltas_ns[i] > max_ns) max_ns = deltas_ns[i];
        sum_ns += deltas_ns[i];
    }
    double avg_ns = (double)sum_ns / (double)NUM_SAMPLES;

    // [MODIFIED] Расчет стандартного отклонения (Standard Deviation)
    // Показывает стабильность таймера (насколько велик разброс)
    double sum_sq_diff = 0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        sum_sq_diff += pow((double)deltas_ns[i] - avg_ns, 2);
    }
    double std_dev_ns = sqrt(sum_sq_diff / NUM_SAMPLES);

    printf("Period stats over %d samples (target: %" PRId64 " ns):\n", NUM_SAMPLES, period_ns);
    printf("  min=%" PRId64 " ns, avg=%.1f ns, max=%" PRId64 " ns, std_dev=%.1f ns\n",
           min_ns, avg_ns, max_ns, std_dev_ns);

    /* Вывести первые несколько измерений для наглядности */
    printf("\nFirst 10 intervals (ns):\n");
    for (int i = 0; i < 10 && i < NUM_SAMPLES; ++i) {
        printf("  sample %d: %" PRId64 "\n", i, deltas_ns[i]);
    }

    return EXIT_SUCCESS;
}
#else
// Fallback для не-Linux систем (оставляем как было в примере)
int main(void) {
    fprintf(stderr, "This example requires Linux with CLOCK_MONOTONIC support.\n");
    return EXIT_FAILURE;
}
#endif