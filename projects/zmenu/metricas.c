#include "metricas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

static char temp_path[256] = "";
static long last_total = 0, last_idle = 0;

static unsigned long long last_rx = 0;
static unsigned long long last_tx = 0;
static char net_iface[32] = "";
static float rx_avg = 0.0f;
static float tx_avg = 0.0f;

// Función auxiliar para leer strings de archivos del sistema
void read_sys_file(const char *path, char *dest, int size)
{
    FILE *f = fopen(path, "r");
    if (f)
    {
        fgets(dest, size, f);
        dest[strcspn(dest, "\n")] = 0; // Quitar el salto de línea
        fclose(f);
    }
    else
    {
        strncpy(dest, "Desconocido", size);
    }
}

void metrics_init(ZMetrics *m)
{
    // 1. Modelo de CPU (de /proc/cpuinfo)
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
        char line[256];
        while (fgets(line, sizeof(line), f))
        {
            if (strncmp(line, "model name", 10) == 0)
            {
                char *name = strchr(line, ':') + 2;
                strncpy(m->cpu_model, name, 63);
                m->cpu_model[strcspn(m->cpu_model, "\n")] = 0;
                break;
            }
        }
        fclose(f);
    }

    // 2. Placa Base (DMI)
    read_sys_file("/sys/class/dmi/id/board_name", m->mobo_name, 64);

    // 3. Gráfica (GPU) - Buscamos en PCI
    // Esto es un resumen, suele estar en /sys/class/drm/card0/device/device
    // Por simplicidad en modo texto, usamos una ruta común de identificación
    // read_sys_file("/sys/class/drm/card0/device/uevent", m->gpu_name, 64);
    // Nota: El uevent es sucio, mañana si quieres lo pulimos con libpci
    // Buscamos el nombre de la GPU en el subsistema DRM
    struct dirent *de_gpu;
    DIR *dr_gpu = opendir("/sys/class/drm");
    if (dr_gpu)
    {
        while ((de_gpu = readdir(dr_gpu)) != NULL)
        {
            // Buscamos "card0" o "card1", evitando los "renderD128"
            if (strncmp(de_gpu->d_name, "card", 4) == 0 && strlen(de_gpu->d_name) < 7)
            {
                char vendor_path[256], device_path[256];
                char vendor_id[16] = {0}, device_id[16] = {0};

                snprintf(vendor_path, sizeof(vendor_path), "/sys/class/drm/%s/device/vendor", de_gpu->d_name);
                snprintf(device_path, sizeof(device_path), "/sys/class/drm/%s/device/device", de_gpu->d_name);

                FILE *fv = fopen(vendor_path, "r");
                FILE *fd = fopen(device_path, "r");

                if (fv && fd)
                {
                    fscanf(fv, "%s", vendor_id);
                    fscanf(fd, "%s", device_id);

                    // Mapeo rápido de Vendors comunes
                    const char *v_name = "GPU";
                    if (strstr(vendor_id, "0x1002"))
                        v_name = "AMD";
                    else if (strstr(vendor_id, "0x10de"))
                        v_name = "NVIDIA";
                    else if (strstr(vendor_id, "0x8086"))
                        v_name = "Intel";

                    snprintf(m->gpu_name, 64, "%s (%s:%s)", v_name, vendor_id, device_id);

                    fclose(fv);
                    fclose(fd);
                    break; // Ya tenemos la primaria
                }
                if (fv)
                    fclose(fv);
                if (fd)
                    fclose(fd);
            }
        }
        closedir(dr_gpu);
    }

    // 4. Buscar sensor de temperatura (Agnóstico)
    struct dirent *de;
    DIR *dr = opendir("/sys/class/hwmon");
    // En metrics_init
    if (dr)
    {
        while ((de = readdir(dr)) != NULL)
        {
            if (de->d_name[0] == 'h')
            {
                char name_path[256], name[64];
                snprintf(name_path, sizeof(name_path), "/sys/class/hwmon/%s/name", de->d_name);
                FILE *fn = fopen(name_path, "r");
                if (fn)
                {
                    if (fscanf(fn, "%s", name) == 1)
                    {
                        // Añadimos más drivers comunes en Intel Mobile/Atom
                        if (strcmp(name, "coretemp") == 0 ||
                            strcmp(name, "k10temp") == 0 ||
                            strcmp(name, "acpitz") == 0 ||
                            strcmp(name, "intel_soc_dts_thermal") == 0)
                        {
                            // Intentamos temp1, pero si no, Rust suele mirar temp2
                            // Por ahora aseguremos la ruta base
                            snprintf(temp_path, sizeof(temp_path), "/sys/class/hwmon/%s/temp1_input", de->d_name);

                            // TEST RÁPIDO: ¿Existe el archivo?
                            if (access(temp_path, F_OK) == -1)
                            {
                                // Si temp1 no existe, probamos temp2 (típico de Celeron)
                                snprintf(temp_path, sizeof(temp_path), "/sys/class/hwmon/%s/temp2_input", de->d_name);
                            }
                        }
                    }
                    fclose(fn);
                }
            }
        }
        closedir(dr);
    }
    // =========================================
    // Detectar interfaz de red principal
    // =========================================
    FILE *f_net = fopen("/proc/net/dev", "r");
    if (f_net)
    {
        char line[256];

        // Saltamos las dos primeras líneas de cabecera
        fgets(line, sizeof(line), f_net);
        fgets(line, sizeof(line), f_net);

        while (fgets(line, sizeof(line), f_net))
        {
            char iface[32];

            // Leemos el nombre antes de los ':'
            if (sscanf(line, " %31[^:]:", iface) == 1)
            {
                // Ignoramos loopback
                if (strcmp(iface, "lo") != 0)
                {
                    strncpy(net_iface, iface, sizeof(net_iface) - 1);
                    net_iface[sizeof(net_iface) - 1] = '\0';

                    printf("Interfaz detectada: %s\n", net_iface);
                    break;
                }
            }
        }

        fclose(f_net);
    }
}

void metrics_update(ZMetrics *m)
{
    // RAM en GB
    FILE *f_mem = fopen("/proc/meminfo", "r");
    if (f_mem)
    {
        long total = 0, avail = 0;
        char buf[256];
        while (fgets(buf, sizeof(buf), f_mem))
        {
            if (sscanf(buf, "MemTotal: %ld", &total) == 1)
                m->mem_total_gb = total / 1024.0 / 1024.0;
            if (sscanf(buf, "MemAvailable: %ld", &avail) == 1)
            {
                m->mem_used_gb = m->mem_total_gb - (avail / 1024.0 / 1024.0);
                break;
            }
        }
        fclose(f_mem);
    }

    // CPU %
    FILE *f_stat = fopen("/proc/stat", "r");
    if (f_stat)
    {
        long u, n, s, i, iw, irq, sirq;
        fscanf(f_stat, "cpu  %ld %ld %ld %ld %ld %ld %ld", &u, &n, &s, &i, &iw, &irq, &sirq);
        fclose(f_stat);
        long cur_i = i + iw;
        long cur_t = u + n + s + i + iw + irq + sirq;
        m->cpu_usage = 100.0 * (1.0 - ((float)(cur_i - last_idle) / (float)(cur_t - last_total)));
        last_idle = cur_i;
        last_total = cur_t;
    }

    // Temp
    // Temp con Filtro de Cordura
    // Temp - Versión Robusta
    if (temp_path[0] != '\0')
    {
        FILE *ft = fopen(temp_path, "r");
        if (ft)
        {
            char temp_buf[32] = {0};
            if (fgets(temp_buf, sizeof(temp_buf), ft))
            {
                long t_val = atol(temp_buf);
                // En Linux, temp siempre es miligrados. Divide siempre.
                int temp_final = (int)(t_val / 1000);

                // Filtro de seguridad: si es absurdo, ponemos 0
                if (temp_final > -20 && temp_final < 125)
                    m->temp_c = temp_final;
                else
                    m->temp_c = 0;
            }
            fclose(ft);
        }
    }
    // download updload
    FILE *f_net = fopen("/proc/net/dev", "r");
    if (f_net)
    {
        char line[256];

        fgets(line, sizeof(line), f_net);
        fgets(line, sizeof(line), f_net);

        while (fgets(line, sizeof(line), f_net))
        {
            if (strstr(line, net_iface))
            {
                unsigned long long rx, tx;

                sscanf(line,
                       " %*[^:]: %llu %*u %*u %*u %*u %*u %*u %*u %llu",
                       &rx,
                       &tx);

                if (last_rx != 0)
                {
                    unsigned long long delta_rx = rx - last_rx;
                    unsigned long long delta_tx = tx - last_tx;

                    /*  m->net_download_mb = (float)delta_rx / (1024.0f * 1024.0f * 2.0f);
                     m->net_upload_mb = (float)delta_tx / (1024.0f * 1024.0f * 2.0f);
  */
                    float down = (float)delta_rx / (1024.0f * 1024.0f * 2.0f);
                    float up = (float)delta_tx / (1024.0f * 1024.0f * 2.0f);

                    // Media exponencial (70% anterior, 30% nueva)
                    rx_avg = rx_avg * 0.25f + down * 0.75f;
                    tx_avg = tx_avg * 0.25f + up * 0.75f;

                    m->net_download_mb = rx_avg;
                    m->net_upload_mb = tx_avg;
                    if (m->net_download_mb < 0.01f)
                        m->net_download_mb = 0.0f;

                    if (m->net_upload_mb < 0.01f)
                        m->net_upload_mb = 0.0f;
                    printf("RX:%llu  TX:%llu  ↓ %.2f MB/s  ↑ %.2f MB/s\n",
                           rx,
                           tx,
                           m->net_download_mb,
                           m->net_upload_mb);
                }

                last_rx = rx;
                last_tx = tx;

                break;
            }
        }

        fclose(f_net);
    }
}