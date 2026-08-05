#include <stdio.h>
#include <wayland-client.h>

int main(void)
{
    struct wl_display *display = wl_display_connect(NULL);

    if (!display) {
        puts("No se pudo conectar al compositor Wayland");
        return 1;
    }

    puts("¡Conectado a Wayland!");

    wl_display_disconnect(display);
    return 0;
}