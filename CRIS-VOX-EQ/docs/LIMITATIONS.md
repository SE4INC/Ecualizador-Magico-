# Límites y comprobaciones pendientes

Esta entrega es una versión funcional de desarrollo, no un producto profesional ya certificado ni una reproducción de Pro-Q. La imagen conceptual sirvió como referencia; el código usa controles JUCE propios y su resultado real se muestra en las capturas adjuntas.

- El asistente usa reglas de balance espectral local. No entiende el género, la letra, la intención artística o el instrumental. Tampoco está entrenado con tu voz.
- Las propuestas pueden confundir armónicos naturales con una acumulación; siempre son opcionales.
- El análisis mide el primer canal y se orienta a voz solista. Una mezcla completa, estéreo muy diferente en L/R, ruido, reverb y distorsión pueden producir consejos poco útiles.
- No hay una entrada sidechain de referencia ni comparación con canciones comerciales.
- No hay bandas dinámicas o de-esser, LUFS, fase lineal, oversampling, mid/side, pendientes de corte seleccionables o más de ocho bandas.
- Los controles se leen por bloque, no hay automatización a precisión de muestra. Los coeficientes se suavizan 128 muestras; el comportamiento debe escucharse durante cambios de tipo o movimientos extremos.
- El texto del preset muestra el último preset seleccionado, aunque luego se modifique; no hay gestor de presets de usuario ni indicador de preset modificado.
- El asistente requiere audio continuo y una banda libre. Una propuesta aplicada invalida las demás hasta un nuevo análisis.
- Los consejos y el espectro no se guardan en el proyecto. Los parámetros y ambos bancos A/B sí.
- IGUALAR VOLUMEN utiliza RMS, tiene un tiempo de estabilización y no es un juicio de sonoridad percibida.
- La ganancia puede superar 0 dBFS. No se añade limitador ni reparación de clipping.
- Las capturas de tests pueden mostrar audio diagnóstico sintético. No son análisis de una voz del usuario.
- No se puede garantizar ausencia de picos de CPU para cualquier ordenador o sesión. Las pruebas locales no son mediciones de FL Studio.

Antes de producción: ejecutar Actions Windows/MSVC, probar escaneo/edición/automatización/estado en FL Studio y escuchar comparativas con tomas reales de voces graves. Medir varias instancias, buffers 64–2048, sample rates 44,1/48/96 kHz y render del host. El asistente necesita una evaluación mucho más amplia antes de afirmar precisión clínica o profesional en sus recomendaciones.
