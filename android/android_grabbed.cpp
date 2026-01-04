#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <csignal>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <poll.h>
#include <algorithm>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#ifndef BITS_PER_LONG
#define BITS_PER_LONG (sizeof(long) * 8)
#endif
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define test_bit(bit, array) ((array[(bit)/BITS_PER_LONG] >> OFF(bit)) & 1)

#define CONTROL_PORT 22222
#define PROTOCOL_HEAD 0x55

struct ControlPacket {
    uint8_t head; 
    uint32_t magic;
    uint8_t type;
    uint16_t x;
    uint16_t y;
    uint16_t data;
    uint8_t crc;
} __attribute__((packed));

std::vector<int> all_fds;
int touch_fd = -1;

volatile sig_atomic_t g_running = 1;
void signal_handler(int) { g_running = 0; }

void log(const std::string& msg) {
    std::cerr << "[Hardware Grabbed] " << msg << std::endl;}

void inject_event(int fd, int type, int code, int value) {
    if (fd < 0) return;
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

void grab_all_inputs() {
    DIR *dir = opendir("/dev/input");
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            std::string path = "/dev/input/" + std::string(entry->d_name);
            int fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
            if (fd >= 0) {
                unsigned long features[NBITS(EV_MAX)];
                if (ioctl(fd, EVIOCGBIT(0, sizeof(features)), features) >= 0) {
                    if (test_bit(EV_KEY, features) || test_bit(EV_ABS, features)) {
                        if (ioctl(fd, EVIOCGRAB, 1) == 0) {
                            log("ZABLOKOWANO HARDWARE: " + path);
                            all_fds.push_back(fd);
                            unsigned long abs_bits[NBITS(ABS_MAX)];
                            if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) >= 0) {
                                if (test_bit(ABS_MT_POSITION_X, abs_bits)) {
                                    touch_fd = fd;
                                    log("Główny ekran przypisany do: " + path);}}
                        } else {
                            close(fd);}
                    } else {
                        close(fd);}
                } else {
                    close(fd);}}}}
    closedir(dir);}

void release_all_inputs() {
    for (int fd : all_fds) {
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);}
    all_fds.clear();
    log("Przyciski fizyczne przywrócone.");}

void process_packet(const ControlPacket& pkt) {
    if (touch_fd < 0 && all_fds.empty()) return;
    uint16_t x = ntohs(pkt.x);
    uint16_t y = ntohs(pkt.y);
    uint16_t data = ntohs(pkt.data);
    switch (pkt.type) {
        case 2: // TOUCH_DOWN
            inject_event(touch_fd, EV_ABS, ABS_MT_SLOT, 0);
            inject_event(touch_fd, EV_ABS, ABS_MT_TRACKING_ID, 1);
            inject_event(touch_fd, EV_ABS, ABS_MT_POSITION_X, x);
            inject_event(touch_fd, EV_ABS, ABS_MT_POSITION_Y, y);
            inject_event(touch_fd, EV_KEY, BTN_TOUCH, 1);
            inject_event(touch_fd, EV_SYN, SYN_REPORT, 0);
            break;
        case 3: // TOUCH_UP
            inject_event(touch_fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
            inject_event(touch_fd, EV_KEY, BTN_TOUCH, 0);
            inject_event(touch_fd, EV_SYN, SYN_REPORT, 0);
            break;
        case 4: // TOUCH_MOVE
            inject_event(touch_fd, EV_ABS, ABS_MT_POSITION_X, x);
            inject_event(touch_fd, EV_ABS, ABS_MT_POSITION_Y, y);
            inject_event(touch_fd, EV_SYN, SYN_REPORT, 0);
            break;
        case 1: // KEY
            for(int fd : all_fds) {
                inject_event(fd, EV_KEY, data, 1);
                inject_event(fd, EV_SYN, SYN_REPORT, 0);
                inject_event(fd, EV_KEY, data, 0);
                inject_event(fd, EV_SYN, SYN_REPORT, 0);}
            break;}}

int main(int argc, char* argv[]) {
    // flaga -d
    bool run_as_daemon = false;
    int port = CONTROL_PORT;
    int opt;
    while ((opt = getopt(argc, argv, "dc:")) != -1) {
        switch (opt) {
            case 'd': run_as_daemon = true; break;
            case 'c': port = std::stoi(optarg); break;}}
    if (run_as_daemon) {
        log("Uruchamianie w tle (daemon)...");
        daemon(0, 0);}
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    grab_all_inputs();
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        release_all_inputs();
        return 1;}
    listen(server_fd, 1);
    log("grabbed gotowy. Port: " + std::to_string(port));

    while (g_running) {
        struct pollfd pfd = { .fd = server_fd, .events = POLLIN };
        int res = poll(&pfd, 1, 500);
        if (res <= 0) continue;
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) continue;
        log("GUI połączone.");
        ControlPacket pkt;
        while (g_running) {
            ssize_t n = read(client_fd, &pkt, sizeof(pkt));
            if (n <= 0) break;
            if (pkt.head == PROTOCOL_HEAD) {
                process_packet(pkt);}}
        close(client_fd);
        log("GUI rozłączone. Oczekiwanie na nowe połączenie...");}
    release_all_inputs();
    close(server_fd);
    return 0;}
