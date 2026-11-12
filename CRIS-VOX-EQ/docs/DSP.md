# Diseño técnico

## Ecualización

Ocho biquads de segundo orden, ligados en L/R, en forma directa II transpuesta y precisión double para coeficientes/estado. El audio entra y sale como float32. Se utilizan fórmulas RBJ para campana, filtros de corte, notch y shelves. La frecuencia efectiva se limita a 0,45 veces la frecuencia de muestreo cuando el parámetro supera el límite útil. Los shelves usan pendiente S=1; el control Q queda desactivado en esos tipos.

Los coeficientes normalizados se interpolan durante 128 muestras. OUTPUT y bypass tienen suavizado de 10 ms. No hay FFT, disco, locks, cadenas de texto ni asignaciones de memoria en processBlock. Las reservas de JUCE, la interfaz y los programas de fábrica ocurren en otras rutas; esto no implica que una llamada del host para guardar estado o cambiar programa sea una operación de tiempo real libre de asignaciones.

Se calcula una relación RMS entre entrada y señal ecualizada, promediada con 750 ms de constante de tiempo. Cada 64 muestras se actualiza la ganancia de compensación, limitada a ±6 dB y suavizada a 500 ms. El silencio no puede disparar una ganancia ilimitada. IGUALAR VOLUMEN facilita la comparación, pero no realiza medición LUFS ni iguala perfectamente el volumen percibido. OUTPUT se aplica después del bypass/compensación.

El bypass mantiene el filtro funcionando y mezcla progresivamente hacia la señal seca. Evita reactivar estados antiguos y no añade un segundo motor de EQ. No se aplica limitador: una suma de realces o OUTPUT alto puede superar 0 dBFS.

La latencia de buffering declarada es cero muestras. Como cualquier IIR de fase mínima, las bandas sí cambian fase y retardo de grupo según la frecuencia. Cero buffering no significa fase idéntica al original.

## Análisis y espectro

El callback copia pares de muestras entrada/salida del primer canal a un FIFO SPSC de capacidad fija 32768, sin mutex. El worker consume los pares, mantiene ventanas de 4096 muestras y realiza FFT radix-2 con ventana Hann. Sus tablas se preparan fuera del callback. El espectro tiene hasta 20 actualizaciones por segundo y se muestra en 256 puntos logarítmicos.

Se observan siete regiones: 80–180, 180–450, 450–1000, 1000–2000, 2000–4500, 4500–9000 y 9000–16000 Hz. Se promedia potencia por bin dentro de cada región y potencia entre ventanas válidas. El umbral de actividad es RMS de salida superior a −45 dBFS; no es un detector de voz entrenado. El primer canal se analiza incluso cuando el procesamiento es estéreo. No se debe usar como analizador independiente de dos voces distintas en L/R.

ANALIZAR observa cuatro segundos de audio recibido, no cuatro segundos de reloj si el host está detenido. Las ventanas silenciosas no aportan evidencia. Se exigen al menos 15 ventanas activas. Si no hay audio continuo en 15 segundos de reloj, se pide repetir. Si el FIFO se desborda durante esa captura, se descarta el diagnóstico. Al cerrar la GUI no se captura espectro salvo que el análisis siga pendiente.

Los resultados son snapshots inmutables publicados con mutex entre worker y GUI. El hilo de audio no toma ese mutex ni consulta esos snapshots. La GUI actualiza a 20 Hz y el dibujo de la respuesta se hace fuera del callback.

## Reglas de recomendaciones

Son heurísticas explicables, no inferencia de un modelo generativo o evaluación de una mezcla. Buscan exceso relativo en graves medios, un realce relativo de medios altos, aspereza y falta relativa de presencia. El contexto de voz grave tolera más graves medios. El contexto claridad permite sugerir presencia con un umbral diferente. El texto reconoce que esos balances pueden ser intencionales.

Cada propuesta es una campana amplia de ±1,5–2 dB. No se recomienda una cantidad a partir de una supuesta curva universal ideal. Una armónica vocal fuerte puede activar una regla aunque no sea una resonancia que convenga eliminar. La prueba de comparación y la decisión del usuario son necesarias.

La previsualización utiliza una banda libre temporalmente; no cambia sus parámetros persistidos. El conjunto de parámetros de cada bloque se obtiene de atomics. Los valores de la propuesta también son atómicos, acotados y suavizados por el motor. APLICAR realiza gestos de automatización sobre frecuencia, ganancia, Q, tipo y activación. No escribe sobre otras bandas activas. Un contador de revisiones invalida el diagnóstico cuando cambian parámetros.

## Persistencia y alcance

APVTS guarda los 44 parámetros, y un contenedor ValueTree guarda los bancos A/B y el último preset. No se guardan grabaciones, FFT ni propuestas en la sesión: se analiza de nuevo cuando haga falta. Los presets no son archivos de modelos ni perfiles de cantantes. No hay ARA, sidechain, bandas dinámicas, procesamiento mid/side o fase lineal en esta versión.
