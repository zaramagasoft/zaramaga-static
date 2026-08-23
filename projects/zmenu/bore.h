#ifndef BORE_H
#define BORE_H
#define BORE_DEFAULT 1
#define BORE_DEFAULT_INHERIT 2
#define BORE_DEFAULT_SMOOTHNESS 1
#define BORE_DEFAULT_PENALTY 24
#define BORE_DEFAULT_SCALE 1536
#define BORE_DEFAULT_CACHE_NS 75000000ULL
#define BORE_DEFAULT_PROTECT 1
typedef struct
{
    int bore;
    int inherit_type;
    int smoothness;
    int penalty_offset;
    int penalty_scale;
    unsigned long long cache_lifetime;
    int protect_slice_lv;
    int boreDisponible;
    int boost;
    int showBoreMenu;
} BoreConfig;
static int set_boost(int enabled);
int bore_detect(int *enabled);

int bore_read(BoreConfig *cfg);

int bore_set(const char *parameter,
             unsigned long long value,
             BoreConfig *cfg);

void bore_print(const BoreConfig *cfg);

#endif