#ifndef ZMETRICS_H
#define ZMETRICS_H

typedef struct
{
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

#endif