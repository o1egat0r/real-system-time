/*
 *  Менеджер ресурсов (Server)
 *  Путь сокета: /tmp/example_resmgr.sock
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define EXAMPLE_SOCK_PATH "/tmp/example_resmgr.sock" // ПУТЬ КАК У КЛИЕНТА

//ОБЩИЙ РЕСУРС
char shared_buffer[256] = "Empty"; 
pthread_mutex_t res_mutex = PTHREAD_MUTEX_INITIALIZER;


static const char *progname = "resmgr";
static int listen_fd = -1;

static void install_signals(void);
static void on_signal(int signo);
static void *client_thread(void *arg);

int main(int argc, char *argv[])
{
  setvbuf(stdout, NULL, _IOLBF, 0);
  install_signals();

  listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd == -1) { perror("socket"); return EXIT_FAILURE; }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, EXAMPLE_SOCK_PATH, sizeof(addr.sun_path) - 1);

  unlink(EXAMPLE_SOCK_PATH); // Удаляем старый файл

  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return EXIT_FAILURE;
  }

  if (listen(listen_fd, 8) == -1) { perror("listen"); return EXIT_FAILURE; }

  printf("%s: Server started on %s\n", progname, EXAMPLE_SOCK_PATH);

  while (1) {
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd == -1) {
      if (errno == EINTR) continue;
      perror("accept");
      break;
    }
    pthread_t th;
    pthread_create(&th, NULL, client_thread, (void *)(long)client_fd);
    pthread_detach(th);
  }

  close(listen_fd);
  unlink(EXAMPLE_SOCK_PATH);
  return EXIT_SUCCESS;
}

static void *client_thread(void *arg)
{
  int fd = (int)(long)arg;
  char buf[512];
  char response[512];

  memset(buf, 0, sizeof(buf));
  ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
  
  if (n > 0) {
      // Логика обработки команд
      if (strncmp(buf, "READ", 4) == 0) {
          pthread_mutex_lock(&res_mutex);
          snprintf(response, sizeof(response), "DATA: %s", shared_buffer);
          pthread_mutex_unlock(&res_mutex);
      } 
      else if (strncmp(buf, "WRITE ", 6) == 0) {
          pthread_mutex_lock(&res_mutex);
          strncpy(shared_buffer, buf + 6, sizeof(shared_buffer) - 1);
          snprintf(response, sizeof(response), "OK: Updated");
          pthread_mutex_unlock(&res_mutex);
      } 
      else {
          snprintf(response, sizeof(response), "ERROR: Unknown command");
      }
      send(fd, response, strlen(response), 0);
  }
  close(fd);
  return NULL;
}

static void install_signals(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = on_signal;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
}

static void on_signal(int signo) {
  (void)signo;
  if (listen_fd != -1) close(listen_fd);
  unlink(EXAMPLE_SOCK_PATH);
  exit(0);
}