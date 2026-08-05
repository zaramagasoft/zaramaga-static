#include "metricas.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <stdlib.h>
#define SOCKET_PATH "/tmp/zmetrics.sock"


typedef struct
{
    char magic[4];   // "ZMET"
    uint8_t version; // 1
    uint8_t type;    // 1 = metrics
    uint16_t size;   // payload size
} ZHeader;

int zsock_init()
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }

    // 🔥 limpiar socket viejo ANTES del bind
    unlink(SOCKET_PATH);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    // 🔥 SOLO filesystem socket (NO abstract mix)
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    int len = sizeof(struct sockaddr_un);

    printf("[DEBUG] socket=%d\n", sock);
    printf("[DEBUG] len=%d\n", len);
    printf("[DEBUG] path=%s\n", SOCKET_PATH);

    if (bind(sock, (struct sockaddr *)&addr, len) < 0)
    {
        perror("bind FAIL");
        close(sock);
        return -1;
    }

    if (listen(sock, 5) < 0)
    {
        perror("listen");
        close(sock);
        return -1;
    }

    // 🔥 non-blocking accept
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    printf("socket bind OK\n");

    return sock;
}

/* void zsock_send_metrics(int sock, ZMetrics *m)
{

    int client = accept(sock, NULL, NULL);
    if (client < 0)
        return;

    // ZHeader h = {...};
    ZHeader h = {
        .magic = {'Z', 'M', 'E', 'T'},
        .version = 1,
        .type = 1,
        .size = sizeof(ZMetrics)};

    write(client, &h, sizeof(h));
    write(client, m, sizeof(ZMetrics));

    close(client); // ✔ correcto en este modelo
} */
void zsock_send_metrics(int client_fd, ZMetrics *m)
{
    ZHeader h = {
        .magic = {'Z', 'M', 'E', 'T'},
        .version = 1,
        .type = 1,
        .size = sizeof(ZMetrics)};

    write(client_fd, &h, sizeof(h));
    write(client_fd, m, sizeof(ZMetrics));
}

int main()
{
    signal(SIGPIPE, SIG_IGN); // 🔥 clave
    ZMetrics m;

    metrics_init(&m);

    printf("\033[2J\033[H");
    printf("ZaramagaOS Hardware Info\n");
    printf("------------------------\n");
    printf("CPU:   %s\n", m.cpu_model);
    printf("PLACA: %s\n", m.mobo_name);
    printf("GPU:   %s\n", m.gpu_name);
    printf("------------------------\n");

    int sock = zsock_init();
    if (sock < 0)
        return 1;

    while (1)
    {
        struct pollfd pfd = {
            .fd = sock,
            .events = POLLIN};

        // duerme hasta 2 segundos o hasta conexión
        int ret = poll(&pfd, 1, 2000);

        if (ret < 0)
        {
            perror("poll");
            break;
        }

        // timeout -> actualizar métricas
        if (ret == 0)
        {
            metrics_update(&m);

            printf("\rCPU: %.1f%% | RAM: %.2f/%.1f GB | TEMP: %d°C    ",
                   m.cpu_usage,
                   m.mem_used_gb,
                   m.mem_total_gb,
                   m.temp_c);

            fflush(stdout);

            continue;
        }

        // cliente conectado
        if (pfd.revents & POLLIN)
        {
            int client = accept(sock, NULL, NULL);

            if (client >= 0)
            {
                ZHeader h = {
                    .magic = {'Z', 'M', 'E', 'T'},
                    .version = 1,
                    .type = 1,
                    .size = sizeof(ZMetrics)};

                write(client, &h, sizeof(h));
                write(client, &m, sizeof(ZMetrics));

                close(client);
            }
        }
        // ... dentro del while(1) ...
        if (pfd.revents & POLLIN)
        {
            int client = accept(sock, NULL, NULL);
            if (client >= 0)
            {
                // --- 1. INTENTAR LEER COMANDO DEL CLIENTE ---
                ZCommand cmd;
                // MSG_DONTWAIT es clave: si el cliente no mandó nada, no nos quedamos colgados
                ssize_t r = recv(client, &cmd, sizeof(ZCommand), MSG_DONTWAIT);

                if (r == sizeof(ZCommand))
                {
                    printf("\n[ZMETRICS] Orden recibida: %d\n", cmd.opcode);
                    if (cmd.opcode == 1)
                        system("reboot");
                    if (cmd.opcode == 2)
                        system("poweroff");
                }

                // --- 2. ENVIAR MÉTRICAS ---
                zsock_send_metrics(client, &m);

                // --- 3. CERRAR ---
                close(client);
            }
        }
    }

    return 0;
}