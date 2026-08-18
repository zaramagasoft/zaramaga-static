BORE
────

sched_bore
    0 = OFF
    1 = ON
    [PROBADO] ✓
    ativar/desactivar BORE
///////////////////////////////////////////////////////
sched_burst_inherit_type 
    0..2
    [PENDIENTE] ○
0 — sin herencia
PADRE
  burst/score
       │
       X
       │
     HIJO

El hijo no hereda la penalización BORE del padre.

1 — herencia directa
PADRE
  burst/score
       │
       ↓
     HIJO

El hijo hereda la información BORE del padre directo.

2 — herencia por ancestro
ABUELO
  │
  ↓
PADRE
  │
  ↓
HIJO
/////////////////////////////////////
sched_burst_smoothness
    0..3
    [PENDIENTE] ○
ontrola cómo de suavemente BORE actualiza la penalización de una tarea. En términos prácticos:

0 → respuesta más directa, menos suavizado.
1 → suavizado moderado; es nuestro valor actual.
2 → más suavizado.
3 → máximo de este parámetro.

La idea conceptual:

smoothness bajo
CPU burst ───────→ score
              cambios más rápidos


smoothness alto
CPU burst ───────→ score
                 cambios más suaves
Primero hacemos exactamente la misma prueba desde shell
sudo sysctl kernel.sched_burst_smoothness=0; sleep 1; \
sysctl kernel.sched_burst_smoothness; sleep 1; \
sudo sysctl kernel.sched_burst_smoothness=1; sleep 1; \
sysctl kernel.sched_burst_smoothness; sleep 1; \
sudo sysctl kernel.sched_burst_smoothness=2; sleep 1; \
sysctl kernel.sched_burst_smoothness; sleep 1; \
sudo sysctl kernel.sched_burst_smoothness=3; sleep 1; \
sysctl kernel.sched_burst_smoothness

Deberíamos terminar otra vez en:

kernel.sched_burst_smoothness = 3

Luego podemos devolverlo a nuestro valor de trabajo:

sudo sysctl kernel.sched_burst_smoothness=1
//////////////////////////////////////////////////
sched_burst_penalty_offset
    0..63
    [PENDIENTE] ○
sched_burst_penalty_offset

Este es especialmente interesante porque determina el umbral/tolerancia inicial antes de que el consumo en burst empiece a penalizar de forma significativa.

En nuestro kernel tenemos:

actual = 24
rango  = 0..63

Probamos exactamente igual:

sudo sysctl kernel.sched_burst_penalty_offset=0; sleep 1; \
sysctl kernel.sched_burst_penalty_offset; sleep 1; \
sudo sysctl kernel.sched_burst_penalty_offset=16; sleep 1; \
sysctl kernel.sched_burst_penalty_offset; sleep 1; \
sudo sysctl kernel.sched_burst_penalty_offset=32; sleep 1; \
sysctl kernel.sched_burst_penalty_offset; sleep 1; \
sudo sysctl kernel.sched_burst_penalty_offset=63; sleep 1; \
sysctl kernel.sched_burst_penalty_offset
///////////////////////////////////////////
sched_burst_penalty_scale
    0..4095
    [PENDIENTE] ○
0 → penalización escalada a 0.
valores bajos → BORE penaliza poco.
1536 → valor actual, equivalente a factor 1,5× respecto a la escala base (1536 / 1024).
valores altos → penalización más agresiva.
máximo que tenemos documentado: 4095.
Probémoslo en caliente

Para no hacer 4096 pruebas, hacemos extremos y valores intermedios:

sudo sysctl kernel.sched_burst_penalty_scale=0; sleep 1; \
sysctl kernel.sched_burst_penalty_scale; sleep 1; \
sudo sysctl kernel.sched_burst_penalty_scale=512; sleep 1; \
sysctl kernel.sched_burst_penalty_scale; sleep 1; \
sudo sysctl kernel.sched_burst_penalty_scale=1024; sleep 1; \
sysctl kernel.sched_burst_penalty_scale; sleep 1; \
sudo sysctl kernel.sched_burst_penalty_scale=1536; sleep 1; \
sysctl kernel.sched_burst_penalty_scale; sleep 1; \
sudo sysctl kernel.sched_burst_penalty_scale=2048; sleep 1; \
sysctl kernel.sched_burst_penalty_scale; sleep 1; \
sudo sysctl kernel.sched_burst_penalty_scale=3072; sleep 1; \
sysctl kernel.sched_burst_penalty_scale; sleep 1; \
sudo sysctl kernel.sched_burst_penalty_scale=4095; sleep 1; \
sysctl kernel.sched_burst_penalty_scale
////////////////////////////////////////////////////////////
sched_burst_cache_lifetime
    0..4294967295 ns
    [PENDIENTE] ○
sudo sysctl kernel.sched_burst_cache_lifetime=0; sleep 1; \
sysctl kernel.sched_burst_cache_lifetime; sleep 1; \
sudo sysctl kernel.sched_burst_cache_lifetime=1000000; sleep 1; \
sysctl kernel.sched_burst_cache_lifetime; sleep 1; \
sudo sysctl kernel.sched_burst_cache_lifetime=10000000; sleep 1; \
sysctl kernel.sched_burst_cache_lifetime; sleep 1; \
sudo sysctl kernel.sched_burst_cache_lifetime=75000000; sleep 1; \
sysctl kernel.sched_burst_cache_lifetime; sleep 1; \
sudo sysctl kernel.sched_burst_cache_lifetime=100000000; sleep 1; \
sysctl kernel.sched_burst_cache_lifetime
///////////////////////////////////////////////////////////////
sched_burst_protect_slice_lv
    [tenemos que confirmar sus valores/rangos]
    [PENDIENTE] ○
Y aquí hay algo interesante: no es simplemente un valor que BORE lea pasivamente. Al cambiarlo, su handler modifica las static keys del scheduler:

if (sched_burst_protect_slice_lv == 1 ||
    sched_burst_protect_slice_lv == 2)
    static_branch_enable(&sched_burst_protect_slice_cond_key);
else
    static_branch_disable(&sched_burst_protect_slice_cond_key);


if (sched_burst_protect_slice_lv >= 2)
    static_branch_enable(&sched_burst_protect_slice_prefer_key);
else
    static_branch_disable(&sched_burst_protect_slice_prefer_key);

Por tanto:

0 → protección desactivada


1 → protección condicional


2 → protección condicional + preferencia

Y lo podemos cambiar en vivo.

Prueba
sudo sysctl kernel.sched_burst_protect_slice_lv=0; sleep 1; \
sysctl kernel.sched_burst_protect_slice_lv; sleep 1; \
sudo sysctl kernel.sched_burst_protect_slice_lv=1; sleep 1; \
sysctl kernel.sched_burst_protect_slice_lv; sleep 1; \
sudo sysctl kernel.sched_burst_protect_slice_lv=2; sleep 1; \
sysctl kernel.sched_burst_protect_slice_lv    