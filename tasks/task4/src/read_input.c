#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s /dev/input/eventX\n", argv[0]);
        return 1;
    }

    const char *device_path = argv[1];

    // Открыть файл устройства для чтения
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    /* --- ЗАДАНИЕ 2: ИСПОЛЬЗОВАНИЕ IOCTL --- */
    char name[256] = "Unknown";
    char phys[256] = "Unknown";

    // 1. Получаем имя устройства (EVIOCGNAME)
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
        perror("Failed to get device name");
    } else {
        printf("Device Name: %s\n", name);
    }

    // 2. Получаем физический адрес (EVIOCGPHYS) - опционально
    if (ioctl(fd, EVIOCGPHYS(sizeof(phys)), phys) < 0) {
        // Не у всех устройств есть phys путь, это не ошибка
    } else {
        printf("Physical Location: %s\n", phys);
    }
    /* -------------------------------------- */

    printf("Reading events from %s. Press Ctrl+C to exit.\n", device_path);

    struct input_event ev;
    while (1) {
        ssize_t bytes = read(fd, &ev, sizeof(struct input_event));
        
        if (bytes < 0) {
            perror("Read error");
            break;
        }
        
        if (bytes != sizeof(struct input_event)) {
            fprintf(stderr, "Short read\n");
            break;
        }

        // Выводим все события (чтобы видеть нажатия нашей вирт. клавиатуры)
        printf("Event: type %d, code %d, value %d\n", ev.type, ev.code, ev.value);
    }

    close(fd);
    return 0;
}