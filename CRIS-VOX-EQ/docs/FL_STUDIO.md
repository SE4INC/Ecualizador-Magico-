# Instalación y primera prueba en Windows

1. Sube las fuentes extraídas a un repositorio nuevo, con CMakeLists.txt y .github en la raíz.
2. Ejecuta el workflow de Actions y espera el resultado correcto del trabajo Windows.
3. Descarga **CRIS-VOX-EQ-Windows-x64** y extrae el artefacto.
4. Cierra FL Studio antes de copiar o reemplazar el plugin.
5. Copia la carpeta completa `CRIS VOX EQ.vst3` a `C:\Program Files\Common Files\VST3` o una carpeta VST3 configurada en tu host. La carpeta de Program Files puede pedir permisos de administrador.
6. En el gestor de plugins de FL Studio comprueba la ruta y realiza la búsqueda de plugins nuevos.
7. Insértalo como efecto en el canal de una voz grabada, con BYPASS desactivado.
8. Empieza con NEUTRO. Prueba RAP GRAVE - CLARIDAD si ese es tu objetivo, conservando la opción de desactivar o ajustar sus bandas.
9. Deja IGUALAR VOLUMEN activado, reproduce una frase representativa y pulsa ANALIZAR.
10. Compara cada propuesta con ESCUCHAR/DETENER antes de APLICAR. Analiza de nuevo si cambias la EQ.

El archivo VST3 de Linux incluido en la entrega no funciona en FL Studio para Windows. No debes renombrar una biblioteca `.so` como `.vst3` de Windows. GitHub Actions realiza una compilación Windows separada.

## Comprobación de aceptación en FL Studio

- Escaneo y apertura/cierre repetido del editor.
- Sesiones a 44,1/48/96 kHz; buffers 64/128/256/512/1024/2048.
- Señal mono y estéreo; entrada silenciosa y niveles altos.
- Automatización de frecuencia, ganancia, Q, activación, tipo, OUTPUT y bypass.
- Cambio de presets, A/B, copia de bancos y recuperación del proyecto.
- Análisis con audio, sin reproducción, con ruido y después de cambiar ajustes.
- ESCUCHAR sin guardar cambios, DETENER, APLICAR y DESCARTAR.
- Interfaz abierta/cerrada y varias instancias, con CPU media y picos.
- Exportación offline del proyecto y escucha comparativa con el original.

Estas comprobaciones en el host real siguen pendientes. La validación local está descrita en VALIDATION.md. Un buffer de 2048 da más tiempo por bloque, pero no certifica el rendimiento global de una sesión.
