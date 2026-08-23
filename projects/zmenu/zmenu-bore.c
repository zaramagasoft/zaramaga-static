#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
static int set_boost(int enabled)
{
    printf("SET BOOST enabled=%d\n", enabled);
    char path[256];

    for (int i = 0;; i++)
    {
        snprintf(
            path,
            sizeof(path),
            "/sys/devices/system/cpu/cpufreq/policy%d/boost",
            i);

        FILE *f = fopen(path, "w");

        if (!f)
        {
            if (errno == ENOENT)
                break;

            return 5;
        }

        if (fprintf(f, "%d\n", enabled ? 1 : 0) < 0)
        {
            fclose(f);
            return 6;
        }

        fclose(f);
    }

    return 0;
}

static int set_governor(const char *governor)
{
    if (strcmp(governor, "performance") != 0 &&
        strcmp(governor, "powersave") != 0)
        return 2;

    char path[256];

    for (int i = 0;; i++)
    {
        snprintf(
            path,
            sizeof(path),
            "/sys/devices/system/cpu/cpufreq/policy%d/scaling_governor",
            i);

        FILE *f = fopen(path, "w");

        if (!f)
        {
            if (errno == ENOENT)
                break;

            return 5;
        }

        if (fprintf(f, "%s\n", governor) < 0)
        {
            fclose(f);
            return 6;
        }

        fclose(f);
    }

    return 0;
}
static int parameter_allowed(const char *p)
{
    if (!p)
        return 0;

    return strcmp(p, "sched_bore") == 0 ||
           strcmp(p, "sched_burst_inherit_type") == 0 ||
           strcmp(p, "sched_burst_smoothness") == 0 ||
           strcmp(p, "sched_burst_penalty_offset") == 0 ||
           strcmp(p, "sched_burst_penalty_scale") == 0 ||
           strcmp(p, "sched_burst_cache_lifetime") == 0 ||
           strcmp(p, "sched_burst_protect_slice_lv") == 0;
}

static int value_valid(const char *p,
                       unsigned long long value)
{
    if (strcmp(p, "sched_bore") == 0)
        return value <= 1;

    if (strcmp(p, "sched_burst_inherit_type") == 0)
        return value <= 2;

    if (strcmp(p, "sched_burst_smoothness") == 0)
        return value <= 3;

    if (strcmp(p, "sched_burst_penalty_offset") == 0)
        return value <= 63;

    if (strcmp(p, "sched_burst_penalty_scale") == 0)
        return value <= 4095;

    if (strcmp(p, "sched_burst_cache_lifetime") == 0)
        return value <= 4294967295ULL;

    if (strcmp(p, "sched_burst_protect_slice_lv") == 0)
        return value <= 2;

    return 0;
}

int main(int argc, char **argv)
{
    char path[256];
    char *end;
    unsigned long long value;
    FILE *f;

    if (argc != 3)
        return 1;

    const char *parameter = argv[1];

    /*
     * =========================================
     * CPU GOVERNOR
     * =========================================
     */

    if (strcmp(parameter, "governor") == 0)
    {
        return set_governor(argv[2]);
    }
    /*
     * =========================================
     * CPU BOOST
     * =========================================
     */

    if (strcmp(parameter, "boost") == 0)
    {
        if (strcmp(argv[2], "0") != 0 &&
            strcmp(argv[2], "1") != 0)
            return 3;

        return set_boost(atoi(argv[2]));
    }
    /*
     * =========================================
     * BORE
     * =========================================
     */

    if (!parameter_allowed(parameter))
        return 2;

    errno = 0;

    value = strtoull(argv[2], &end, 10);

    if (errno != 0 || *end != '\0')
        return 3;

    if (!value_valid(parameter, value))
        return 4;

    snprintf(
        path,
        sizeof(path),
        "/proc/sys/kernel/%s",
        parameter);

    f = fopen(path, "w");

    if (!f)
        return 5;

    if (fprintf(f, "%llu\n", value) < 0)
    {
        fclose(f);
        return 6;
    }

    fclose(f);

    return 0;
}