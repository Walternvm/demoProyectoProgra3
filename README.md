# Plataforma de Streaming – Proyecto Final
Programación III (2026‑1)

**Integrantes**
* Collazos Solis, Maxwell Lupo Gregorio
* Heredia Cadenas Guillermo Arturo
* Noreña Paredes, Steven Daniel
* Vizcardo Chavez, Juan Diego

---

## Descripción del proyecto
 
Se implementa una plataforma de streaming que permite buscar películas por palabras, frases o subcadenas en el título y la sinopsis, así como por tags (director, cast, género, etc.). El sistema muestra las mejores coincidencias ordenadas por relevancia, permite navegar entre páginas de resultados, visualizar la sinopsis completa y marcar "Like" o "Ver más tarde".
 
---
 
## Estructura del proyecto
 
```
├── data_movies.csv        # Dataset con 34,887 películas
├── main.cpp               # Punto de entrada del programa
├── movie.h                # Estructura de datos de una película
├── preprocesador.h/.cpp   # Limpieza y lectura del CSV
├── suffix_trie.h/.cpp     # Árbol de búsqueda (Suffix Trie)
├── sistema.h/.cpp         # Lógica de búsqueda e interfaz
└── README.md              # Documentación
```
 
---
 
## 1. Pre-procesamiento de datos
 
### Objetivo
Convertir el texto crudo del CSV en palabras limpias y uniformes, listas para ser ingresadas al Suffix Trie. Este paso es crítico porque el árbol indexa exactamente los tokens que recibe.
 
### Campos procesados
Se procesan todos los campos de cada película: título, origen, director, cast, género y plot. Todos se unen en un solo texto y pasan por el mismo proceso de limpieza.
 
### Pasos de limpieza
 
**Paso 1 — Convertir a minúsculas**
 
Todo el texto se convierte a minúsculas para que la búsqueda no distinga entre mayúsculas y minúsculas. Sin esto, "Kansas" y "kansas" serían nodos distintos en el árbol.
 
```
"Kansas Saloon Smashers"  →  "kansas saloon smashers"
```
 
**Paso 2 — Eliminar puntuación y caracteres especiales**
 
Se recorre el texto carácter por carácter. Todo lo que no sea letra o número se reemplaza por un espacio. También se eliminan referencias como [1] que aparecen en los plots, y caracteres especiales como el guión largo.
 
```
"bartender, serving drinks.[1]"  →  "bartender  serving drinks "
"presidents—abraham"             →  "presidents abraham"
```
 
**Paso 3 — Separar en palabras**
 
El texto se divide en palabras individuales usando los espacios como separador. Los espacios dobles del paso anterior se ignoran automáticamente. Cada palabra resultante es un token que se insertará en el Suffix Trie.
 
```
"bartender  serving drinks "  →  ["bartender", "serving", "drinks"]
```
 
**Paso 4 — Eliminar stopwords**
 
Se eliminan palabras que aparecen en casi todas las películas y no aportan valor a la búsqueda: artículos, preposiciones, conjunciones, etc.
 
```
["the", "bartender", "is", "serving", "drinks", "a", "customers"]
                    ↓ eliminar stopwords
["bartender", "serving", "drinks", "customers"]
```
 
Las stopwords filtradas son:
```
"a", "an", "the", "and", "or", "but", "in", "on", "at", "to", "for",
"of", "with", "by", "from", "is", "are", "was", "were", "be", "been",
"being", "have", "has", "had", "do", "does", "did", "will", "would",
"could", "should", "may", "might", "can", "shall", "you", "he", "she",
"it", "we", "they", "his", "her", "its", "our", "their", "this", "that",
"these", "those", "not", "no", "nor", "so", "if", "then", "than",
"too", "just", "about", "also", "very", "s", "t", "ll", "ve", "re"
```
 
### Ejemplo completo
 
```
Entrada CSV:
"The Martyred Presidents—Abraham Lincoln, James A. Garfield[1]"
 
Paso 1: minúsculas
"the martyred presidents—abraham lincoln, james a. garfield[1]"
 
Paso 2: eliminar puntuación y caracteres especiales
"the martyred presidents abraham lincoln  james a  garfield "
 
Paso 3: separar en palabras
["the", "martyred", "presidents", "abraham", "lincoln", "james", "a", "garfield"]
 
Paso 4: eliminar stopwords  (se eliminan "the" y "a")
["martyred", "presidents", "abraham", "lincoln", "james", "garfield"]
 
Resultado: tokens listos para insertar en el Suffix Trie
```
 
### Código del pre-procesamiento
 
```cpp
// Paso 1: minúsculas
string aMinusculas(string texto) {
    transform(texto.begin(), texto.end(), texto.begin(), ::tolower);
    return texto;
}
 
// Paso 2: eliminar puntuación, referencias [1] y caracteres especiales
string eliminarPuntuacion(string texto) {
    string resultado;
    bool dentroDeCorchete = false;
    for (unsigned char letra : texto) {
        if      (letra == '[')      dentroDeCorchete = true;
        else if (letra == ']')      dentroDeCorchete = false;
        else if (dentroDeCorchete)  continue;
        else if (letra >= 128)      resultado += ' ';
        else if (ispunct(letra))    resultado += ' ';
        else                        resultado += letra;
    }
    return resultado;
}
 
// Paso 3: separar en palabras individuales
vector<string> separarEnPalabras(string texto) {
    vector<string> palabras;
    string palabra;
    for (char letra : texto) {
        if (letra == ' ') {
            if (!palabra.empty()) {
                palabras.push_back(palabra);
                palabra.clear();
            }
        } else {
            palabra += letra;
        }
    }
    if (!palabra.empty()) palabras.push_back(palabra);
    return palabras;
}
 
// Paso 4: filtrar stopwords
const unordered_set<string> stopwords = { "a", "an", "the", "and", ... };
 
// Aplica los 4 pasos y devuelve los tokens listos para el árbol
vector<string> limpiarCampo(string texto) {
    vector<string> palabras = separarEnPalabras(
                                  eliminarPuntuacion(
                                      aMinusculas(texto)));
    vector<string> filtradas;
    for (const string& p : palabras)
        if (stopwords.find(p) == stopwords.end())
            filtradas.push_back(p);
    return filtradas;
}
```
 
---
 
## 2. Pseudo-código de inserción al árbol
 
Una vez que los tokens están limpios, se insertan en el Suffix Trie. Por cada película se generan todos los sufijos de cada token y se recorre el árbol letra por letra, creando nodos donde no existen.
 
```
PARA cada pelicula en el CSV:
 
    texto = titulo + origen + director + cast + genero + plot
    tokens = limpiarCampo(texto)
 
    PARA cada token en tokens:
        agregarToken(token, idPelicula)
 
 
FUNCION agregarToken(token, idToken):
    SI token esta vacio O ya fue insertado antes:
        salir
    insertarToken(token, idToken)
    marcar token como ya insertado
 
 
FUNCION insertarToken(token, idToken):
    longitud = largo(token)
 
    PARA start desde 0 hasta longitud:
        nodo = raiz
        limite = min(longitud, start + 25)
 
        PARA i desde start hasta limite:
            letra = token[i]
            SI letra es caracter especial: ignorar y continuar
            SI nodo NO tiene hijo con esa letra:
                crear nuevo nodo hijo vacio
            nodo = nodo.children[letra]
            nodo.ids.agregar(idToken)
```
 
### Ejemplo visual de inserción
 
```
token "kansas" de la pelicula #1:
 
sufijo "kansas":  raiz -> k -> a -> n -> s -> a -> s   [pelicula #1]
sufijo "ansas":   raiz -> a -> n -> s -> a -> s        [pelicula #1]
sufijo "nsas":    raiz -> n -> s -> a -> s             [pelicula #1]
sufijo "sas":     raiz -> s -> a -> s                  [pelicula #1]
sufijo "as":      raiz -> a -> s                       [pelicula #1]
sufijo "s":       raiz -> s                            [pelicula #1]
```
 
Gracias a esto, buscar "kan", "ans", "nsa" o cualquier fragmento de "kansas" lleva directamente al resultado sin recorrer las 34,887 películas.
 
---
 
## 3. Elección de la estructura de datos: **Suffix Tree**
### Justificación
- La búsqueda debe soportar **cualquier subcadena** (palabra completa, fragmento de palabra o frase).
- Un **suffix tree** (o su variante simple, un **suffix trie**) permite encontrar todas las ocurrencias de un patrón en tiempo proporcional a la longitud del patrón, una vez construido el índice.
- A diferencia de un Trie normal de palabras, un suffix tree almacena todos los sufijos del texto concatenado, por lo que responde de forma natural a consultas como `"bar"` (que encuentra *barco*, *embarcación*, etc.).
- La implementación puede mantenerse con **sintaxis básica de C++** (arreglos de punteros, vectores) para cumplir con el requisito de simplicidad.

### Construcción del índice generalizado
1. Se pre‑procesa el archivo CSV y se genera un texto limpio por película (título + sinopsis + tags, sin stopwords).
2. Se concatena el texto de todas las películas, separando cada película con un carácter único no imprimible (e.g., `'$'`).
3. Se insertan **todos los sufijos** de cada película en un **Suffix Trie**. Cada nodo almacena una lista de IDs de las películas en las que aparece ese sufijo.
4. Durante la inserción se mapea cada sufijo a su ID de película (duplicando la referencia en nodos compartidos).

### Búsqueda de patrones
- **Subcadena suelta**: se recorre el trie con los caracteres del patrón. Si se llega a un nodo, se devuelven todos los IDs de película almacenados en él y en sus descendientes (todos los sufijos que empiezan con el patrón).
- **Frase (varias palabras)**: se divide la frase en palabras, se busca cada palabra por separado y se realiza la **intersección** de los conjuntos de películas resultantes.
- **Tag**: los campos `director`, `cast`, `género`, etc., se concatenan con un prefijo fijo (ej. `director:spielberg`). Al buscar por tag se busca la cadena `"director:" + nombre` y se recuperan las películas que contengan ese prefijo exacto. Esto evita coincidencias en otros campos.

---

### Algoritmo de inserción
 
```
FUNCION agregarToken(token, idToken):
    SI token esta vacio O ya fue insertado antes:
        salir
    insertarToken(token, idToken)
    marcar token como ya insertado
 
FUNCION insertarToken(token, idToken):
    longitud = largo(token)
 
    PARA start desde 0 hasta longitud:
        nodo = raiz
        limite = min(longitud, start + 25)
 
        PARA i desde start hasta limite:
            letra = token[i]
            SI letra es caracter especial (>= 128): ignorar y continuar
            SI nodo NO tiene hijo con esa letra:
                crear nuevo nodo hijo vacio
            nodo = nodo.children[letra]
            nodo.ids.agregar(idToken)
```
 
El límite de 25 caracteres por sufijo es un balance entre velocidad y uso de memoria.
 
### Algoritmo de búsqueda
 
```
FUNCION buscar(patron):
    nodo = raiz
 
    PARA cada letra en patron:
        SI letra es caracter especial: retornar vacio
        SI nodo NO tiene hijo con esa letra: retornar vacio
        nodo = nodo.children[letra]
 
    retornar lista de ids del nodo actual
 
 
FUNCION buscarFrase(frase):
    palabras = separarEnPalabras(aMinusculas(frase))
    resultados = conjunto vacio
 
    PARA cada palabra en palabras:
        ids = buscar(palabra)
        agregar ids a resultados
 
    retornar resultados
```
 
### Ejemplo visual de búsqueda
 
```
Usuario busca: "bar"
 
recorrer el arbol letra por letra:
raiz -> 'b' -> 'a' -> 'r'   nodo encontrado
 
recolectar ids del nodo:
ids = [token "bartender", token "harbor", token "bar"]
 
traducir a peliculas:
"bartender" -> pelicula #1  "Kansas Saloon Smashers"
"harbor"    -> pelicula #5  "The Pirate Ship"
"bar"       -> pelicula #12 "The Barber Shop"
 
Resultado: 3 peliculas encontradas
```
 
---
 
## 4. Avance de la interfaz
 
El programa corre en consola con un menú principal:
 
```
==================================================
         PLATAFORMA DE STREAMING
==================================================
1. Buscar pelicula por palabras
2. Buscar por director
3. Buscar por genero
4. Buscar por actor
5. Ver mas tarde
6. Ver recomendaciones
7. Salir
Opcion: 1
```
 
Búsqueda y resultados:
```
Ingrese palabra o frase a buscar: saloon
 
Buscando: "saloon"...
Resultados encontrados: 8
 
--- Resultados 1-5 ---
1. Kansas Saloon Smashers (1901)  - Relevancia: 6
2. The Saloon Keeper (1904)       - Relevancia: 4
3. Cowboy Saloon (1910)           - Relevancia: 3
4. The Last Saloon (1913)         - Relevancia: 2
5. Saloon Girls (1920)            - Relevancia: 1
 
Opcion (1-5 para ver pelicula, 6 para mas resultados, 0 para volver): 1
```
 
Detalle de película:
```
==================================================
Titulo:   Kansas Saloon Smashers (1901)
Director: Unknown
Cast:
Genero:   unknown
Origen:   American
Sinopsis: A bartender is working at a saloon, serving drinks...
==================================================
1. Dar Like
2. Ver mas tarde
3. Volver
Opcion:
```
 
Funcionalidades implementadas:
- Busqueda por palabras o frases — encuentra peliculas por cualquier subcadena en titulo o sinopsis
- Busqueda por tag — busca por director, genero o actor
- Paginacion — muestra 5 resultados a la vez con opcion de ver mas
- Likes — marca peliculas favoritas
- Ver mas tarde — guarda peliculas para ver despues
- Recomendaciones — sugiere peliculas similares a las que diste like
