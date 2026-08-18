# ⚙️ Guía de Configuración del Planificador de Kernel

---

## 🚀 1. Parámetros de BORE (`sched_bore = 1`)
> **Nota:** Estos son los ajustes exclusivos que controlan el comportamiento de las ráfagas (*bursts*) cuando el parche BORE está activo.

### 🔹 `kernel.sched_bore`
* **Descripción:** Enciende o apaga el mecanismo BORE.
* **Rango:** `0` (Desactivado) o `1` (Activado) 
* **Por defecto:** `1`

### 🔹 `kernel.sched_burst_penalty_scale`
* **Descripción:** Define con qué fuerza castiga o discrimina el planificador a los procesos que consumen CPU de forma continua (*tragones*) frente a los interactivos.
* **Rango:** `0` a `4095` 
* **Por defecto:** `1536`

### 🔹 `kernel.sched_burst_smoothness`
* **Descripción:** Controla la suavidad en la transición de las puntuaciones de prioridad para evitar cambios bruscos.
* **Rango:** `0` a `3` 
* **Por defecto:** `1`

### 🔹 `kernel.sched_burst_penalty_offset`
* **Descripción:** Desplazamiento matemático para evitar que las tareas de ráfagas muy cortas dominen excesivamente.
* **Rango:** `0` a `63` 
* **Por defecto:** `24`

### 🔹 `kernel.sched_burst_cache_lifetime`
* **Descripción:** Tiempo en nanosegundos que el kernel almacena en caché la puntuación de ráfaga calculada para los procesos hijos.
* **Rango:** `0` a `4294967295` 
* **Por defecto:** `75000000` *(75 ms)*

### 🔹 `kernel.sched_burst_inherit_type`
* **Descripción:** Define cómo heredan los procesos hijos las características de los padres (por ejemplo, usando un árbol jerárquico avanzado).
* **Rango:** `0` a `2` 
* **Por defecto:** `2`

---

## 🛡️ 2. Parámetros Estándar (`sched_bore = 0` o Planificador Base)
> **Nota:** Si desactivas BORE, los parámetros anteriores dejan de existir. En su lugar, el kernel base (CFS/EEVDF) utiliza los ajustes clásicos de latencia y granularidad (en nanosegundos).

### 🔸 `kernel.sched_latency_ns`
* **Descripción:** El período de tiempo objetivo en el que el planificador intenta ejecutar todas las tareas activas al menos una vez.
* **Rango:** Ajustable en nanosegundos (ej. `6000000` para 6ms).

### 🔸 `kernel.sched_min_granularity_ns`
* **Descripción:** El tiempo mínimo garantizado que se le permite ejecutar a una tarea de forma continuada bajo alta carga para evitar un exceso de cambios de contexto.
* **Rango:** Ajustable en nanosegundos inferiores a la latencia (ej. `750000` para 0.75ms).

### 🔸 `kernel.sched_wakeup_granularity_ns`
* **Descripción:** Define la sensibilidad de preempción cuando un proceso se despierta (priorizando la respuesta inmediata de mandos de juego o eventos de red).
* **Rango:** Ajustable en nanosegundos (ej. `1000000` para 1ms).