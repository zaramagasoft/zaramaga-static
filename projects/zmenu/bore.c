#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int bore;
    int inherit_type;
    int smoothness;
    int penalty_offset;
    int penalty_scale;
    unsigned long long cache_lifetime;
    int protect_slice_lv;
} BoreConfig;

static int read_int(const char *path, int *value)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    int ret = fscanf(f, "%d", value);
    fclose(f);

    return ret == 1 ? 0 : -1;
}

static int read_ull(const char *path, unsigned long long *value)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    int ret = fscanf(f, "%llu", value);
    fclose(f);

    return ret == 1 ? 0 : -1;
}

int bore_read(BoreConfig *cfg)
{
    if (!cfg)
        return -1;

    memset(cfg, 0, sizeof(*cfg));

    if (read_int("/proc/sys/kernel/sched_bore",
                 &cfg->bore) < 0)
        return -1;

    if (read_int("/proc/sys/kernel/sched_burst_inherit_type",
                 &cfg->inherit_type) < 0)
        return -1;

    if (read_int("/proc/sys/kernel/sched_burst_smoothness",
                 &cfg->smoothness) < 0)
        return -1;

    if (read_int("/proc/sys/kernel/sched_burst_penalty_offset",
                 &cfg->penalty_offset) < 0)
        return -1;

    if (read_int("/proc/sys/kernel/sched_burst_penalty_scale",
                 &cfg->penalty_scale) < 0)
        return -1;

    if (read_ull("/proc/sys/kernel/sched_burst_cache_lifetime",
                  &cfg->cache_lifetime) < 0)
        return -1;

    if (read_int("/proc/sys/kernel/sched_burst_protect_slice_lv",
                 &cfg->protect_slice_lv) < 0)
        return -1;

    return 0;
}

void bore_print(const BoreConfig *cfg)
{
    if (!cfg)
        return;

    printf("BORE configuration\n");
    printf("------------------\n");
    printf("sched_bore                  = %d\n", cfg->bore);
    printf("sched_burst_inherit_type    = %d\n", cfg->inherit_type);
    printf("sched_burst_smoothness      = %d\n", cfg->smoothness);
    printf("sched_burst_penalty_offset  = %d\n", cfg->penalty_offset);
    printf("sched_burst_penalty_scale   = %d\n", cfg->penalty_scale);
    printf("sched_burst_cache_lifetime  = %llu\n",
           cfg->cache_lifetime);
    printf("sched_burst_protect_slice_lv = %d\n",
           cfg->protect_slice_lv);
}
int main(void)
{
    BoreConfig cfg;

    if (bore_read(&cfg) < 0) {
        fprintf(stderr, "Error leyendo parametros BORE\n");
        return 1;
    }

    bore_print(&cfg);

    return 0;
}