#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bore.h"

/* ============================================================
 * INTERNAL READ FUNCTIONS
 * ============================================================ */

static int read_int(const char *path, int *value)
{
    FILE *f;
    int ret;

    if (!path || !value)
        return -1;

    f = fopen(path, "r");

    if (!f)
        return -1;

    ret = fscanf(f, "%d", value);

    fclose(f);

    return ret == 1 ? 0 : -1;
}

static int read_ull(const char *path,
                    unsigned long long *value)
{
    FILE *f;
    int ret;

    if (!path || !value)
        return -1;

    f = fopen(path, "r");

    if (!f)
        return -1;

    ret = fscanf(f, "%llu", value);

    fclose(f);

    return ret == 1 ? 0 : -1;
}

/* ============================================================
 * BORE DETECTION
 *
 * return:
 *
 *   1  BORE available
 *   0  BORE not available
 *  -1  error
 *
 * enabled:
 *
 *   0  BORE disabled
 *   1  BORE enabled
 * ============================================================ */

int bore_detect(int *enabled)
{
    FILE *f;
    int value;

    if (!enabled)
        return -1;

    f = fopen("/proc/sys/kernel/sched_bore", "r");

    if (!f)
    {

        return 0;
    }

    if (fscanf(f, "%d", &value) != 1)
    {
        fclose(f);
        return -1;
    }

    fclose(f);

    *enabled = value;

    return 1;
}

/* ============================================================
 * READ COMPLETE BORE CONFIGURATION
 * ============================================================ */

int bore_read(BoreConfig *cfg)
{
    if (!cfg)
        return -1;

    memset(cfg, 0, sizeof(*cfg));

    if (read_int(
            "/proc/sys/kernel/sched_bore",
            &cfg->bore) < 0)
    {
        //cfg->boreDisponible=1;
        return -1;
    }

    if (read_int(
            "/proc/sys/kernel/sched_burst_inherit_type",
            &cfg->inherit_type) < 0)
        return -1;

    if (read_int(
            "/proc/sys/kernel/sched_burst_smoothness",
            &cfg->smoothness) < 0)
        return -1;

    if (read_int(
            "/proc/sys/kernel/sched_burst_penalty_offset",
            &cfg->penalty_offset) < 0)
        return -1;

    if (read_int(
            "/proc/sys/kernel/sched_burst_penalty_scale",
            &cfg->penalty_scale) < 0)
        return -1;

    if (read_ull(
            "/proc/sys/kernel/sched_burst_cache_lifetime",
            &cfg->cache_lifetime) < 0)
        return -1;

    if (read_int(
            "/proc/sys/kernel/sched_burst_protect_slice_lv",
            &cfg->protect_slice_lv) < 0)
        return -1;
    cfg->boreDisponible = 1;
    return 0;
}

/* ============================================================
 * CHECK ALLOWED PARAMETER
 * ============================================================ */

static int bore_parameter_allowed(const char *parameter)
{
    if (!parameter)
        return 0;

    if (strcmp(parameter, "sched_bore") == 0)
        return 1;

    if (strcmp(parameter, "sched_burst_inherit_type") == 0)
        return 1;

    if (strcmp(parameter, "sched_burst_smoothness") == 0)
        return 1;

    if (strcmp(parameter, "sched_burst_penalty_offset") == 0)
        return 1;

    if (strcmp(parameter, "sched_burst_penalty_scale") == 0)
        return 1;

    if (strcmp(parameter, "sched_burst_cache_lifetime") == 0)
        return 1;

    if (strcmp(parameter, "sched_burst_protect_slice_lv") == 0)
        return 1;

    return 0;
}

/* ============================================================
 * CHECK PARAMETER VALUE
 * ============================================================ */

static int bore_value_valid(const char *parameter,
                            unsigned long long value)
{
    if (!parameter)
        return 0;

    if (strcmp(parameter, "sched_bore") == 0)
        return value <= 1;

    if (strcmp(parameter, "sched_burst_inherit_type") == 0)
        return value <= 2;

    if (strcmp(parameter, "sched_burst_smoothness") == 0)
        return value <= 3;

    if (strcmp(parameter, "sched_burst_penalty_offset") == 0)
        return value <= 63;

    if (strcmp(parameter, "sched_burst_penalty_scale") == 0)
        return value <= 4095;

    if (strcmp(parameter, "sched_burst_cache_lifetime") == 0)
        return value <= 4294967295ULL;

    if (strcmp(parameter, "sched_burst_protect_slice_lv") == 0)
        return value <= 2;

    return 0;
}

/* ============================================================
 * SET BORE PARAMETER
 * ============================================================ */

int bore_set(const char *parameter,
             unsigned long long value,
             BoreConfig *cfg)
{
    char command[512];

    if (!parameter)
        return -1;

    if (!bore_parameter_allowed(parameter))
        return -1;

    if (!bore_value_valid(parameter, value))
        return -1;

    snprintf(
        command,
        sizeof(command),
        "sudo -n /usr/local/libexec/zmenu-bore '%s' '%llu'",
        parameter,
        value);

    printf(
        "BORE SET: %s = %llu\n",
        parameter,
        value);

    int ret = system(command);

    if (ret != 0)
    {
        printf(
            "BORE SET ERROR: helper ret=%d\n",
            ret);

        return -1;
    }

    /*
     * Volvemos a leer los valores reales
     * del kernel.
     */
    if (cfg)
    {
        if (bore_read(cfg) < 0)
            return -1;
    }

    printf(
        "BORE SET OK: %s = %llu\n",
        parameter,
        value);

    return 0;
}

/* ============================================================
 * PRINT CONFIGURATION
 * ============================================================ */

void bore_print(const BoreConfig *cfg)
{
    if (!cfg)
        return;

    printf("BORE configuration\n");
    printf("------------------\n");
    printf("boreDisponibleeee            = %d\n",
           cfg->boreDisponible);
    printf("sched_bore                   = %d\n",
           cfg->bore);

    printf("sched_burst_inherit_type     = %d\n",
           cfg->inherit_type);

    printf("sched_burst_smoothness       = %d\n",
           cfg->smoothness);

    printf("sched_burst_penalty_offset   = %d\n",
           cfg->penalty_offset);

    printf("sched_burst_penalty_scale    = %d\n",
           cfg->penalty_scale);

    printf("sched_burst_cache_lifetime   = %llu\n",
           cfg->cache_lifetime);

    printf("sched_burst_protect_slice_lv = %d\n",
           cfg->protect_slice_lv);
}