#ifndef METRICAS_H
#define METRICAS_H

typedef struct {
    float net_download_mb;
    float net_upload_mb;
    float cpu_usage;
    float mem_used_gb;
    float mem_total_gb;

    int temp_c;
    char cpu_model[64];
    char mobo_name[64];
    char gpu_name[64];
} ZMetrics;
typedef struct
{
    int opcode; // 1: REBOOT, 2: POWEROFF, 3: NADA
    char payload[256];
} ZCommand;
void metrics_init(ZMetrics *m);
void metrics_update(ZMetrics *m);

#endif