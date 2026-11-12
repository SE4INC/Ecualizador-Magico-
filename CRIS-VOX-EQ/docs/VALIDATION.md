# Validación — 11 de noviembre de 2026

## Entorno

Linux x86_64, Debian 13, GCC 14.2, CMake 4.4.4, configuración Release y JUCE 8.0.13 fijado en CMake. El entorno es compartido y estas medidas no representan el ordenador del usuario ni el medidor de CPU de FL Studio.

## Pruebas DSP pasadas

- Respuesta neutral transparente.
- Ganancia medida con seno de 1 kHz: campana de +6/−6 dB dentro de 0,03 dB del objetivo a 44,1/48/96 kHz.
- Pasa altos: respuesta de corte y atenuación de graves coherentes con segundo orden.
- Coeficientes de los seis tipos: respuestas finitas y denominadores estables en las frecuencias comprobadas.
- Bypass sin cambio de nivel; compensación RMS convergente en el caso medido.
- Automatización de varias campanas con movimientos rápidos: salida finita en los casos comprobados.
- FFT, clasificación de regiones, regla de graves medios y ausencia de consejos por debajo del umbral de señal.
- Integridad del FIFO y parámetros reales de presets.
- Asignaciones ordinarias/alineadas de memoria: ninguna en el primer bloque preparado ni en los bloques automatizados de EQ comprobados.

## Medición orientativa de CPU

Una instancia estéreo, preset RAP GRAVE, compensación RMS activada y ganancia automatizada. La medición incluye generar el seno de prueba. En una pasada local, procesar aproximadamente un segundo de señal tomó:

| Frecuencia | Buffer 64 | Buffer 128 | Buffer 2048 |
|---|---:|---:|---:|
| 44,1 kHz | 2,06 ms | 1,95 ms | 1,92 ms |
| 48 kHz | 2,14 ms | 2,18 ms | 1,95 ms |
| 96 kHz | 4,52 ms | 4,17 ms | 4,06 ms |

Esto corresponde aproximadamente a 0,2–0,5% de un núcleo para **ese test del motor**, no para todo el plugin dentro de FL Studio. No incluye el hilo FFT, dibujo de GUI, coste del host, muchas instancias o peores picos. Los logs adjuntos contienen otra pasada y pueden diferir. No se promete “cero lag”.

## Integración JUCE pasada

- Carga de la muestra real `examples/Assets/singing.ogg` del árbol de JUCE, duración 1,36025 s. Se comprueba que RAP GRAVE modifica realmente sus muestras y que la salida es finita. No se certifica que ese ajuste sea apropiado para dicha voz.
- Presets, copia/cambio de bancos A/B y recuperación de parámetros y ambos bancos desde estado serializado.
- Análisis en segundo plano de señal diagnóstica sintética con energía dominante en 280 Hz: genera la propuesta esperada. Eso prueba el recorrido de datos y una regla, no la precisión de un diagnóstico sobre voces humanas.
- Previsualización temporal sin modificar los parámetros persistidos y aplicación posterior a una banda libre.
- Detección de asignaciones con contador por hilo: cero en los callbacks de 2048 muestras comprobados mientras el worker analiza. Las reservas legítimas del worker se excluyen de esa medida.
- Capturas de la interfaz real a 1340×860 y 1100×760, revisadas visualmente. El espectro de esas capturas procede del audio diagnóstico del test, no de una grabación del usuario.

## Validación externa VST3 pasada

pluginval 1.0.4 para Linux, **strictness level 5: SUCCESS**. Se ejecutó con una pantalla X11 virtual accesible. Comprobó escaneo, apertura fría/caliente, programas, editor, editor durante audio, estados, automatización, parámetros y buses.

Las pruebas de procesamiento y automatización del validador usaron 44,1/48/96 kHz y bloques 64/128/256/512/1024. El buffer 2048 se comprueba por separado en los tests DSP y de integración. El validador oficial de Steinberg no se ejecutó: pluginval informa que no se configuró su ruta.

## Pendiente

1. Ejecutar el workflow Windows/MSVC en un repositorio y comprobar su resultado y artefactos.
2. Instalar ese VST3 en FL Studio, verificar escaneo, audio, gestos, automatización, estado y exportación del host.
3. Medir picos y CPU total con GUI y análisis, varias instancias y distintas configuraciones de hardware/buffer.
4. Escuchar comparaciones con tomas de voces masculinas graves reales y con el instrumental. Evaluar utilidad y falsos positivos de las propuestas.

No se ha compilado ni probado un binario Windows en este entorno. El workflow para generarlo está incluido; una configuración de CI no equivale a una ejecución correcta ya realizada. Los binarios adjuntos son exclusivamente Linux x86_64 y la entrega sigue siendo una versión de desarrollo.
