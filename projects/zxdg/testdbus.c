#include <pulse/pulseaudio.h>
#include <stdio.h>

// Callback que se ejecuta cuando el servidor de audio emite un evento
void subscription_cb(pa_context *c, pa_subscription_event_type_t t,
                     uint32_t idx, void *userdata) {
  int facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
  int type = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

  // Filtramos: evento de SINK (salida de audio) + tipo CHANGE (cambio de
  // volumen/mute)
  if (facility == PA_SUBSCRIPTION_EVENT_SINK &&
      type == PA_SUBSCRIPTION_EVENT_CHANGE) {
    printf("🔊 ¡CAMBIO DE VOLUMEN DETECTADO!\n");
    fflush(stdout); // Forzamos el volcado a la terminal de inmediato
  }
}

// Callback para gestionar los cambios de estado de la conexión
void state_cb(pa_context *c, void *userdata) {
  pa_context_state_t state = pa_context_get_state(c);

  switch (state) {
  case PA_CONTEXT_READY:
    printf("✅ Conectado a PipeWire/PulseAudio. Escuchando eventos de "
           "volumen...\n");

    // 1. Decimos qué función procesa los eventos
    pa_context_set_subscribe_callback(c, subscription_cb, NULL);

    // 2. Nos suscribimos a los eventos de las salidas de audio (Sinks)
    pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK, NULL, NULL);
    break;

  case PA_CONTEXT_FAILED:
    fprintf(stderr, "❌ Error al conectar con el servidor de audio: %s\n",
            pa_strerror(pa_context_errno(c)));
    break;

  case PA_CONTEXT_TERMINATED:
    printf("Conexión cerrada.\n");
    break;

  default:
    break;
  }
}

int main() {
  int retval = 0;

  // 1. Crear el bucle de eventos de PulseAudio
  pa_mainloop *m = pa_mainloop_new();
  if (!m) {
    fprintf(stderr, "Error al crear el mainloop.\n");
    return 1;
  }

  // 2. Obtener la API del bucle
  pa_mainloop_api *api = pa_mainloop_get_api(m);

  // 3. Crear el cliente de audio
  pa_context *c = pa_context_new(api, "ZaramagaOS_Vol_Test");
  if (!c) {
    fprintf(stderr, "Error al crear el contexto.\n");
    pa_mainloop_free(m);
    return 1;
  }

  // 4. Asignar el callback de estado y conectar
  pa_context_set_state_callback(c, state_cb, NULL);
  if (pa_context_connect(c, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
    fprintf(stderr, "Error en la conexión inicial: %s\n",
            pa_strerror(pa_context_errno(c)));
    pa_context_unref(c);
    pa_mainloop_free(m);
    return 1;
  }

  // 5. Iniciar el bucle bloqueante
  pa_mainloop_run(m, &retval);

  // Limpieza al terminar
  pa_context_unref(c);
  pa_mainloop_free(m);

  return retval;
}