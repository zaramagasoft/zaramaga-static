#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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