# Monitor de sesiones largas

El monitor combina telemetría ligera del motor con métricas de Linux. Registra
FPS y frametimes una vez por segundo, y de forma predeterminada toma cada cinco
segundos una muestra de CPU, RAM, I/O, frecuencia, uso de GPU, VRAM, batería y
todos los sensores térmicos que el kernel exponga mediante `hwmon`.

Primero compila e instala el cliente actualizado:

```bash
make install
```

Después inicia una sesión normal desde la raíz del repositorio:

```bash
./tools/performance/monitor_session.sh
```

Juega como de costumbre y cierra Cowbraten normalmente al terminar. El monitor
creará una carpeta con fecha dentro de `session_reports/`. Para limitar la
prueba a un tiempo concreto se aceptan segundos, minutos u horas:

```bash
./tools/performance/monitor_session.sh --duration 2h
```

Los argumentos posteriores a `--` se envían al juego. Por ejemplo:

```bash
./tools/performance/monitor_session.sh --duration 45m -- -w1920 -h1080 -t0
```

Cada carpeta contiene `report.md`, `system.csv`, `engine.csv` y
`metadata.json`. Al finalizar también se crea un `.zip` junto a la carpeta;
ése es el archivo que puedes compartir directamente. El CSV del motor se
escribe incrementalmente y el del sistema se vacía en cada muestra, así que
una interrupción o cierre inesperado conserva casi toda la sesión.

La opción `--game-home` sólo es necesaria si el launcher usa un directorio
distinto de `~/.cowbraten` con `-q`. Se puede reducir el intervalo externo, con
un mínimo de 0.25 s, pero para sesiones largas el valor predeterminado de 5 s
produce menos ruido y archivos pequeños.
