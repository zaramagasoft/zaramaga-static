#define _GNU_SOURCE
#include "wlr-layer-shell-unstable-v1.h"
#include <cairo.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h> // <-- Añade este include
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#include "nuklear.h"
#include <linux/prctl.h>
#include <signal.h>

// --- GLOBALES ---
struct wl_display *display;
struct wl_compositor *compositor;
struct wl_shm *shm;
struct zwlr_layer_shell_v1 *layer_shell;
static struct wl_region *empty_region = NULL;

struct wl_seat *seat;
struct wl_buffer *buffer;
struct nk_context ctx;
uint32_t *shm_data_global;
int win_width = 400;
int win_height = 80;
int cur_x = 0, cur_y = 0;

// Al inicio de tu archivo
cairo_surface_t *c_surf_global = NULL;
cairo_t *cr_global = NULL;

// Globales para el control del mando
int inotify_fd = -1;
char gamepad_action[32] = "CONECTADO";

struct shared_metrics {
  float cpu;
  float mem_u;
  float mem_t;
  int temp;
  int volume;
};
struct shared_metrics *m_shared;
// --- GLOBALES ADICIONALES ---
struct wl_surface *global_surf = NULL; // La hacemos global para usarla en main
volatile sig_atomic_t volume_changed = 0;
int global_configured = 0;
int is_visible = 0;

// --- PROTOTIPOS ---
static void cairo_rounded_rectangle(cairo_t *cr, double x, double y, double w,
                                    double h, double r);
static bool IsMuted(void);
void limpiaexit();
void (*current_ui)(void);
int GetSystemVolume();
void start_zui_monitor();
static void render_frame(struct wl_surface *surface, void (*ui_func)(void));
void toastVolumen();
void toastCero();
void toastGamepad();
// definiciones
void clear_screen(struct nk_context *ctx, struct nk_color color) {
  // Dibujar un rectángulo que cubra toda la pantalla
  struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

  nk_fill_rect(canvas, nk_rect(0, 0, win_width, win_height),
               20, // radio de esquinas redondeadas (0 = recto)
               color);
}
void handle_vol_signal(int sig) { volume_changed = 1; }

int GetSystemVolume() {
  int volume = 0;
  // if (IsMuted())
  // m_shared->volume=-1;
  // Este comando de pactl es estándar y muy rápido
  FILE *fp = popen(
      "pactl get-sink-volume @DEFAULT_SINK@ | grep -Po '\\d+(?=%)' | head -n 1",
      "r");

  if (fp != NULL) {
    char buffer[16];
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
      volume = atoi(buffer);
    }
    pclose(fp);
  }
  return volume;
}
void start_zui_monitor() {
  // printf("empezamos \n");
  pid_t pid_audio = fork();

  if (pid_audio < 0)
    return;

  if (pid_audio == 0) {
    // HIJO: Configurar muerte por herencia
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    // Ignorar la señal que él mismo provoca en el padre para evitar bucles
    signal(SIGUSR1, SIG_IGN);
    // Cambia el antiguo signal() por esto:

    // Usamos stdbuf para que pactl no guarde datos en el buffer y el aviso sea
    // instantáneo
    FILE *fp = popen("stdbuf -oL pactl subscribe", "r");
    if (!fp)
      exit(1);

    char linea[1024];
    // Este bucle no consume CPU, está bloqueado esperando texto
    while (fgets(linea, sizeof(linea), fp) != NULL) {
      if (strstr(linea, "sink") && strstr(linea, "change")) {
        //printf("Evento de volumen\n");
        fflush(stdout);
        if (m_shared->volume==GetSystemVolume() && !IsMuted())
        {
          printf("no deberia cambiar ni aparecer toast\n");
        }else
        {
          /* code */
          m_shared->volume = GetSystemVolume();

          // El "Codazo" al padre
          printf("Nuevo volumen = %d\n", m_shared->volume);
          fflush(stdout);
          kill(getppid(), SIGUSR1);

          // Pequeña pausa para no ametrallar al padre si mueves el slider
          // rápido
          usleep(200000);
        }
        

        
      }
    }
    pclose(fp);
    exit(0);
  }
}
void init_cairo_static() {
  c_surf_global = cairo_image_surface_create_for_data(
      (unsigned char *)shm_data_global, CAIRO_FORMAT_ARGB32, win_width,
      win_height, win_width * 4);
  cr_global = cairo_create(c_surf_global);

  // Dibuja aquí TODO lo estático (fuentes, logos, colores fijos)
  cairo_select_font_face(cr_global, "3270 Nerd Font Propo",
                         CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr_global, 28);
  //cairo_set_source_rgb(cr_global, 0.20, 1.00, 0.20);
  cairo_set_source_rgb(cr_global, 51.0 / 255.0, 255.0 / 255.0, 51.0 / 255.0);
  cairo_move_to(cr_global, 30, 60);
  // cairo_rounded_rectangle(cr_global, 10, 10, 380, 100, 16);

  // cairo_fill(cr_global);
  //  cairo_show_text(cr_global, "ZARAMAGA OS");
}
// --- FUNCIONES DE APOYO ---
static void noop() {}

static float text_get_width(nk_handle handle, float height, const char *text,
                            int len) {
  if (cr_global && text && len > 0) {
    // Nuklear pasa un puntero al texto y la longitud en bytes (no viene
    // terminado en \0) Creamos una copia temporal para podérsela pasar a Cairo
    // de forma segura
    char *tmp = malloc(len + 1);
    if (tmp) {
      memcpy(tmp, text, len);
      tmp[len] = '\0';

      cairo_text_extents_t extents;
      cairo_text_extents(cr_global, tmp, &extents);
      free(tmp);

      return extents.x_advance; // Devuelve el ancho real exacto en píxeles
    }
  }
  // Fallback por si acaso
  return len * (height * 0.5f);
}

// --- CALLBACKS DEL RATÓN ---
// static void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t
// serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
//{
// nk_input_begin(&ctx);
//}

static void pointer_leave(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface) {
  // nk_input_end(&ctx);
}

static void pointer_motion(void *data, struct wl_pointer *pointer,
                           uint32_t time, wl_fixed_t x, wl_fixed_t y) {
  /*  cur_x = wl_fixed_to_int(x);
   cur_y = wl_fixed_to_int(y);
   nk_input_motion(&ctx, cur_x, cur_y);
   if (data)
       render_frame((struct wl_surface *)data, current_ui); */
}

// Y ahora en pointer_button usa cur_x y cur_y
static void pointer_button(void *data, struct wl_pointer *pointer,
                           uint32_t serial, uint32_t time, uint32_t button,
                           uint32_t state) {
  /*  if (button == 272)
   {
       // PASAMOS LAS COORDENADAS REALES
       nk_input_button(&ctx, NK_BUTTON_LEFT, cur_x, cur_y, state ==
   WL_POINTER_BUTTON_STATE_PRESSED);
   }
   if (data)
       render_frame((struct wl_surface *)data, current_ui); */
}

static const struct wl_pointer_listener pointer_listener = {
    //.enter = pointer_enter, .leave = pointer_leave, .motion = pointer_motion,
    //.button = pointer_button, .axis = (void *)noop, .frame = (void *)noop,
    //.axis_source = (void *)noop, .axis_stop = (void *)noop, .axis_discrete =
    //(void *)noop
};

// --- DIBUJO ---
void draw_nuklear_to_cairo(struct nk_context *ctx, cairo_t *cr) {
  const struct nk_command *cmd;
  // Fondo del buffer SHM totalmente transparente al inicio
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, 0, 0, 0, 0); // 0 en el canal Alpha
  cairo_paint(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  // cairo_show_text(cr, "Hola Cairo");
  nk_foreach(cmd, ctx) {
    //printf("CMD = %d\n", cmd->type);
    switch (cmd->type) {
    case NK_COMMAND_RECT_FILLED: {
      //printf("RECT_FILLED\n");
    
      //printf("rounding = %d\n", r->rounding);
      const struct nk_command_rect_filled *r =
          (const struct nk_command_rect_filled *)cmd;

      if (r->rounding == 0 && r->x == 0 && r->y == 0 && r->w == win_width &&
          r->h == win_height) {
        break;
      }

      //printf("RECT x=%d y=%d w=%d h=%d round=%d\n", r->x, r->y, r->w, r->h,
      //       r->rounding);
      /* printf("RECT x=%d y=%d w=%d h=%d round=%d color=(%d,%d,%d,%d)\n", r->x,
             r->y, r->w, r->h, r->rounding, r->color.r, r->color.g, r->color.b,
             r->color.a); */
      cairo_set_source_rgba(cr, r->color.r / 255.0, r->color.g / 255.0,
                            r->color.b / 255.0, r->color.a / 255.0);
      cairo_rounded_rectangle(cr, r->x, r->y, r->w, r->h, r->rounding);

      cairo_fill(cr);
    } break;
    case NK_COMMAND_TEXT: {
      //printf("comand_text\n");
      const struct nk_command_text *t = (const struct nk_command_text *)cmd;
      //printf("r %d g %d b %d \n", t->foreground.r, t->foreground.g,
      //       t->foreground.b);
      /* cairo_set_source_rgba(cr, t->foreground.r / 255.0,
                            t->foreground.g / 255.0, t->foreground.b / 255.0,
                            t->foreground.a / 255.0); */
      cairo_set_source_rgba(cr, 51 / 255.0,
                            255 / 255.0, 51 / 255.0,
                            255 / 255.0);
      cairo_move_to(cr, t->x, t->y + t->height - 5);
      cairo_show_text(cr, (const char *)t->string);
    } break;
    case NK_COMMAND_SCISSOR:
      //printf("SCISSOR\n");
      break;

    case NK_COMMAND_LINE:
      //printf("LINE\n");
      break;

    case NK_COMMAND_CURVE:
      //printf("CURVE\n");
      break;

    case NK_COMMAND_RECT:
      //printf("RECT\n");
      break;

    case NK_COMMAND_CIRCLE:
      //printf("CIRCLE\n");
      break;

    case NK_COMMAND_CIRCLE_FILLED:
      //printf("CIRCLE_FILLED\n");
      break;

    case NK_COMMAND_TRIANGLE:
      //printf("TRIANGLE\n");
      break;

    case NK_COMMAND_TRIANGLE_FILLED:
      //printf("TRIANGLE_FILLED\n");
      break;

    case NK_COMMAND_POLYGON:
      //printf("POLYGON\n");
      break;

    case NK_COMMAND_POLYGON_FILLED:
      //printf("POLYGON_FILLED\n");
      break;

    case NK_COMMAND_POLYLINE:
      //printf("POLYLINE\n");
      break;

    case NK_COMMAND_IMAGE:
      //printf("IMAGE\n");
      break;

    default:
      //printf("CMD %d\n", cmd->type);
      break;
    }

    // nk_clear(&ctx);
  }
}
// void toastVolumen();
void cairoSetup();
void toastBrillo();
static void render_frame(struct wl_surface *surface, void (*ui_func)(void)) {
  if (ui_func) {
    ui_func();
  }
  // NO llames a cairoSetup() aquí.
  // toastVolumen();
  // 1. Limpia solo la parte de la UI (o repinta sobre el fondo que ya existe en
  // el buffer) Nuklear ya sabe qué áreas debe actualizar.
  draw_nuklear_to_cairo(&ctx, cr_global);

  // 2. Commit a Wayland
  wl_surface_attach(surface, buffer, 0, 0);
  wl_surface_damage(surface, 0, 0, win_width, win_height);
  wl_surface_commit(surface);
  // wl_surface_set_input_region(global_surf, NULL);
}

// --- WAYLAND SETUP ---
static void layer_surface_configure(void *data,
                                    struct zwlr_layer_surface_v1 *ls,
                                    uint32_t serial, uint32_t width,
                                    uint32_t height) {
  zwlr_layer_surface_v1_ack_configure(ls, serial);
  global_configured = 1;

  // Si por un milagro mutaste el volumen antes de que Wayland responda,
  // pintamos inmediatamente. Si no, se queda invisible.
  if (is_visible) {
    render_frame((struct wl_surface *)data, current_ui);
  }
}
static const struct zwlr_layer_surface_v1_listener ls_listener = {
    .configure = layer_surface_configure, .closed = (void *)exit};

static void global_registry_handler(void *data, struct wl_registry *reg,
                                    uint32_t id, const char *interface,
                                    uint32_t version) {
  if (!strcmp(interface, wl_compositor_interface.name))
    compositor = wl_registry_bind(reg, id, &wl_compositor_interface, 1);
  else if (!strcmp(interface, wl_shm_interface.name))
    shm = wl_registry_bind(reg, id, &wl_shm_interface, 1);
  else if (!strcmp(interface, zwlr_layer_shell_v1_interface.name))
    layer_shell = wl_registry_bind(reg, id, &zwlr_layer_shell_v1_interface, 1);
  else if (!strcmp(interface, wl_seat_interface.name))
    seat = wl_registry_bind(reg, id, &wl_seat_interface, 1);
}
static const struct wl_registry_listener reg_listener = {
    global_registry_handler, NULL};
int initZtoast(void);
int main() {
  atexit(limpiaexit);
  m_shared = mmap(NULL, sizeof(struct shared_metrics), PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (m_shared == MAP_FAILED) {
    perror("mmap falló");
    exit(1);
  }
  m_shared->volume = GetSystemVolume();

  struct sigaction sa;
  sa.sa_handler = handle_vol_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGUSR1, &sa, NULL) == -1) {
    perror("Error al registrar sigaction");
    exit(1);
  }

  start_zui_monitor();
  current_ui = toastVolumen;

  initZtoast();

  // Escuchamos 2 descriptores: [0] Wayland, [1] Inotify (Mandos)
  struct pollfd fds[2];
  fds[0].fd = wl_display_get_fd(display);
  fds[0].events = POLLIN;
  fds[1].fd = inotify_fd;
  fds[1].events = POLLIN;

  int timeout_ms = 0;
  struct timespec last_time;
  clock_gettime(CLOCK_MONOTONIC, &last_time);

  while (1) {
    if (wl_display_get_error(display)) {
      fprintf(stderr, "Error fatal en la conexión de Wayland. Saliendo.\n");
      break;
    }

    // Si el tiempo expira, limpiamos la pantalla volviendo a la UI vacía
    if (is_visible && timeout_ms <= 0) {
      is_visible = 0;
      if (global_configured) {
        render_frame(global_surf, toastCero);
      }
    }

    while (wl_display_prepare_read(display) != 0) {
      wl_display_dispatch_pending(display);
    }
    wl_display_flush(display);

    int poll_timeout = is_visible ? timeout_ms : -1;
    int ret = poll(fds, 2, poll_timeout);
    int saved_errno = errno;

    // --- CORRECCIÓN 1: MATEMÁTICA DEL TIEMPO RESTAURADA ---
    if (is_visible) {
      struct timespec current_time;
      clock_gettime(CLOCK_MONOTONIC, &current_time);
      long elapsed = (current_time.tv_sec - last_time.tv_sec) * 1000 +
                     (current_time.tv_nsec - last_time.tv_nsec) / 1000000;
      timeout_ms -= elapsed;
      last_time = current_time;
    } else {
      clock_gettime(CLOCK_MONOTONIC, &last_time);
    }

    // --- MANEJO DE SITUACIONES DE POLL ---
    if (ret > 0) {
      // Caso A: Wayland tiene eventos listos
      if (fds[0].revents & POLLIN) {
        wl_display_read_events(display);
        wl_display_dispatch_pending(display);
      } else {
        // Obligatorio para no romper la máquina de estados de Wayland si
        // despierta inotify
        wl_display_cancel_read(display);
      }

      // Caso B: Inotify detecta un mando
      if (fds[1].revents & POLLIN) {
        char buf[4096]
            __attribute__((aligned(__alignof__(struct inotify_event))));
        const struct inotify_event *event;
        ssize_t len;

        while ((len = read(inotify_fd, buf, sizeof(buf))) > 0) {
          for (char *ptr = buf; ptr < buf + len;
               ptr += sizeof(struct inotify_event) + event->len) {
            event = (const struct inotify_event *)ptr;

            if (event->len > 0 && strncmp(event->name, "js", 2) == 0) {
              if (event->mask & IN_CREATE) {
                strcpy(gamepad_action, "CONECTADO");
              } else if (event->mask & IN_DELETE) {
                strcpy(gamepad_action, "DESCONECTADO");
              }

              current_ui = toastGamepad; // Cambiamos a UI de mando
              timeout_ms = 4000;         // 4 segundos en pantalla
              is_visible = 1;
              clock_gettime(CLOCK_MONOTONIC, &last_time);

              if (global_configured) {
                render_frame(global_surf, current_ui);
              }
            }
          }
        }
      }
    }
    // --- CORRECCIÓN 2: RETORNO DE TIMEOUT NATURAL ---
    else if (ret == 0) {
      wl_display_cancel_read(display);
      timeout_ms = 0; // Provoca que en la siguiente vuelta entre al toastCero
    }
    // --- CORRECCIÓN 3: MANEJO DE SEÑALES (VOLUMEN) ---
    else {
      wl_display_cancel_read(display);

      if (saved_errno == EINTR) {
        if (volume_changed) {
          volume_changed = 0;
          timeout_ms = 3000; // 5 segundos en pantalla para el volumen
          is_visible = 1;
          current_ui = toastVolumen; // Forzamos que pinte el volumen

          clock_gettime(CLOCK_MONOTONIC, &last_time);

          if (global_configured) {
            render_frame(global_surf, current_ui);
          }
        }
        continue;
      } else {
        perror("Error real en poll");
        break;
      }
    }
  }

  wl_display_disconnect(display);
  return 0;
}
int initZtoast(void) {
  display = wl_display_connect(NULL);
  struct wl_registry *reg = wl_display_get_registry(display);
  wl_registry_add_listener(reg, &reg_listener, NULL);
  wl_display_roundtrip(display);

  nk_init_default(&ctx, 0);
  struct nk_color background_neon =
      nk_rgba(20, 10, 25, 180); // Fondo oscuro semi-transparente
  struct nk_color neon_magenta = nk_rgb(255, 0, 255);
  struct nk_color neon_cyan = nk_rgb(0, 255, 255);

  // Aplicar colores al estilo
  ctx.style.window.fixed_background = nk_style_item_color(background_neon);
  ctx.style.window.header.normal =
      nk_style_item_color(nk_rgba(40, 20, 50, 200));
  ctx.style.window.header.label_normal = neon_cyan;

  // Botones con borde neón
  ctx.style.button.normal = nk_style_item_color(nk_rgba(60, 0, 60, 255));
  ctx.style.button.hover = nk_style_item_color(nk_rgba(100, 0, 100, 255));
  ctx.style.button.active = nk_style_item_color(neon_magenta);
  ctx.style.button.border_color = neon_cyan;
  ctx.style.button.border = 2.0f;
  ctx.style.button.text_normal = neon_cyan;
  ctx.style.button.text_hover = nk_rgb(255, 255, 255);
  static struct nk_user_font font;
  font.height = 28;
  font.width = text_get_width;
  nk_style_set_font(&ctx, &font);

  int size = win_width * win_height * 4;
  int fd = memfd_create("shm", MFD_CLOEXEC);
  ftruncate(fd, size);
  shm_data_global = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
  buffer = wl_shm_pool_create_buffer(pool, 0, win_width, win_height,
                                     win_width * 4, WL_SHM_FORMAT_ARGB8888);
  close(fd);

  global_surf = wl_compositor_create_surface(compositor);
  empty_region = wl_compositor_create_region(compositor);
  struct zwlr_layer_surface_v1 *ls = zwlr_layer_shell_v1_get_layer_surface(
      layer_shell, global_surf, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
      "overlay");
  zwlr_layer_surface_v1_set_anchor(ls, ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                           ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                                           ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
  zwlr_layer_surface_v1_set_size(ls, win_width, win_height);
  zwlr_layer_surface_v1_set_margin(ls,
                                   20, // top
                                   50, // right
                                   50,  // bottom
                                   0); // left
  zwlr_layer_surface_v1_add_listener(ls, &ls_listener, global_surf);

  /*  if (seat)
   {
       struct wl_pointer *ptr = wl_seat_get_pointer(seat);
       wl_pointer_add_listener(ptr, &pointer_listener, global_surf);
   } */

  // ---- MONITOR DE MANDOS (INOTIFY) ----
  inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (inotify_fd >= 0) {
    // Vigilamos /dev/input para capturar cuando se crea (IN_CREATE) o borra
    // (IN_DELETE) algo
    inotify_add_watch(inotify_fd, "/dev/input", IN_CREATE | IN_DELETE);
  }
  // --------------------------------------
  wl_surface_set_input_region(global_surf, empty_region);

  wl_surface_commit(global_surf);
  init_cairo_static();
}
void toastVolumen() {
  nk_clear(&ctx);
  //ctx.style.window.background = nk_rgba(0, 0, 0, 0);
  if (nk_begin(&ctx, "", nk_rect(0, 0, win_width, win_height),
               NK_WINDOW_NO_SCROLLBAR)) {
    // clear_screen(&ctx, nk_rgba(0, 0, 0, 0));
    struct nk_command_buffer *canvas = nk_window_get_canvas(&ctx);
    nk_fill_rect(canvas, nk_rect(0, 0, win_width, win_height), 25,
                 nk_rgba(10, 15, 10, 210));
    if (IsMuted()) {
      /* code */
      //printf("muted volumen hacemos toast\n");
      nk_layout_row_dynamic(&ctx, 30, 1);
      nk_label(&ctx, "VOLUMEN MUTED", NK_TEXT_CENTERED);
      nk_label(&ctx, "\ueee8", NK_TEXT_CENTERED);
    } else {
      nk_layout_row_dynamic(&ctx, 30, 1);
      nk_label(&ctx, "VOLUMEN", NK_TEXT_CENTERED);
      char vol_text[32];

      // 2. Formateas el texto y le inyectas tu variable entera
      // El "%%" al final es para que dibuje el símbolo de porcentaje literal
      // (%)
      snprintf(vol_text, sizeof(vol_text), "\uf028 %d%%", m_shared->volume);

      // 3. Se lo pasas al label de Nuklear
      nk_label(&ctx, vol_text, NK_TEXT_CENTERED);
    }
  }
  nk_end(&ctx);
}
void cairoSetup() {
  cairo_surface_t *c_surf = cairo_image_surface_create_for_data(
      (unsigned char *)shm_data_global, CAIRO_FORMAT_ARGB32, win_width,
      win_height, win_width * 4);
  cairo_t *cr = cairo_create(c_surf);
  cairo_select_font_face(cr, "3270 Nerd Font Propo", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size(cr, 28);

  cairo_set_source_rgb(cr, 0.20, 1.00, 0.20);
  // cairo_rounded_rectangle(cr,
  //                      10,
  //                      10,
  //                      380,
  //                      100,
  //                      16);   // radio
  // cairo_fill(cr);
  cairo_move_to(cr, 30, 60);

  //draw_nuklear_to_cairo(&ctx, cr);
  cairo_destroy(cr);
  cairo_surface_destroy(c_surf);
  // nk_clear(&ctx);
}
void toastBrillo() {
  nk_clear(&ctx);
  if (nk_begin(&ctx, "Zaramaga", nk_rect(0, 0, win_width, win_height),
               NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(&ctx, 30, 1);
    nk_label(&ctx, "BRILLO: 80%", NK_TEXT_CENTERED);
    nk_layout_row_dynamic(&ctx, 40, 1);
    if (nk_button_label(&ctx, "VOLVER")) {
      // Aquí podríamos cambiar de interfaz dinámicamente
    }
  }
  nk_end(&ctx);
}
void toastCero() {
  nk_clear(&ctx);

  // 1. Cambiamos el fondo de la ventana ANTES de nk_begin.
  // Usamos 'nk_style_item_color' para convertir el color al tipo correcto.
  // 'nk_style_push_style_item' guarda el estilo anterior automáticamente.
  nk_style_push_style_item(&ctx, &ctx.style.window.fixed_background,
                           nk_style_item_color(nk_rgba(0, 0, 0, 0)));

  // Si necesitas que la ventana sea 100% invisible para pintar tú todo en el
  // canvas, entonces pasa nk_rgba(0,0,0,0) en la línea de arriba.

  if (nk_begin(&ctx, "ToastWindow", nk_rect(0, 0, win_width, win_height),
               NK_WINDOW_NO_SCROLLBAR)) {

    // Si decides hacer la ventana invisible arriba y quieres dibujar un fondo
    // manual:
    /*
    struct nk_command_buffer *canvas = nk_window_get_canvas(&ctx);
    nk_fill_rect(canvas, nk_rect(0, 0, win_width, win_height), 0, nk_rgba(10,
    15, 10, 230));
    */

    // Aquí van tus elementos de interfaz (botones, etiquetas, etc.)
  }
  nk_end(&ctx);

  // 2. Restauramos el estilo original para que no afecte a las demás ventanas
  nk_style_pop_style_item(&ctx);
}
// Nueva interfaz para el Toast del mando
void toastGamepad() {
  nk_clear(&ctx);
  if (nk_begin(&ctx, "", nk_rect(0, 0, win_width, win_height),
               NK_WINDOW_NO_SCROLLBAR)) {
    struct nk_command_buffer *canvas = nk_window_get_canvas(&ctx);

    nk_fill_rect(canvas, nk_rect(0, 0, win_width, win_height), 25,
                 nk_rgba(10, 15, 10, 210));
    nk_layout_row_dynamic(&ctx, 30, 1);
    nk_label(&ctx, "\uf11b MANDO DETECTADO", NK_TEXT_CENTERED);

    nk_layout_row_dynamic(&ctx, 30, 1);
    char estado[64];
    snprintf(estado, sizeof(estado), "%s", gamepad_action);
    nk_label(&ctx, estado, NK_TEXT_CENTERED);
  }
  nk_end(&ctx);
}
void limpiaexit() // llamamos atxit
{
  nk_free(&ctx);
  // nk_font_atlas_clear(&atlas);
  cairo_destroy(cr_global);
  cairo_surface_destroy(c_surf_global);
  char cmd[256];

  snprintf(cmd, sizeof(cmd), "ps -o pid,ppid,pgid,comm -g %d", getpgrp());

  system(cmd);
  //printf("Mi PID  = %d\n", getpid());
  //printf("Mi PGID = %d\n", getpgrp());
  // printf("metrics = %d\n", pid_metrics);
  //printf("Ztoast: Saliendo, matando procesos hijos...\n");

  kill(-getpgrp(), SIGTERM);
  /* if (hilo > 0) {
    kill(hilo, SIGTERM);
    waitpid(hilo, NULL, 0);
    printf("ZaramagaOS: Hilo de métricas terminado.\n");
  }
  if (pid_metrics > 0) {
    kill(pid_metrics, SIGTERM);
    waitpid(pid_metrics, NULL, 0);
  }
  if (kill(pid_metrics, SIGKILL) == -1) {
    perror("kill");
  }
  if (pid_audio > 0) {
    kill(pid_audio, SIGTERM);
    waitpid(pid_audio, NULL, 0);
  }
  if (logo_surface)
    cairo_surface_destroy(logo_surface);

  if (logo_pixels)
    stbi_image_free(logo_pixels); */
}
static bool IsMuted(void) {
  FILE *fp = popen("pactl get-sink-mute @DEFAULT_SINK@", "r");
  if (!fp)
    return false;

  char line[64];
  bool muted = false;

  if (fgets(line, sizeof(line), fp)) {
    muted = strstr(line, "yes") != NULL;
    // m_shared->volume = -1;
  }

  //printf("esta muted %d\n", muted);

  pclose(fp);
  return muted;
}
#include <math.h>

static void cairo_rounded_rectangle(cairo_t *cr, double x, double y, double w,
                                    double h, double r) {

  //printf("DIBUJANDO REDONDEADO\n");

  
  if (r <= 0.0) {
   
    cairo_rectangle(cr, x, y, w, h);
    return;
  }

  if (r > w / 2.0)
    r = w / 2.0;
  if (r > h / 2.0)
    r = h / 2.0;

  cairo_new_sub_path(cr);

  cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0);
  cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
  cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
  cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
  //cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
  cairo_close_path(cr);
}