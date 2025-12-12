/*
 * Клиент POSIX Message Queues
 */
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

int main() {
    mqd_t mq_server, mq_client;
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    // Подключаемся к серверу
    mq_server = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if (mq_server == (mqd_t)-1) {
        fprintf(stderr, "Error: Cannot connect to server queue. Is server running?\n");
        exit(1);
    }

    // Создаем свою очередь для ответов
    // Важно: unlink нужен, чтобы очистить очередь от старых сообщений
    mq_unlink(CLIENT_QUEUE_NAME); 
    mq_client = mq_open(CLIENT_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if (mq_client == (mqd_t)-1) {
        perror("mq_open (client)");
        exit(1);
    }

    char *messages[] = {"hello task 1", "URGENT ALERT!", "hello task 2"};
    unsigned int priorities[] = {MSG_PRIO_NORMAL, MSG_PRIO_HIGH, MSG_PRIO_NORMAL};

    printf("Client started. Sending %ld messages...\n", sizeof(messages)/sizeof(char*));

    for (int i = 0; i < 3; ++i) {
        printf("[Client] Sending (prio %u): '%s'\n", priorities[i], messages[i]);
        
        if (mq_send(mq_server, messages[i], strlen(messages[i]) + 1, priorities[i]) == -1) {
            perror("mq_send");
            continue;
        }

        // Ждем ответа
        char buffer[MAX_MSG_SIZE];
        ssize_t bytes = mq_receive(mq_client, buffer, MAX_MSG_SIZE, NULL);
        if (bytes >= 0) {
            printf("[Client] Reply received: '%s'\n\n", buffer);
        } else {
            perror("mq_receive");
        }
        
        sleep(1); 
    }

    mq_close(mq_server);
    mq_close(mq_client);
    mq_unlink(CLIENT_QUEUE_NAME); // Удаляем свою очередь

    return 0;
}