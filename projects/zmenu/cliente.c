#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/zmetrics.sock"

/* === PROTOCOLO === */

typedef struct
{
    char magic[4];
    unsigned char version;
    unsigned char type;
    unsigned short size;
} ZHeader;

typedef struct
{
    float cpu_usage;
    float mem_used_gb;
    float mem_total_gb;
    int temp_c;
    char cpu_model[64];
    char mobo_name[64];
    char gpu_name[64];
} ZMetrics;
static int read_full(int sock, void *buf, size_t len)
{
    size_t off = 0;

    while (off < len)
    {
        ssize_t r = read(sock, (char *)buf + off, len - off);

        if (r == 0)
            return 0; // server closed

        if (r < 0)
            return -1;

        off += r;
    }

    return 1;
}
int main()
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        return 1;
    }
    while (1)
    {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("connect");
            close(sock);
            sleep(1);
            continue;
        }

        ZHeader h;
        if (read_full(sock, &h, sizeof(h)) <= 0)
        {
            close(sock);
            continue;
        }

        ZMetrics m;
        if (read_full(sock, &m, sizeof(m)) <= 0)
        {
            close(sock);
            continue;
        }

        printf("\033[H\033[J");
        printf("CPU: %.1f\n", m.cpu_usage);
        printf("RAM: %.2f / %.2f GB\n", m.mem_used_gb, m.mem_total_gb);
        printf("TEMP: %d°C\n", m.temp_c);
        fflush(stdout);
        close(sock);
        usleep(200000);
    }

    close(sock);
    return 0;
}