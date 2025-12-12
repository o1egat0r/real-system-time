#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/uinput.h>

void emit(int fd, int type, int code, int val) {
    struct input_event ie;
    ie.type = type;
    ie.code = code;
    ie.value = val;
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;
    write(fd, &ie, sizeof(ie));
}

int main() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open /dev/uinput");
        return 1;
    }

    // Включаем поддержку клавиш
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, KEY_A); // Будем нажимать 'A'
    ioctl(fd, UI_SET_KEYBIT, KEY_B); // И 'B'

    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Virtual Lab Keyboard");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;

    write(fd, &uidev, sizeof(uidev));
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        perror("ioctl create");
        return 1;
    }

    printf("Virtual device created. Press Ctrl+C to stop.\n");
    printf("Emulating key presses (A)... check /dev/input/\n");

    while(1) {
        // Нажать 'A'
        emit(fd, EV_KEY, KEY_A, 1);
        emit(fd, EV_SYN, SYN_REPORT, 0);
        usleep(100000); // Держать 0.1 сек

        // Отпустить 'A'
        emit(fd, EV_KEY, KEY_A, 0);
        emit(fd, EV_SYN, SYN_REPORT, 0);
        
        sleep(2); // Ждать 2 секунды
    }

    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}