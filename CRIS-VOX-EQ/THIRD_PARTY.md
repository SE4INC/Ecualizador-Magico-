# Licencias y fuentes

Fuentes originales de CRIS VOX EQ bajo AGPL-3.0-only. El texto completo está en LICENSE.

JUCE 8.0.13 se obtiene del commit `7c9d3783b127263d72bb65fe0a7e2dc8a02a7ac2`:

https://github.com/juce-framework/JUCE/tree/7c9d3783b127263d72bb65fe0a7e2dc8a02a7ac2

Sus módulos se ofrecen bajo AGPLv3 o licencia comercial. El aviso original se conserva en `licenses/JUCE-LICENSE.md`; no se incluye licencia comercial. JUCE contiene el SDK VST3 y otros componentes con avisos propios dentro de sus fuentes, conservados al descargar el repositorio.

El test de integración utiliza `examples/Assets/singing.ogg` de ese árbol como muestra real corta para comprobar procesamiento. No se copia a las fuentes ni al paquete de binarios. No se han usado grabaciones de artistas o del usuario para crear presets.

El ecualizador, FFT, reglas y FIFO de este proyecto están implementados en sus propias fuentes; no requieren una API de IA o una biblioteca externa de aprendizaje automático. Al redistribuir binarios, acompáñalos de las fuentes y scripts correspondientes y de los avisos/fuentes de las dependencias. CMake identifica la revisión exacta necesaria.
