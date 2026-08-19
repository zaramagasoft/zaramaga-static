#ifndef BORE_H
#define BORE_H

typedef struct
{
    int bore;
    int inherit_type;
    int smoothness;
    int penalty_offset;
    int penalty_scale;
    unsigned long long cache_lifetime;
    int protect_slice_lv;
} BoreConfig;

int bore_detect(int *enabled);

int bore_read(BoreConfig *cfg);

int bore_set(const char *parameter,
             unsigned long long value,
             BoreConfig *cfg);

void bore_print(const BoreConfig *cfg);

#endif