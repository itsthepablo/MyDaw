### 1. Ruteo y Sidechain
- [ ] **Sidechain Avanzado con EQ Interno**: Permitir que la señal de sidechain pase por un filtro (Low Pass / Band Pass) antes de llegar al plugin.
    - *Nota*: En Sidechain el desfase no importa porque la señal no se escucha, es solo control.
- [ ] **Envíos (Sends) con EQ Interno**: Aplicar filtros al audio enviado a un bus.
    - *¡Cuidado!*: Aquí la fase es crítica para evitar cancelación (Comb Filtering). Evaluar el uso de **Fase Lineal** o compensación de retardo estricta si se usa en procesado paralelo.

### 2. Estética y UI
- [ ] **Temas Dinámicos**: Cambiar el color del track según el tipo de contenido automáticamente.
- [ ] **Micro-animaciones**: Añadir suavizado a las agujas de los medidores de Loudness para que se sientan más "analógicas".

### 3. Modularización
- [ ] **Clip Media Manager**: Extraer la carga de archivos y gestión de thumbnails de `Track.h`.
- [ ] **PDC Latency System**: Modularizar el sistema de compensación de latencia.
