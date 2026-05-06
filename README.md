# Plataforma de Streaming – Proyecto Parte 1
Programación III (2026‑1)

**Integrantes**
* Collazos Solis, Maxwell Lupo Gregorio
* Heredia Cadenas Guillermo Arturo
* Noreña Paredes, Steven Daniel
* Vizcardo Chavez, Juan Diego
## Preprocesamiento de datos
Antes de ingresar los datos al arbol, cada pelicula pasa por un proceso de limpieza en 3 pasos.
### Objetivo
Convertir el texto crudo del CSV en palabras uniformes y limpias listas para ser ingresadas al Suffix Tree.
### Campos procesados
Se procesan todos los campos de cada pelicula: titulo, origen, director, cast, genero y plot. Todos se juntan en un solo texto y se limpian juntos.
### Pasos
**Paso 1 — Convertir a minúsculas**
Todo el texto se convierte a minúsculas para que la búsqueda no distinga entre mayúsculas y minúsculas.
```
"Kansas Saloon Smashers" -> "kansas saloon smashers"
```
**Paso 2 — Eliminar puntuación**
Se recorre el texto carácter por carácter. Todo lo que no sea letra o número se reemplaza por un espacio. También se eliminan las referencias como `[1]` que aparecen en los plots. Los caracteres especiales como `—` también se eliminan.
```
"bartender, serving drinks.[1]" -> "bartender  serving drinks "
"presidents—abraham"           -> "presidents abraham"
```
**Paso 3 — Separar en palabras**
El texto se divide en palabras individuales usando los espacios como separador. Los espacios dobles que quedaron del paso anterior se ignoran automáticamente.
```
"bartender  serving drinks " -> ["bartender", "serving", "drinks"]
```
### Ejemplo completo
```
Entrada CSV:
"The Martyred Presidents—Abraham Lincoln, James A. Garfield[1]"

Paso 1: minúsculas
"the martyred presidents—abraham lincoln, james a. garfield[1]"

Paso 2: eliminar puntuación
"the martyred presidents abraham lincoln  james a  garfield "

Paso 3: separar en palabras
["the", "martyred", "presidents", "abraham", "lincoln", "james", "a", "garfield"]
```
