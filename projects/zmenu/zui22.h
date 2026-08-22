extern char *mi_buffer[1];
#include "zmetrics.h"
#include "bore.h"
extern ZMetrics *metricasZui;

#ifndef ZUI22_H
#define ZUI22_H

#include "nuklear.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <time.h>   // Para time, localtime, strftime
#include <stdint.h> // Para uint32_t
#include <signal.h> // Para kill y SIGTERM
#include <unistd.h> // Para getpgrp
#include <string.h>
#include <sys/socket.h> // Para socket(), setsockopt(), SOL_SOCKET...
#include <sys/un.h>     // Para la estructura sockaddr_un y AF_UNIX
#include <arpa/inet.h>
#include <fcntl.h> // Opcional, pero ayuda con estructuras de red    // Para strcat
#define SOCKET_PATH "/tmp/zmetrics.sock"
#include "zui_hover.h"
///// globales /////
struct nk_style_button bore_normal;
struct nk_style_button bore_active;
struct nk_style_button bore_disabled;
// Variables de fecha/hora
char time_str[10];
char date_str[20];
// --- Métricas del sistema (ZaramagaOS) ---
static float sys_cpu = 0.0f;
static float sys_mem_u = 0.0f;
static float sys_mem_t = 0.0f;
static int sys_temp = 0;
extern int zmenu_visible;
static int cpu_mode = 1;
extern bool logo_dirty;
// prueba puntero a struct compartida
extern struct wl_surface *surfGlobal;
struct shared_metrics
{
    float cpu;
    float mem_u;
    float mem_t;
    int temp;
    int volume;
};

// LA CLAVE: Esto dice "m_shared existe fuera de este archivo"
extern struct shared_metrics *m_shared;
static int show_confirm = 0; // 0: nada, 1: reboot, 2: poweroff
                             // Definimos los colores aquí arriba para que todas las funciones los vean
struct nk_color dark_bg;
struct nk_color phosphor_green;
struct nk_color dark_green;
struct nk_style_button estilo_original;
struct nk_style_button miestilo; // ✅ Copia directa
int contador = 0;
static float bright_value = 1.0f;
static float contrast_value = 1.0f;
static float gamma_value = 1.0f;

static float vol_value = 0.6f;
// static float bright_value = 0.8f;
typedef struct
{
    pthread_t thread;
    bool running;
    int last_ping_ms; // Variable para almacenar el último ping
} PingWorker;
extern PingWorker *ping; // Variable global para el PingWorker
typedef enum
{
    SLIDER_INT,
    SLIDER_UINT,
    SLIDER_FLOAT
} SliderType;

static void z_reboot(void)
{
    if (access("/run/systemd/system", F_OK) == 0)
        system("systemctl reboot");
    else
        system("loginctl reboot");
}

static void z_poweroff(void)
{
    if (access("/run/systemd/system", F_OK) == 0)
        system("systemctl poweroff");
    else
        system("loginctl poweroff");
}

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
/// definiciones ////
static void sync_cpu_mode_from_governor(void);
static int bore_slider_factory(
    struct nk_context *ctx,
    float y,
    float win_width,
    const char *label,
    SliderType type,
    void *value,
    double min,
    double max,
    double step,
    struct nk_rect *hover);
static void *ping_thread(void *arg);
void ping_start(PingWorker *ping);
void ping_stop(PingWorker *ping);
static int bore_slider_factory(
    struct nk_context *ctx,
    float y,
    float win_width,
    const char *label,
    SliderType type,
    void *value,
    double min,
    double max,
    double step,
    struct nk_rect *hover)
{
    const float icon_w = 0.0f;
    const float label_w = 90.0f;
    const float value_w = 55.0f;
    const float row_h = 20.0f;

    float slider_w =
        win_width - (2.0f + icon_w + label_w + value_w);
    nk_layout_space_begin(ctx, NK_STATIC, row_h, 3);
    /* LABEL */
    nk_layout_space_push(ctx,
                         nk_rect(icon_w, y, label_w, row_h * 2));

    nk_label(ctx, label, NK_TEXT_LEFT);

    /* SLIDER */
    nk_layout_space_push(ctx,
                         nk_rect(
                             icon_w + label_w,
                             y,
                             slider_w,
                             row_h * 2));

    struct nk_rect bounds = nk_widget_bounds(ctx);

    if (hover)
        *hover = bounds;

    int changed = 0;

    switch (type)
    {
    case SLIDER_INT:
    {
        int *v = value;

        changed = nk_slider_int(
            ctx,
            (int)min,
            v,
            (int)max,
            (int)step);
        break;
    }

    case SLIDER_UINT:
    {
        uint32_t *v = value;

        int tmp = (int)*v;

        changed = nk_slider_int(
            ctx,
            (int)min,
            &tmp,
            (int)max,
            (int)step);

        if (changed)
            *v = (uint32_t)tmp;

        break;
    }

    case SLIDER_FLOAT:
    {
        float *v = value;

        changed = nk_slider_float(
            ctx,
            (float)min,
            v,
            (float)max,
            (float)step);
        break;
    }
    }

    /* VALOR */
    char buffer[32];

    switch (type)
    {
    case SLIDER_INT:
        snprintf(buffer, sizeof(buffer),
                 "%d", *(int *)value);
        break;

    case SLIDER_UINT:
        snprintf(buffer, sizeof(buffer),
                 "%u", *(uint32_t *)value);
        break;

    case SLIDER_FLOAT:
        snprintf(buffer, sizeof(buffer),
                 "%.2f", *(float *)value);
        break;
    }

    nk_layout_space_push(ctx,
                         nk_rect(
                             1.0f + icon_w + label_w + slider_w,
                             y,
                             value_w,
                             row_h * 2));

    nk_label(ctx, buffer, NK_TEXT_LEFT);

    return changed;
}
void obtener_gamma_del_servicio(float *b, float *c, float *g)
{
    // 1. Forzamos al sistema a enviar la 'q' y cerrar el pipe
    //
    static int fd = -1;

    if (fd == -1)
        fd = open("/tmp/gamma_pipe", O_WRONLY);

    write(fd, "q", 1);
    usleep(1000); // Esperamos un poco para que el servicio procese la 'q'
                  // close(fd);
    // 2. Leemos la respuesta de un archivo temporal
    // Para que sea ultra sencillo, hagamos que el server escriba el resultado en un .txt
    // Es mucho más fiable que intentar leer el pipe de vuelta.

    FILE *f = fopen("/tmp/gamma_state.txt", "r");
    if (f)
    {
        char buf[64];
        if (fgets(buf, sizeof(buf), f))
        {
            sscanf(buf, "v %f %f %f", b, c, g);
            // printf("obtener_gamma_del_servicio: bright=%f, contrast=%f, gamma=%f\n", *b, *c, *g);
        }
        fclose(f);
    }
}
int logoDraw(struct nk_command_buffer *canvas, float y, float win_width, float logo_h);
int datedraw(struct nk_context *ctx, float y, float win_width);
int voldraw(struct nk_context *ctx, float y, float win_width, float middle_h);
int kernelraw(struct nk_context *ctx, float y, float win_width, float middle_h);
int metricsDraw(struct nk_context *ctx, float y, float win_width, float footer_h);
int gammaDraw(struct nk_context *ctx, float y, float win_width); // Declaración de gammaDraw
int pingDraw(struct nk_context *ctx, float y, float win_width, PingWorker *ping);
int upDownDraw(struct nk_context *ctx, float y, float win_width); // Declaración de gammaDraw
int boreDraw(struct nk_context *ctx, float y, float win_width, BoreConfig *bore);
int modeDraw(struct nk_context *ctx, float y, float win_width, BoreConfig *bore);
// DECLARACIÓN QUE TE FALTA:
void enviar_comando_gamma(char cmd, float valor);
#include <errno.h>

int gammaDraw(struct nk_context *ctx, float y, float win_width)
{
    uint64_t t = now_ns();
    // obtener_gamma_del_servicio(&bright_value, &contrast_value, &gamma_value);
    printf("gamma service %lu us\n", (now_ns() - t) / 1000);
    float row_h = 20.0f;
    int offset = 30;
    float row_height = 20.0f; // La altura que reservamos para este bloque
    float icon_w = 30.0f;
    float label_w = 60.0f;
    float value_w = 40.0f;
    float slider_w = win_width - (1 * 2 + icon_w + label_w + value_w);
    // float paddingM = 20.0f;
    float slider_h = 55.0f;
    y += offset;
    // printf("gammaDraw: y inicial = %f\n", y);
    //  Usamos row_dynamic para que los elementos se posicionen solos
    //  Esto evita que los sliders se dibujen arriba del todo
    //  nk_layout_row_dynamic(ctx, row_h, 1);

    // BRILLOOOOO
    // iconobrillo
    nk_layout_space_push(ctx,
                         nk_rect(0, y, icon_w, row_height * 2));
    nk_label(ctx, "\uf185", NK_TEXT_CENTERED);

    // LABEL brillo
    nk_layout_space_push(ctx,
                         nk_rect(1 + icon_w, y, label_w, row_height * 2));
    nk_label(ctx, "BRIGHT", NK_TEXT_LEFT);

    nk_layout_space_push(ctx,
                         nk_rect(icon_w + label_w, y, slider_w - offset, row_height * 2));

    // Slider Brillo
    // nk_layout_space_push(ctx,
    // nk_rect(1 + icon_w + label_w, y, slider_w - offset, row_height * 2));
    // nk_label(ctx, "BRIGHT", NK_TEXT_LEFT);
    struct nk_rect bounds = nk_widget_bounds(ctx);
    g_hover.bright = bounds;
    if (nk_slider_float(ctx, 0.1f, &bright_value, 2.0f, 0.05f))
    {
        enviar_comando_gamma('b', bright_value);
        // obtener_gamma_del_servicio(&bright_value, &contrast_value, &gamma_value);
    }
    // VALOR BRILLO
    char buffer[16];
    sprintf(buffer, "%d%%", (int)(bright_value * 100));

    nk_layout_space_push(ctx,
                         nk_rect(1 + icon_w + label_w + slider_w - offset, y, value_w, row_height * 2));
    nk_label(ctx, buffer, NK_TEXT_CENTERED);
    // y = y + row_h;
    //  icono contraste
    y = y + row_h;
    nk_layout_space_push(ctx,
                         nk_rect(0, y, icon_w, row_height * 2));
    nk_label(ctx, "\uf042", NK_TEXT_CENTERED);
    // label Contraste
    nk_layout_space_push(ctx,
                         nk_rect(0 + icon_w, y, slider_w, row_height * 2));

    nk_label(ctx, "CONTRA", NK_TEXT_LEFT);
    // slider contraste
    nk_layout_space_push(ctx,
                         nk_rect(icon_w + label_w, y, slider_w - offset, row_height * 2));
    bounds = nk_widget_bounds(ctx);
    g_hover.contrast = bounds;
    if (nk_slider_float(ctx, 0.1f, &contrast_value, 2.0f, 0.05f))
    {
        enviar_comando_gamma('c', contrast_value);
    }
    // VALOR CONTRASTE

    char bufferC[16];
    sprintf(bufferC, "%d%%", (int)(contrast_value * 100));

    nk_layout_space_push(ctx,
                         nk_rect(1 + icon_w + label_w + slider_w - offset, y, value_w, row_height * 2));
    nk_label(ctx, bufferC, NK_TEXT_CENTERED);
    y = y + row_h;
    //  gama
    // icono gamma
    y = y + row_h;
    nk_layout_space_push(ctx,
                         nk_rect(0, y, icon_w, row_height * 2));
    nk_label(ctx, "\uf0eb", NK_TEXT_CENTERED);
    // label gamma
    nk_layout_space_push(ctx,
                         nk_rect(0 + icon_w, y, slider_w, row_height * 2));

    nk_label(ctx, "GAMMA", NK_TEXT_LEFT);
    // Slider Gamma
    nk_layout_space_push(ctx,
                         nk_rect(icon_w + label_w, y, slider_w - offset, row_height * 2));
    bounds = nk_widget_bounds(ctx);
    g_hover.gamma = bounds;
    if (nk_slider_float(ctx, 0.1f, &gamma_value, 2.0f, 0.05f))
    {
        enviar_comando_gamma('g', gamma_value);
    }
    // VALOR GAMMA
    char bufferG[16];
    sprintf(bufferG, "%d%%", (int)(gamma_value * 100));

    nk_layout_space_push(ctx,
                         nk_rect(1 + icon_w + label_w + slider_w - offset, y, value_w, row_height * 2));
    nk_label(ctx, bufferG, NK_TEXT_CENTERED);

    // obtener_gamma_del_servicio(&bright_value, &contrast_value, &gamma_value);
    y = y + row_h;
    return (int)(y); // Retornamos el nuevo espacio ocupado
}
// Copiamos tu función ganadora del cliente.c
static int zui_read_full(int sock, void *buf, size_t len)
{
    size_t off = 0;
    while (off < len)
    {
        ssize_t r = read(sock, (char *)buf + off, len - off);
        if (r <= 0)
            return 0;
        off += r;
    }
    return 1;
}

#include <errno.h>

// 🔊 VOLUMEN
static void zui_set_volume(float v)
{
    int vol = (int)(v * 100.0f);
    char cmd[64];
    snprintf(cmd, sizeof(cmd),
             "pactl set-sink-volume @DEFAULT_SINK@ %d%%", vol);
    system(cmd);
}
// --- LÓGICA DE AUDIO (ABSTRACCIÓN) ---
int GetSystemVolume()
{
    int volume = 0;
    // Este comando de pactl es estándar y muy rápido
    FILE *fp = popen("pactl get-sink-volume @DEFAULT_SINK@ | grep -Po '\\d+(?=%)' | head -n 1", "r");

    if (fp != NULL)
    {
        char buffer[16];
        if (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            volume = atoi(buffer);
        }
        pclose(fp);
    }
    return volume;
}

void UpdateVolume(int delta)
{
    vol_value = GetSystemVolume();
    vol_value += delta;
    if (vol_value < 0)
        vol_value = 0;
    if (vol_value >= 150)
        vol_value = 150;

    // Ejecución en segundo plano para no congelar el frame
    char cmd[64];
    sprintf(cmd, "pactl set-sink-volume @DEFAULT_SINK@ %d%% &", (int)(vol_value * 100));
    system(cmd);
}

void zui_init_colors()
{
    dark_bg = nk_rgba(10, 15, 10, 230);
    phosphor_green = nk_rgb(51, 255, 51);
    dark_green = nk_rgb(20, 60, 20);
    obtener_gamma_del_servicio(&bright_value, &contrast_value, &gamma_value);
    /*
     * =========================
     * BORE NORMAL
     * =========================
     */

    bore_normal.normal =
        nk_style_item_color(nk_rgb(25, 80, 25));

    bore_normal.hover =
        nk_style_item_color(nk_rgb(40, 110, 40));

    bore_normal.active =
        nk_style_item_color(nk_rgb(55, 140, 55));

    bore_normal.text_normal =
        phosphor_green;

    bore_normal.text_hover =
        phosphor_green;

    bore_normal.text_active =
        phosphor_green;

    /*
     * =========================
     * BORE ACTIVO
     * =========================
     */
    bore_active.normal =
        nk_style_item_color(nk_rgb(70, 70, 70));

    bore_active.hover =
        nk_style_item_color(nk_rgb(90, 90, 90));

    bore_active.active =
        nk_style_item_color(nk_rgb(110, 110, 110));

    bore_active.text_normal =
        nk_rgb(255, 255, 255);

    bore_active.text_hover =
        nk_rgb(255, 255, 255);

    bore_active.text_active =
        nk_rgb(255, 255, 255);

    /*
     * =========================
     * BORE DESHABILITADO
     * =========================
     */

    bore_disabled.normal =
        nk_style_item_color(nk_rgb(25, 25, 25));

    bore_disabled.hover =
        bore_disabled.normal;

    bore_disabled.active =
        bore_disabled.normal;

    bore_disabled.text_normal =
        nk_rgb(70, 70, 70);

    bore_disabled.text_hover =
        nk_rgb(70, 70, 70);

    bore_disabled.text_active =
        nk_rgb(70, 70, 70);
}

void zui_set_style(struct nk_context *ctx)
{
    obtener_gamma_del_servicio(&bright_value, &contrast_value, &gamma_value);

    zui_init_colors();

    ctx->style.slider.bar_height = 30.0f;
    ctx->style.slider.cursor_size = nk_vec2(46, 46);

    // VENTANA
    ctx->style.window.fixed_background = nk_style_item_color(dark_bg);
    ctx->style.window.border_color = phosphor_green;
    ctx->style.window.border = 2.0f;

    // BOTONES
    ctx->style.button.normal = nk_style_item_color(dark_bg);
    ctx->style.button.hover = nk_style_item_color(dark_green);
    ctx->style.button.active = nk_style_item_color(phosphor_green);
    ctx->style.button.border_color = phosphor_green;

    // El texto del botón sí es un nk_color, no un style_item
    ctx->style.button.text_normal = phosphor_green;
    ctx->style.button.text_hover = nk_rgb(255, 255, 255);

    // SLIDERS (Corregido para ZaramagaOS)
    ctx->style.slider.bar_normal = dark_green;
    ctx->style.slider.bar_active = phosphor_green;                            // Color de la barra "rellena"
    ctx->style.slider.cursor_normal = nk_style_item_color(nk_rgb(0, 205, 0)); // 👈 EL TIRADOR
    ctx->style.slider.cursor_hover = nk_style_item_color(phosphor_green);
    ctx->style.slider.cursor_active = nk_style_item_color(phosphor_green);

    ctx->style.slider.cursor_size = nk_vec2(25, 25); // Tamaño del cuadrado
    ctx->style.slider.bar_height = 6.0f;             // Un poco más gruesa para que se vea industrial
    ctx->style.slider.rounding = 0;                  // Cuadrado puro
    // TEXTO
    ctx->style.text.color = phosphor_green;
}
void zui_render(struct nk_context *ctx, int win_width, int win_height, BoreConfig *bore_cfg)
{
    uint64_t t; // Para medir el tiempo de ejecución de cada sección
    t = now_ns();
    estilo_original = ctx->style.button; // Guardamos el estilo original del botón
    miestilo = estilo_original;          // Inicializamos mi_estilo con el original
    // printf("winheightzUI:%f \n", win_height);
    // obtener_gamma_del_servicio(&bright_value, &contrast_value, &gamma_value);
    // printf("zui_render %d\n", contador++);
    //  fflush(stdout); // Esto te ayudará a ver cuándo se llama a zui_render
    static float last_sys_vol = -1.0f;
    // --- LÓGICA DE TIEMPO ---
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Formateamos HH:MM y la fecha (ej: 29 Abr)
    strftime(time_str, sizeof(time_str), "%H:%M", timeinfo);
    strftime(date_str, sizeof(date_str), "%d %b %Y", timeinfo);
    // strftime(date_str, sizeof(date_str), "%d %b", timeinfo);
    //////zmetrics////////////
    /* while (metricasZui == NULL)
    {
        if (metricasZui== NULL)
        {
            printf("Métricas aún no disponibles en zui_render, esperando...\n");
        }

        printf("Esperando a que metricasZui esté disponible...\n");
        usleep(2000000); // Espera 100ms antes de volver a comprobar
    }
    printf("Métricas en zui_render: CPU=%.1f%%, RAM=%.2f/%.2fGB, Temp=%d°C\n",
           metricasZui->cpu_usage, metricasZui->mem_used_gb, metricasZui->mem_total_gb, metricasZui->temp_c);
 */
    vol_value = m_shared->volume / 100.0f;

    // printf("Nuevo volumen zuirender = %d\n", m_shared->volume);
    // fflush(stdout);
    //  float sys_vol = GetSystemVolume() / 100.0f; // siempre leer sistema
    //   Dentro de tu zui_render o donde leas el volumen:
    // static uint32_t frame_count = 0;
    // frame_count++;
    printf("antes de renderNukear en zui render %lu\n", (now_ns() - t) / 1000);

    float v = vol_value;
    // --- ZONAS ---
    float logo_h = win_height * 0.15f;
    float footer_h = win_height * 0.150f;
    // printf("CCCOMOOOOwinheight:%f \n", win_height);

    float middle_h = win_height - logo_h - footer_h;
    // vol_value = GetSystemVolume() / 100.0f; // Actualiza el volumen cada frame

    t = now_ns();
    if (nk_begin(ctx, "ZaramagaDock",
                 nk_rect(0, 0, (float)win_width, (float)win_height),
                 NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

        // tam;o nuklear
        /*  printf("width nuklear = %f height nuklear = %f\n",
                nk_window_get_content_region_size(ctx).x,
                nk_window_get_content_region_size(ctx).y); */
        // --- POSICIONES ---

        float y = 0;
        y = logoDraw(canvas, y, win_width, logo_h);
        // printf("logo      %6lu us\n", (now_ns() - t) / 1000);
        printf("logo...... %lu us\n", (now_ns() - t) / 1000);
        // printf("Después de logoDraw, y = %f\n", y);
        t = now_ns();
        y = kernelraw(ctx, y, win_width, middle_h);
        // printf("Después de kernelraw, y = %f\n", y);
        printf("kernel.... %lu us\n", (now_ns() - t) / 1000);
        t = now_ns();
        y = datedraw(ctx, y, win_width);
        // printf("Después de datedraw, y = %f\n", y);
        printf("date...... %lu us\n", (now_ns() - t) / 1000);
        t = now_ns();
        y = voldraw(ctx, y, win_width, middle_h);
        // printf("Después de voldraw, y = %f\n", y);
        //  printf("cpuZui %f\n", m_shared->cpu);
        printf("vol....... %lu us\n", (now_ns() - t) / 1000);
        t = now_ns();
        y = gammaDraw(ctx, y, win_width);
        printf("gamma..... %lu us\n", (now_ns() - t) / 1000);
        t = now_ns();
        y = modeDraw(ctx, y, win_width, bore_cfg);
        /// bore////
        int offsetBore = 160;
        if (bore_cfg->boreDisponible == 1)
        {
            // printf("bore en kernel ok DESDE ZUIRENDER\n");
            y = boreDraw(ctx, y, win_width, bore_cfg);
        }
        else
        {
            offsetBore = 0;
        }

        printf("gamma..... %lu us\n", (now_ns() - t) / 1000);
        t = now_ns();

        int pos = y - 30;
        pos = metricsDraw(ctx, win_height - footer_h - offsetBore, win_width, footer_h);
        y = pos - 20;
        printf("metrics... %lu us\n", (now_ns() - t) / 1000);
        // Actualizamos y con la posición devuelta por metricsDraw
        t = now_ns();
        int temping = ping->last_ping_ms; // aqui necesitamos damage ojo va pa todo
        // printf("pingZui %d\n", temping);

        y = pingDraw(ctx, y, win_width, ping);
        printf("ping...... %lu us\n", (now_ns() - t) / 1000);
        y = upDownDraw(ctx, y, win_width);
        t = now_ns();
        // =========================
        // 🔵 MIDDLE ZONE (debug opcional)
        // =========================
        nk_fill_rect(canvas,
                     nk_rect(0, y, win_width, middle_h),
                     0,
                     // nk_rgb(0, 0, 255));
                     nk_rgba(40, 40, 40, 20)); // 👈 ALPHA
        // nk_button_label(ctx, "\uF028"); // volumen
        float paddingM = 20.0f;
        float row_h = 30.0f;
        float start_y = y + 40;

        // columnas
        float icon_w = 30.0f;
        float label_w = 60.0f;
        float value_w = 40.0f;
        float slider_w = win_width - (paddingM * 2 + icon_w + label_w + value_w + 20);
        // float paddingM = 20.0f;
        float slider_h = 55.0f;

        paddingM = 20.0f;
        row_h = 30.0f;
        // Ajustamos start_y un poco para dejar sitio a la hora
        start_y = y + 40;

        // =========================
        // 🔴 FOOTER ZONE
        // =========================
        int footeroffset = 30;
        miestilo.text_normal = nk_rgb(255, 0, 0); // Rojo para el texto del botón
        nk_fill_rect(canvas,
                     nk_rect(0, win_height, win_width, footer_h),
                     0,
                     // nk_rgb(40, 40, 40));

                     nk_rgba(0, 0, 40, 0)); // 👈 ALPHA

        // =========================
        // 🔴 FOOTER ZONE (UNA FILA - 3 BOTONES)
        // =========================

        // --- CÁLCULO DE MEDIDAS ---
        float padding = 10.0f; // Un pelín menos de padding para que quepan bien
        float btn_h = 25.0f;

        // Dividimos el ancho entre 3, restando los 4 huecos de padding (izq, entre-1, entre-2, der)
        float btn_w_third = (win_width - (padding * 4)) / 3;

        // Centrado vertical en el footer (una sola fila)
        float y_btn = win_height - footer_h + (footer_h - btn_h) / 2;
        // middle_h= win_height - footer_h;
        // printf("Medidas middleh:%f \n", middle_h);
        // printf("Medidas foother:%f \n", footer_h);
        // printf("Medidas middleh:%f \n", middle_h);
        // printf("winheightdESPUESLOGO:%f \n", win_height);
        // aqui offset para que los botones no se solapen con el footer
        // middle_h = middle_h - 80; // ajuste offset metricas + ping
        // int offsetBore=160;
        middle_h = middle_h - 80 - offsetBore;
        // Iniciamos el layout para 3 widgets
        nk_layout_space_begin(ctx, NK_STATIC, footer_h, 3);

        // Definimos el mismo espacio de 3 huecos para ambos estados
        nk_layout_space_begin(ctx, NK_STATIC, btn_h, 3);
        int semaforo = 0;

        if (show_confirm == 0)
        {
            // --- MODO NORMAL (Los 3 iconos) ---

            // REBOOT
            nk_layout_space_push(ctx, nk_rect(padding, middle_h, btn_w_third, btn_h));
            // cambiodecolor(ctx);
            struct nk_rect bounds = nk_widget_bounds(ctx);
            g_hover.reboot = bounds;
            if (nk_button_label(ctx, "\uf01e"))
            {
                show_confirm = 1;
                // Forzamos a Gamescope a recuperar el foco
                nk_input_begin(ctx);
                nk_input_end(ctx);
            }

            // ctx->style.button = estilo_original;
            //  EXIT
            nk_layout_space_push(ctx, nk_rect(padding * 2 + btn_w_third, middle_h, btn_w_third, btn_h));
            bounds = nk_widget_bounds(ctx);
            g_hover.exit = bounds;
            if (nk_button_label(ctx, "\uf08b"))
            {
                // kill(-getpgrp(), SIGTERM);
                // system("swaymsg exit");
                system("swaymsg -q exit >/dev/null 2>&1");
                exit(0);
            }

            // POWER
            nk_layout_space_push(ctx, nk_rect(padding * 3 + btn_w_third * 2, middle_h, btn_w_third, btn_h));
            bounds = nk_widget_bounds(ctx);
            g_hover.power = bounds;
            if (nk_button_label(ctx, "\uf011"))
                show_confirm = 2;
        }
        else
        {
            // --- MODO CONFIRMACIÓN (Ocupamos el mismo espacio pero con 2 botones anchos) ---
            // Calculamos un ancho para que dos botones ocupen lo que antes ocupaban tres
            float btn_w_confirm = (win_width - (padding * 3)) / 2;

            // BOTÓN SÍ (A la izquierda)
            nk_layout_space_push(ctx, nk_rect(padding, middle_h, btn_w_confirm, btn_h));
            const char *msg = (show_confirm == 1) ? "pc-REBOOT\uf00c" : "pc-OFF\uf00c";
            if (nk_button_label(ctx, msg))
            {
                if (show_confirm == 1)
                    // system("loginctl reboot");
                    z_reboot();
                else
                    // system("loginctl poweroff");
                    z_poweroff();
                exit(0);
            }

            // BOTÓN CANCELAR (A la derecha)
            nk_layout_space_push(ctx, nk_rect(padding * 2 + btn_w_confirm, middle_h, btn_w_confirm, btn_h));
            if (nk_button_label(ctx, "Cancel\uf00d"))
            {
                show_confirm = 0;
                // Limpiamos el input para evitar que el clic "atraviese" al modo normal
                nk_input_begin(ctx);
                nk_input_end(ctx);
            }
        }

        nk_layout_space_end(ctx);
    }
    printf("botones... %lu us\n", (now_ns() - t) / 1000);
    nk_end(ctx);
    // printf("todo layout %lu ns\n", now_ns() - t);
}
int logoDraw(struct nk_command_buffer *canvas,
             float y,
             float win_width,
             float logo_h)
{
    printf("logoDraw: dirty=%d y=%.1f h=%.1f\n",
           logo_dirty, y, logo_h);

    return y + logo_h;
}
int datedraw(struct nk_context *ctx, float y, float win_width)
{
    float row_height = 1.0f; // La altura que reservamos para este bloque

    // =========================
    // 🕒 BLOQUE RELOJ
    // =========================
    nk_layout_space_begin(ctx, NK_STATIC, row_height, 1);

    // Empujamos el rect en la posición 'y' actual
    nk_layout_space_push(ctx, nk_rect(5, y, win_width, row_height));

    char icoReloj[60] = " \uf017 ";
    char icoCalendario[30] = "  \uf073 ";

    strcat(icoReloj, time_str);
    strcat(icoReloj, " / ");
    strcat(icoCalendario, date_str);
    strcat(icoReloj, icoCalendario);

    nk_label(ctx, icoReloj, NK_TEXT_LEFT);

    nk_layout_space_end(ctx);

    // DEVOLVEMOS 'y' + la altura de lo que hemos dibujado
    // Así, el siguiente elemento sabrá que debe empezar más abajo.
    return (int)(y + row_height);
}
int voldraw(struct nk_context *ctx, float y, float win_width, float middle_h)
{

    // SLIDER 1
    int offset = 30;          // Un pequeño offset para que el slider no toque los bordes
    float row_height = 20.0f; // La altura que reservamos para este bloque
    float icon_w = 30.0f;
    float label_w = 60.0f;
    float value_w = 40.0f;
    float slider_w = win_width - (1 * 2 + icon_w + label_w + value_w);
    // float paddingM = 20.0f;
    float slider_h = 55.0f;
    nk_layout_space_begin(ctx, NK_STATIC, row_height, 8);
    y += offset; // Un pequeño espacio antes de empezar a dibujar el bloque

    // ICONO
    nk_layout_space_push(ctx,
                         nk_rect(0, y, icon_w, row_height * 2));

    nk_label(ctx, "\uF028", NK_TEXT_CENTERED);

    // LABEL
    nk_layout_space_push(ctx,
                         nk_rect(1 + icon_w, y, label_w, row_height * 2));
    nk_label(ctx, "VOLUME", NK_TEXT_CENTERED);

    // SLIDER
    nk_layout_space_push(ctx, nk_rect(1 + icon_w + label_w, y, slider_w - offset, row_height * 2));
    struct nk_rect boundsvol = nk_widget_bounds(ctx);
    g_hover.volume = boundsvol;
    if (nk_input_is_mouse_hovering_rect(&ctx->input, nk_widget_bounds(ctx)))
    {
        // printf("Mouse is hovering over the ping label\n");
        //  ctx->style.text.color = nk_rgb(255, 0, 0); // Cambiamos el color del texto a amarillo
        ctx->style.window.background = nk_rgba(10, 15, 10, 230); // Cambiamos el color del texto a amarillo

        g_hover.is_hovering_volume = true; // Guardamos las coordenadas del rectángulo del ping en la estructura global

        // ctx->style.text.color = nk_rgb(255, 0, 0); // Cambiamos el color del texto a amarillo
    }
    else
    {
        // printf("Mouse is NOT hovering over the ping label\n");
        // ctx->style.text.color = color_original; // Restauramos el color original
        g_hover.is_hovering_volume = false;
        // nk_label(ctx, ping_str, NK_TEXT_LEFT);
    }
    if (nk_slider_float(ctx, 0.0f, &vol_value, 2.0f, 0.01f))
    {
        zui_set_volume(vol_value);
    }

    // VALOR
    char buffer[16];
    sprintf(buffer, "%d%%", (int)(vol_value * 100));

    nk_layout_space_push(ctx,
                         nk_rect(1 + icon_w + label_w + slider_w - offset, y, value_w, row_height * 2));
    nk_label(ctx, buffer, NK_TEXT_CENTERED);

    return (int)(y + row_height); // Devolvemos la posición final después de dibujar el bloque
}
int kernelraw(struct nk_context *ctx, float y, float win_width, float middle_h)
{
    float row_height = 15.0f; // La altura que reservamos para este bloque

    // =========================
    // 🕒 labelKernel
    // =========================
    nk_layout_space_begin(ctx, NK_STATIC, row_height, 1);

    // Empujamos el rect en la posición 'y' actual
    nk_layout_space_push(ctx, nk_rect(15, y, win_width * 0.75, row_height));

    nk_label(ctx, *mi_buffer, NK_TEXT_CENTERED);

    nk_layout_space_end(ctx);

    // DEVOLVEMOS 'y' + la altura de lo que hemos dibujado
    // Así, el siguiente elemento sabrá que debe empezar más abajo.
    return (int)(y + row_height);
}
int metricsDraw(struct nk_context *ctx, float y, float win_width, float footer_h)
{
    printf("metricasZui=%p\n", (void *)metricasZui);
    printf("ptr=%p cpu=%f ram=%f/%f temp=%d\n",
           (void *)metricasZui,
           metricasZui->cpu_usage,
           metricasZui->mem_used_gb,
           metricasZui->mem_total_gb,
           metricasZui->temp_c);
    if (metricasZui == NULL || metricasZui->temp_c > 150) // Verificamos que metricasZui esté listo y tenga datos válidos
    {
        float row_height = 20.0f; // La altura que reservamos para este bloque

        // =========================
        // 📊 BLOQUE MÉTRICAS
        // =========================
        nk_layout_space_begin(ctx, NK_STATIC, row_height, 1);

        // Empujamos el rect en la posición 'y' actual
        nk_layout_space_push(ctx, nk_rect(0, y - 30, win_width * 0.75, row_height));
        printf("Métricas en zui_render: CPU=%.1f%%, RAM=%.2f/%.2fGB, Temp=%d°C\n",
               metricasZui->cpu_usage, metricasZui->mem_used_gb, metricasZui->mem_total_gb, metricasZui->temp_c);

        /* char icoReloj[60] = " \uf017 ";
        char icoCalendario[30] = "  \uf073 ";

        strcat(icoReloj, time_str);
        strcat(icoReloj, " / ");
        strcat(icoCalendario, date_str);
        strcat(icoReloj, icoCalendario);
         */
        // char metricasall[100];
        // char cpu_str[20] = "", mem_str[30] = "\uefc5 ", temp_str[30] = "\uef2b ";
        //  cpu_str="CPU: %.1f%%";
        char metricasall[128]; // Asegúrate de que sea lo bastante grande

        // Formateamos todo de una sola vez
        snprintf(metricasall, sizeof(metricasall),
                 "\uf4bccpu:%.0f%% \uefc5ram:%.0f \uef2b%d°C",
                 0.0f, // metricasZui->cpu_usage,
                 0.0f, // metricasZui->mem_used_gb / metricasZui->mem_total_gb * 100.0f,
                 0);   // metricasZui->temp_c

        // Ahora Nuklear lo recibirá perfecto
        nk_label(ctx, metricasall, NK_TEXT_CENTERED);
        // nk_label(ctx, metricasall, NK_TEXT_LEFT);

        nk_layout_space_end(ctx);
        return y - 30;
    }

    // printf("Entrando a metricsDraw, footer_h = %f\n", y);
    float row_height = 20.0f; // La altura que reservamos para este bloque

    // =========================
    // 📊 BLOQUE MÉTRICAS
    // =========================
    nk_layout_space_begin(ctx, NK_STATIC, row_height, 1);

    // Empujamos el rect en la posición 'y' actual
    nk_layout_space_push(ctx, nk_rect(25, y - 30, win_width, row_height));
    /*   printf("Métricas en zui_render: CPU=%.1f%%, RAM=%.2f/%.2fGB, Temp=%d°C\n",
             metricasZui->cpu_usage, metricasZui->mem_used_gb, metricasZui->mem_total_gb, metricasZui->temp_c);
  */
    /* char icoReloj[60] = " \uf017 ";
    char icoCalendario[30] = "  \uf073 ";

    strcat(icoReloj, time_str);
    strcat(icoReloj, " / ");
    strcat(icoCalendario, date_str);
    strcat(icoReloj, icoCalendario);
     */
    // char metricasall[100];
    // char cpu_str[20] = "", mem_str[30] = "\uefc5 ", temp_str[30] = "\uef2b ";
    //  cpu_str="CPU: %.1f%%";
    char metricasall[128]; // Asegúrate de que sea lo bastante grande

    // Formateamos todo de una sola vez
    snprintf(metricasall, sizeof(metricasall),
             "\uf4bc cpu:%.0f%%  \uefc5 ram:%.0f%%  \uef2b %d°C",
             metricasZui->cpu_usage,
             metricasZui->mem_used_gb / metricasZui->mem_total_gb * 100.0f,
             metricasZui->temp_c);

    // Ahora Nuklear lo recibirá perfecto
    nk_label(ctx, metricasall, NK_TEXT_LEFT);
    // nk_label(ctx, metricasall, NK_TEXT_LEFT);

    nk_layout_space_end(ctx);
    return y - 30;
}
int pingDraw(struct nk_context *ctx, float y, float win_width, PingWorker *ping)
{
    float row_height = 20.0f; // La altura que reservamos para este bloque

    // =========================
    // 🌐 BLOQUE PING
    // =========================
    nk_layout_space_begin(ctx, NK_STATIC, row_height, 1);

    struct nk_rect ping_rect = nk_rect(25, y - 30, win_width, row_height);

    // g_hover.ping = ping_rect;

    nk_layout_space_push(ctx, ping_rect);

    // Empujamos el rect en la posición 'y' actual
    // nk_layout_space_push(ctx, nk_rect(25, y - 30, win_width / 2, row_height));

    char ping_str[64];
    snprintf(ping_str, sizeof(ping_str), "\uf0ac Ping: %d ms", ping->last_ping_ms);
    // puebas de tooltips and colors
    // ctx->style.text.color = nk_rgb(255, 0, 0); // Verde fosforito para el ping
    //  1. GET: Guardar el color actual en una variable temporal
    struct nk_color color_original = ctx->style.text.color;
    // 1. Guardamos el estilo de fondo actual (es un nk_style_item)
    struct nk_style_item old_bg = ctx->style.window.fixed_background;

    // 2. SET: Aplicar el nuevo color
    ctx->style.text.color = nk_rgb(255, 255, 255);
    // ctx->style.window.tooltip_border_color = nk_rgb(255, 0, 0); // Cambiamos el color del borde del popup
    // ctx->style.window.fixed_background.type = NK_STYLE_ITEM_COLOR;

    // ... aquí dibujas tu tooltip ...
    // nk_tooltip(ctx, "ping google.com");
    struct nk_rect boundsping = nk_widget_bounds(ctx);
    g_hover.ping = boundsping;
    if (nk_input_mouse_clicked(&ctx->input, NK_BUTTON_LEFT, boundsping))
    {
        // ping->running = !ping->running;
        ping_start(ping);
        // ping->running = !ping->running; // Toggle the running state
        // pingDraw(ctx, y, win_width, ping);
        /* for (int i = 0; i < 115; i++)
        y = pingDraw(ctx, y, win_width, ping);
            printf("¡Has hecho clic en el label del ping! Iteración %d\n", i + 1);
            //usleep(100000); // Espera de 100 ms entre iteraciones
         */
        // printf("¡Has hecho clic en el label del ping!\n");
    }
    // 3. RESTORE: Volver al color original usando la variable que guardaste
    if (nk_input_is_mouse_hovering_rect(&ctx->input, nk_widget_bounds(ctx)))
    {
        // printf("Mouse is hovering over the ping label\n");
        //  ctx->style.text.color = nk_rgb(255, 0, 0); // Cambiamos el color del texto a amarillo
        ctx->style.window.background = nk_rgba(10, 15, 10, 230); // Cambiamos el color del texto a amarillo
        nk_tooltip(ctx, "ping google.com");
        g_hover.is_hovering_ping = true; // Guardamos las coordenadas del rectángulo del ping en la estructura global

        ctx->style.text.color = nk_rgb(255, 0, 0); // Cambiamos el color del texto a amarillo
        nk_label(ctx, ping_str, NK_TEXT_LEFT);
    }
    else
    {
        // printf("Mouse is NOT hovering over the ping label\n");
        ctx->style.text.color = color_original; // Restauramos el color original
        g_hover.is_hovering_ping = false;
        nk_label(ctx, ping_str, NK_TEXT_LEFT);
    }

    ctx->style.text.color = color_original;
    ctx->style.window.fixed_background = old_bg;

    // nk_label(ctx, ping_str, NK_TEXT_LEFT);

    nk_layout_space_end(ctx);
    return y - 30;
}
int upDownDraw(struct nk_context *ctx, float y, float win_width)
{
    float row_height = 20.0f; // La altura que reservamos para este bloque
    // printf("Métricas en zui_render RED download: %.2f MB/s y UPLOAD: %.2f MB/s\n", metricasZui->net_download_mb, metricasZui->net_upload_mb);

    // =========================
    // 🌐 BLOQUE updownDraw
    // =========================
    nk_layout_space_begin(ctx, NK_STATIC, row_height, 1);

    struct nk_rect updown_rect = nk_rect(25, y - 50, win_width, row_height);

    // g_hover.ping = ping_rect;

    nk_layout_space_push(ctx, updown_rect);

    // Empujamos el rect en la posición 'y' actual
    // nk_layout_space_push(ctx, nk_rect(25, y - 30, win_width / 2, row_height));

    char updown_str[64]; // f06f4
    snprintf(updown_str,
             sizeof(updown_str),
             "\uf019 %.2f MB/s  \uf093 %.2f MB/s",
             metricasZui->net_download_mb,
             metricasZui->net_upload_mb);
    // puebas de tooltips and colors
    // ctx->style.text.color = nk_rgb(255, 0, 0); // Verde fosforito para el ping
    //  1. GET: Guardar el color actual en una variable temporal
    // struct nk_color color_original = ctx->style.text.color;
    // 1. Guardamos el estilo de fondo actual (es un nk_style_item)
    // struct nk_style_item old_bg = ctx->style.window.fixed_background;

    // 2. SET: Aplicar el nuevo color
    // ctx->style.text.color = nk_rgb(255, 255, 255);
    // ctx->style.window.tooltip_border_color = nk_rgb(255, 0, 0); // Cambiamos el color del borde del popup
    // ctx->style.window.fixed_background.type = NK_STYLE_ITEM_COLOR;

    // ... aquí dibujas tu tooltip ...
    // nk_tooltip(ctx, "ping google.com");
    // struct nk_rect boundsupdown = nk_widget_bounds(ctx);
    // g_hover.updown = boundsupdown;
    // if (nk_input_mouse_clicked(&ctx->input, NK_BUTTON_LEFT, boundsupdown))
    //{
    // ping->running = !ping->running;
    // ping_start(ping);
    // ping->running = !ping->running; // Toggle the running state
    // zui_render pingDraw(ctx, y, win_width, ping);
    /* for (int i = 0; i < 115; i++)
    y = pingDraw(ctx, y, win_width, ping);
        printf("¡Has hecho clic en el label del ping! Iteración %d\n", i + 1);
        //usleep(100000); // Espera de 100 ms entre iteraciones
     */
    // printf("¡Has hecho clic en el label del ping!\n");
    //}
    // 3. RESTORE: Volver al color original usando la variable que guardaste

    // printf("Mouse is hovering over the ping label\n");
    //  ctx->style.text.color = nk_rgb(255, 0, 0); // Cambiamos el color del texto a amarillo
    // ctx->style.window.background = nk_rgba(10, 15, 10, 230); // Cambiamos el color del texto a amarillo
    // nk_tooltip(ctx, "ping google.com");
    // g_hover.is_hovering_updown = true; // Guardamos las coordenadas del rectángulo del ping en la estructura global

    // ctx->style.text.color = nk_rgb(255, 0, 0); // Cambiamos el color del texto a amarillo
    // nk_label(ctx, updown_str, NK_TEXT_LEFT);

    // printf("Mouse is NOT hovering over the ping label\n");
    // ctx->style.text.color = color_original; // Restauramos el color original
    // g_hover.is_hovering_updown = false;
    nk_label(ctx, updown_str, NK_TEXT_LEFT);

    // ctx->style.text.color = color_original;
    // ctx->style.window.fixed_background = old_bg;

    // nk_label(ctx, ping_str, NK_TEXT_LEFT);

    nk_layout_space_end(ctx);
    return y - 30;
}
int boreDraw(struct nk_context *ctx, float y, float win_width, BoreConfig *bore)
{
    /*
     * =========================================
     * KERNEL -> UI
     * =========================================
     */
    if (bore->bore == 0)
    {
        printf("BOREEEEE IGUAL A CEROOOOOOOOO\n");
        bore_set(
            "sched_burst_inherit_type",
            BORE_DEFAULT_INHERIT,
            bore);

        bore_set(
            "sched_burst_smoothness",
            BORE_DEFAULT_SMOOTHNESS,
            bore);

        bore_set(
            "sched_burst_penalty_offset",
            BORE_DEFAULT_PENALTY,
            bore);

        bore_set(
            "sched_burst_penalty_scale",
            BORE_DEFAULT_SCALE,
            bore);

        bore_set(
            "sched_burst_cache_lifetime",
            BORE_DEFAULT_CACHE_NS,
            bore);

        bore_set(
            "sched_burst_protect_slice_lv",
            BORE_DEFAULT_PROTECT,
            bore);
        /* code */
    }

    int bore_enabled = bore->bore;
    int inherit = bore->inherit_type;
    int smooth = bore->smoothness;
    int penalty = bore->penalty_offset;
    int penaltyS = bore->penalty_scale;
    int protect_slice = bore->protect_slice_lv;

    /*
     * Kernel:
     *
     *     nanosegundos
     *
     * UI:
     *
     *     milisegundos
     */

    uint32_t cache_ms =
        (uint32_t)(bore->cache_lifetime / 1000000ULL);

    printf(
        "BORE al inicio: on=%d | inherit=%d | smooth=%d\n"
        "penalty=%d | penaltyScale=%d | cache-ms=%u | protect=%d\n",
        bore_enabled,
        inherit,
        smooth,
        penalty,
        penaltyS,
        cache_ms,
        protect_slice);

    uint64_t t = now_ns();

    float row_h = 10.0f;

    /* =========================================
       sched_bore
       0 - 1
       ========================================= */

    /*  if (bore_slider_factory(
             ctx,
             y,
             win_width,
             "BORE",
             SLIDER_INT,
             &bore_enabled,
             0,
             1,
             1,
             &g_hover.bright))
     {
         bore_set(
             "sched_bore",
             (unsigned long long)bore_enabled,
             bore);
         printf("bore setter");
     } */

    y += row_h;

    /* =========================================
       sched_burst_inherit_type
       0 - 2
       ========================================= */

    if (bore_slider_factory(
            ctx,
            y,
            win_width,
            "Inh",
            SLIDER_INT,
            &inherit,
            0,
            2,
            1,
            &g_hover.bright))
    {
        bore_set(
            "sched_burst_inherit_type",
            (unsigned long long)inherit,
            bore);
    }

    y += row_h;

    /* =========================================
       sched_burst_smoothness
       0 - 3
       ========================================= */

    if (bore_slider_factory(
            ctx,
            y,
            win_width,
            "smooth",
            SLIDER_INT,
            &smooth,
            0,
            3,
            1,
            &g_hover.bright))
    {
        bore_set(
            "sched_burst_smoothness",
            (unsigned long long)smooth,
            bore);
    }

    y += row_h;

    /* =========================================
       sched_burst_penalty_offset
       0 - 63
       ========================================= */

    if (bore_slider_factory(
            ctx,
            y,
            win_width,
            "penOff",
            SLIDER_INT,
            &penalty,
            0,
            63,
            1,
            &g_hover.bright))
    {
        bore_set(
            "sched_burst_penalty_offset",
            (unsigned long long)penalty,
            bore);
    }

    y += row_h;

    /* =========================================
       sched_burst_penalty_scale
       0 - 4095
       ========================================= */

    if (bore_slider_factory(
            ctx,
            y,
            win_width,
            "penScl",
            SLIDER_INT,
            &penaltyS,
            0,
            4095,
            1,
            &g_hover.bright))
    {
        bore_set(
            "sched_burst_penalty_scale",
            (unsigned long long)penaltyS,
            bore);
    }

    y += row_h;

    /* =========================================
       sched_burst_cache_lifetime

       Kernel:
           0 - 4294967295 ns

       UI:
           0 - 4294 ms

       Paso:
           100 ms
       ========================================= */

    if (bore_slider_factory(
            ctx,
            y,
            win_width,
            "cache",
            SLIDER_UINT,
            &cache_ms,
            0,
            4294,
            1,
            &g_hover.bright))
    {
        unsigned long long cache_ns =
            (unsigned long long)cache_ms * 1000000ULL;

        bore_set(
            "sched_burst_cache_lifetime",
            cache_ns,
            bore);
    }

    y += row_h;

    /* =========================================
       sched_burst_protect_slice_lv
       0 - 1
       ========================================= */

    if (bore_slider_factory(
            ctx,
            y,
            win_width,
            "protect",
            SLIDER_INT,
            &protect_slice,
            0,
            1,
            1,
            &g_hover.bright))
    {
        bore_set(
            "sched_burst_protect_slice_lv",
            (unsigned long long)protect_slice,
            bore);
    }

    y += row_h;

    /* =========================================
       DEBUG
       ========================================= */

    printf(
        "BORE UI: on=%d | inherit=%d | smooth=%d | "
        "penalty=%d | scale=%d | cache=%u ms | protect=%d\n",
        bore_enabled,
        inherit,
        smooth,
        penalty,
        penaltyS,
        cache_ms,
        protect_slice);

    printf(
        "BoreDraw %lu us\n",
        (now_ns() - t) / 1000);

    return (int)y - 10;
}
int modeDraw(struct nk_context *ctx, float y, float win_width, BoreConfig *bore)
{
    int offset = 0;

    float row_height = 30.0f;

    float icon_w = 30.0f;
    float label_w = 60.0f;
    float value_w = 40.0f;

    float slider_w =
        win_width - (1 * 2 + icon_w + label_w + value_w);

    float slider_h = 55.0f;

    y += offset;

    float padding = 5.0f;
    float gap = 10.0f;

    float total_w = win_width - padding * 2.0f;

    float btn_w =
        (total_w - gap * 4.0f) / 4.0f;

    /*
     * =========================================
     * MODO ACTUAL
     *
     * 0 = GAME
     * 1 = BAL
     * 2 = ECO
     * =========================================
     */

    // static int cpu_mode = 1;

    /*
     * =========================================
     * ESTILOS
     * =========================================
     */

    struct nk_style_button estilo_original;
    struct nk_style_button estilo_activo;

    estilo_original = ctx->style.button;

    estilo_activo = estilo_original;

    estilo_activo.normal =
        nk_style_item_color(nk_rgb(70, 70, 70));

    estilo_activo.hover =
        nk_style_item_color(nk_rgb(90, 90, 90));

    estilo_activo.active =
        nk_style_item_color(nk_rgb(110, 110, 110));

    estilo_activo.text_normal =
        nk_rgb(255, 255, 255);

    estilo_activo.text_hover =
        nk_rgb(255, 255, 255);

    estilo_activo.text_active =
        nk_rgb(255, 255, 255);

    /*
     * =========================================
     * LAYOUT
     * =========================================
     */

    nk_layout_space_begin(
        ctx,
        NK_STATIC,
        row_height,
        4);

    /*
     * =========================================
     * GAME
     * =========================================
     */

    nk_layout_space_push(
        ctx,
        nk_rect(
            padding,
            y,
            btn_w - 20,
            row_height));

    ctx->style.button =
        (cpu_mode == 0)
            ? estilo_activo
            : estilo_original;

    if (nk_button_label(ctx, "\uf11b"))
    {
        printf("=== GAME MODE ===\n");

        int ret = system(
            "sudo -n /usr/local/libexec/zmenu-bore governor performance");

        printf(
            "GAME governor ret = %d\n",
            ret);

        printf("=== GAME MODE END ===\n");

        cpu_mode = 0;
    }

    /*
     * =========================================
     * BALANCED
     * =========================================
     */

    nk_layout_space_push(
        ctx,
        nk_rect(
            padding + (btn_w - 20 + gap) * 1,
            y,
            btn_w - 10,
            row_height));

    ctx->style.button =
        (cpu_mode == 1)
            ? estilo_activo
            : estilo_original;

    if (nk_button_label(ctx, "BAL"))
    {
        printf("=== BALANCED MODE ===\n");

        int ret = system(
            "sudo -n /usr/local/libexec/zmenu-bore governor powersave");

        printf(
            "BAL governor ret = %d\n",
            ret);

        printf("=== BALANCED MODE END ===\n");

        cpu_mode = 1;
    }

    /*
     * =========================================
     * ECO
     * =========================================
     */

    nk_layout_space_push(
        ctx,
        nk_rect(
            padding + (btn_w - 15 + gap) * 2,
            y,
            btn_w,
            row_height));

    ctx->style.button =
        (cpu_mode == 2)
            ? estilo_activo
            : estilo_original;

    if (nk_button_label(ctx, "ECO"))
    {
        printf("=== ECO MODE ===\n");

        int ret = system(
            "sudo -n /usr/local/libexec/zmenu-bore governor powersave");

        printf(
            "ECO governor ret = %d\n",
            ret);

        printf("=== ECO MODE END ===\n");

        cpu_mode = 2;
    }

    /*
     * =========================================
     * BOREBOTON
     * =========================================
     */
    printf("bore activo...%d y disponible... %d \n", bore->bore, bore->boreDisponible);
    nk_layout_space_push(
        ctx,
        nk_rect(
            padding + (btn_w - 10 + gap) * 3,
            y,
            btn_w + 30,
            row_height));

    /*
     * BORE siempre conserva el estilo normal
     * por ahora.
     */

    struct nk_style_button bore_test = ctx->style.button;

    bore_test.normal =
        nk_style_item_color(nk_rgb(0, 155, 0));

    bore_test.hover =
        nk_style_item_color(nk_rgb(0, 255, 0));

    bore_test.active =
        nk_style_item_color(nk_rgb(255, 150, 150));

    bore_test.text_normal =
        nk_rgb(255, 255, 255);

    bore_test.text_hover =
        nk_rgb(155, 0, 0);

    bore_test.text_active =
        nk_rgb(255, 255 , 255);

    ctx->style.button = bore_test;

     if (nk_button_label(ctx, "BORE"))
    {
        printf("bore pulsaoooooooooo\n");
    }
    /*
     * =========================================
     * FIN LAYOUT
     * =========================================
     */

    nk_layout_space_end(ctx);

    /*
     * Restauramos SIEMPRE el estilo original
     * para los widgets que vienen después.
     */

    ctx->style.button = estilo_original;

    /*
     * =========================================
     * RETURN
     * =========================================
     */

    y += row_height;

    return (int)y;
}
static void sync_cpu_mode_from_governor(void)
{
    FILE *f = fopen(
        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor",
        "r");

    if (!f)
    {
        printf("GOVERNOR: no se pudo leer\n");
        return;
    }

    char governor[32] = {0};

    if (fgets(governor, sizeof(governor), f))
    {
        governor[strcspn(governor, "\n")] = '\0';

        printf(
            "GOVERNOR ACTUAL: [%s]\n",
            governor);

        if (strcmp(governor, "performance") == 0)
        {
            cpu_mode = 0; // GAME
        }
        else if (strcmp(governor, "powersave") == 0)
        {
            /*
             * No cambiamos cpu_mode aquí.
             *
             * BAL y ECO usan ambos powersave.
             * Conservamos el último estado lógico.
             */
        }
    }

    fclose(f);

    printf(
        "CPU MODE SINCRONIZADO: %d\n",
        cpu_mode);
}
#endif