#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>

/* Мьютекс для защиты ресурса */
pthread_mutex_t mutex;

/* Функция для имитации нагрузки (жжет процессор) */
void burn_cpu(int iterations) {
    long long i;
    for(i=0; i<iterations * 1000000LL; i++) {
        asm volatile("nop");
    }
}

/* Поток с НИЗКИМ приоритетом (Low) */
/* Захватывает ресурс первым, но работает медленно */
void* thread_low(void* arg) {
    printf("[LOW]  Старт (Low Priority)\n");
    
    pthread_mutex_lock(&mutex);
    printf("[LOW]  Захватил мьютекс. Начинаю долгую работу...\n");
    
    // Работаем в критической секции
    for(int i=0; i<3; i++) {
        printf("[LOW]  Работаю %d/3...\n", i+1);
        burn_cpu(50); // Имитация работы
    }
    
    printf("[LOW]  Разблокирую мьютекс\n");
    pthread_mutex_unlock(&mutex);
    
    printf("[LOW]  Завершил работу\n");
    return NULL;
}

/* Поток с ВЫСОКИМ приоритетом (High) */
/* Ему нужен тот же ресурс, что и Low */
void* thread_high(void* arg) {
    usleep(100000); // Ждем чуть-чуть, чтобы Low успел захватить мьютекс
    printf("[HIGH] Старт (High Priority). Пытаюсь захватить мьютекс...\n");
    
    pthread_mutex_lock(&mutex); // Здесь он должен повиснуть и "пнуть" Low (поднять ему приоритет)
    printf("[HIGH] Получил мьютекс! Быстрая важная работа!\n");
    burn_cpu(20);
    pthread_mutex_unlock(&mutex);
    
    printf("[HIGH] Завершил работу\n");
    return NULL;
}

/* Поток со СРЕДНИМ приоритетом (Mid) */
/* Просто грузит процессор и мешает Low, если нет защиты */
void* thread_mid(void* arg) {
    usleep(200000); // Стартует, когда Low уже работает, а High уже ждет
    printf("[MID]  Старт (Medium Priority). Пытаюсь загрузить CPU...\n");
    
    // Пытаемся перебить Low. При PTHREAD_PRIO_INHERIT это НЕ выйдет, 
    // так как Low временно стал High.
    burn_cpu(100); 
    
    printf("[MID]  Завершил работу\n");
    return NULL;
}

int main() {
    pthread_t t_low, t_mid, t_high;
    pthread_attr_t attr;
    struct sched_param param;
    pthread_mutexattr_t m_attr;
    
    // 1. Инициализируем атрибуты мьютекса
    pthread_mutexattr_init(&m_attr);
    // 2. Включаем протокол наследования приоритетов (Priority Inheritance)
    pthread_mutexattr_setprotocol(&m_attr, PTHREAD_PRIO_INHERIT);
    // 3. Создаем мьютекс с этими настройками
    pthread_mutex_init(&mutex, &m_attr);

    // Настройка потоков для планировщика реального времени (FIFO)
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    // 1. Запускаем LOW (Приоритет 10 - низкий)
    printf("Запуск scenario_2 с защитой от инверсии\n");
    param.sched_priority = 10;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t_low, &attr, thread_low, NULL);

    // 2. Запускаем HIGH (Приоритет 50 - высокий)
    param.sched_priority = 50;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t_high, &attr, thread_high, NULL);

    // 3. Запускаем MID (Приоритет 30 - средний)
    param.sched_priority = 30;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t_mid, &attr, thread_mid, NULL);

    // Ждем завершения
    pthread_join(t_low, NULL);
    pthread_join(t_high, NULL);
    pthread_join(t_mid, NULL);

    printf("завершено\n");
    return 0;
}