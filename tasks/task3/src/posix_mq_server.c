/*
 * Сервер POSIX Message Queues
 */
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

void to_upper(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

int main() {
    mqd_t mq_server, mq_client;
    struct mq_attr attr;

    // Настройки очереди
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    // Очистка старых очередей, если программа упала в прошлый раз
    mq_unlink(SERVER_QUEUE_NAME);
    mq_unlink(CLIENT_QUEUE_NAME);

    // Создаем очередь сервера
    mq_server = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if (mq_server == (mqd_t)-1) {
        perror("mq_open (server)");
        exit(1);
    }

    printf("Server started. Waiting for messages...\n");

    while (1) {
        char buffer[MAX_MSG_SIZE];
        unsigned int priority;

        // Блокирующее чтение сообщения
        ssize_t bytes_read = mq_receive(mq_server, buffer, MAX_MSG_SIZE, &priority);
        if (bytes_read >= 0) {
            buffer[bytes_read] = '\0';
            printf("Received (prio %u): '%s'\n", priority, buffer);

            // Обработка данных
            to_upper(buffer);

            // Открытие очереди клиента для ответа
            mq_client = mq_open(CLIENT_QUEUE_NAME, O_WRONLY);
            if (mq_client == (mqd_t)-1) {
                // Клиент мог еще не создать очередь или отключиться
                perror("mq_open (client) failed - maybe client not ready");
                continue; 
            }

            if (mq_send(mq_client, buffer, strlen(buffer) + 1, 0) == -1) {
                perror("mq_send");
            } else {
                printf("Sent reply: '%s'\n", buffer);
            }
            mq_close(mq_client);

        } else {
            perror("mq_receive");
            break;
        }
    }

    /*
     * --- АНАЛИЗ POSIX MQ (для отчета) ---
     * 
     * Плюсы:
     * 1. Приоритеты: Встроенная поддержка приоритетов (как мы видели, urgent сообщения
     *    встают в начало очереди).
     * 2. Ориентированность на сообщения: Не нужно парсить поток байт (как в TCP/Pipe),
     *    границы сообщений сохраняются.
     * 3. Надежность: Очередь живет в ядре. Если процесс упал, сообщения остаются.
     * 
     * Минусы:
     * 1. Ограничения: Лимиты на размер сообщения и длину очереди (mq_maxmsg).
     *    Не подходит для передачи больших файлов.
     * 2. Копирование: Данные копируются из user-space в kernel и обратно (overhead).
     */

    mq_close(mq_server);
    mq_unlink(SERVER_QUEUE_NAME);

    return 0;
}