# Plataforma de Streaming – Proyecto Final
Programación III (2026‑1)

**Integrantes**
* Collazos Solis, Maxwell Lupo Gregorio
* Heredia Cadenas Guillermo Arturo
* Noreña Paredes, Steven Daniel
* Vizcardo Chavez, Juan Diego

---

## 1. Descripción del proyecto
Se implementa una plataforma de streaming que permite buscar películas por palabras, frases o subcadenas en el título y la sinopsis, así como por tags (director, cast, género, etc.).  
El sistema muestra las cinco mejores coincidencias, permite navegar entre páginas de resultados, visualizar la sinopsis y marcar “Like” o “Ver más tarde”. Además, al iniciar sugiere las películas guardadas en **Ver más tarde** y recomienda películas similares a las que el usuario ha dado **Like**.

---

## 2. Elección de la estructura de datos: **Suffix Tree**
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

## 3. Pre‑procesamiento de los datos
### Objetivo
Convertir el texto crudo del CSV en palabras limpias y uniformes, listas para ser ingresadas al Suffix Trie.
 
### Campos procesados
Se procesan todos los campos de cada película: **título, origen, director, cast, género y plot**. Todos se unen en un solo texto y pasan por el mismo proceso de limpieza.

El programa lee el archivo `data_movies.csv` (formato con comillas escapadas) y realiza:

1. **Limpieza**
    - Conversión a minúsculas.
    - Eliminación de puntuación (excepto cuando está dentro de corchetes, que se omite).
    - Reemplazo de caracteres no ASCII (>= 128) por espacio.
2. **Tokenización**
    - Se separa el texto en palabras utilizando el espacio como delimitador.
3. **Remoción de stopwords**
    - Se filtran palabras vacías (artículos, preposiciones, etc.) para reducir ruido y tamaño del índice.
4. **Construcción de vector de palabras por película**
    - Cada película guarda un `vector<string>` con las palabras limpias (para el cálculo de similitud posterior).

### Ejemplo completo
```
Entrada CSV:
"The Martyred Presidents—Abraham Lincoln, James A. Garfield[1]"
 
Paso 1: minusculas
"the martyred presidents—abraham lincoln, james a. garfield[1]"
 
Paso 2: eliminar puntuacion y caracteres especiales
"the martyred presidents abraham lincoln  james a  garfield "
 
Paso 3: separar en palabras
["the", "martyred", "presidents", "abraham", "lincoln", "james", "a", "garfield"]
 
Paso 4: eliminar stopwords  ->  se eliminan "the" y "a"
["martyred", "presidents", "abraham", "lincoln", "james", "garfield"]
```
### Codigo del pre-procesamiento
```cpp
// Paso 1: minusculas
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
        else if (letra >= 128)      resultado += ' '; // guión largo, tildes, etc.
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
 
// Paso 4: stopwords
const unordered_set<string> stopwords = {
    "a", "an", "the", "and", "or", "but", "in", "on", "at", "to", "for",
    "of", "with", "by", "from", "is", "are", "was", "were", "be", "been",
    "being", "have", "has", "had", "do", "does", "did", "will", "would",
    "could", "should", "may", "might", "can", "shall", "you", "he", "she",
    "it", "we", "they", "his", "her", "its", "our", "their", "this", "that",
    "these", "those", "not", "no", "nor", "so", "if", "then", "than",
    "too", "just", "about", "also", "very", "s", "t", "ll", "ve", "re"
};
 
// Aplica los 4 pasos en orden
vector<string> limpiarCampo(string texto) {
    vector<string> palabras = separarEnPalabras(eliminarPuntuacion(aMinusculas(texto)));
    vector<string> filtradas;
    for (const string& p : palabras)
        if (stopwords.find(p) == stopwords.end())
            filtradas.push_back(p);
    return filtradas;
}
```
## Pseudo-código de inserción al árbol
 
Una vez que las palabras están limpias, se insertan en el Suffix Trie. Por cada película se generan todos los sufijos de cada palabra y se insertan en el árbol apuntando al ID de esa película.
 
```
PARA cada pelicula en el CSV:
 
    texto = titulo + origen + director + cast + genero + plot
    palabras = limpiarCampo(texto)
 
    PARA cada palabra en palabras:
 
        PARA i desde 0 hasta largo(palabra):
            sufijo = palabra desde posicion i hasta el final
 
            nodo = raiz del arbol
            PARA cada letra en sufijo:
                SI nodo NO tiene hijo con esa letra:
                    crear nuevo nodo hijo
                nodo = hijo con esa letra
                nodo.ids.agregar(id de pelicula)
```
 
### Ejemplo visual de inserción
 
```
Película #1: "Kansas Saloon Smashers"
palabra limpia: "kansas"

sufijo "kansas":  raiz -> k -> a -> n -> s -> a -> s
sufijo "ansas":   raiz -> a -> n -> s -> a -> s
sufijo "nsas":    raiz -> n -> s -> a -> s
sufijo "sas":     raiz -> s -> a -> s
sufijo "as":      raiz -> a -> s
sufijo "s":       raiz -> s
```
 
Gracias a esto, buscar `"kan"`, `"ans"`, `"nsa"` o cualquier fragmento de `"kansas"` lleva directamente al resultado.
 
### Tags especiales
 
Los campos director, cast y género se indexan con un prefijo para poder buscarlos de forma exacta sin confundirlos con palabras del plot:
 
```
director "george fleming"  ->  se inserta como  "dir:georgefleming"
género   "drama"           ->  se inserta como  "gen:drama"
actor    "john wayne"      ->  se inserta como  "cas:johnwayne"
```
 
Así, buscar por director no confunde resultados con películas que mencionan ese nombre en el plot.

## 🌳 3. Estructura de datos: Suffix Trie
 
### ¿Por qué un Suffix Trie?
 
El proyecto requiere búsqueda por subcadenas: buscar `"bar"` debe encontrar películas con `"bartender"`, `"harbor"` o `"bar"`. Un BST o AVL solo encuentra palabras exactas. El Suffix Trie resuelve esto guardando **todos los sufijos posibles** de cada palabra del dataset.
 
| Característica | BST / AVL | Suffix Trie |
|---|---|---|
| Buscar palabra exacta (`"kansas"`) | ✅ | ✅ |
| Buscar por prefijo (`"kan"` → `"kansas"`) | ❌ | ✅ |
| Buscar subcadena (`"bar"` → `"bartender"`) | ❌ | ✅ |
| Buscar por fragmento en plot y título | ❌ | ✅ |
| Velocidad con 34,887 películas | lenta | rápida |
 
### Estructura de un nodo
 
Cada nodo del árbol tiene:
- **children[128]** — un puntero hijo por cada carácter ASCII posible
- **ids** — conjunto de IDs de películas donde aparece ese sufijo
```
Nodo raíz
├── 'b' → Nodo
│    └── 'a' → Nodo
│         └── 'r' → Nodo  ← ids = [1, 5, 12]  (películas con "bar...")
│              └── 't' → Nodo
│                   └── ...  ← ids = [1]  (películas con "bart...")
├── 'k' → Nodo
│    └── 'a' → Nodo
│         └── 'n' → ...  ← ids = [1]  (películas con "kan...")
└── ...
```
 
### Algoritmo de inserción
 
```
FUNCIÓN insertarToken(token, idPelicula):
    longitud = largo(token)
 
    PARA start desde 0 hasta longitud:
        nodo = raiz
        limite = min(longitud, start + 25)   // máximo 25 chars por sufijo
        PARA i desde start hasta limite:
            letra = token[i]
            SI nodo no tiene hijo con esa letra:
                crear nodo hijo vacío
            nodo = nodo.children[letra]
            nodo.ids.agregar(idPelicula)     // marca esta película en cada nodo
```
 
El límite de 25 caracteres por sufijo es un balance entre velocidad y uso de memoria — sufijos muy largos raramente aparecen en búsquedas reales.
 
### Algoritmo de búsqueda por subcadena
 
```
FUNCIÓN buscar(textoBusqueda):
    palabrasBuscadas = limpiarCampo(textoBusqueda)
    resultados = conjunto vacío
 
    PARA cada palabra en palabrasBuscadas:
        nodo = raiz del arbol
 
        PARA cada letra en palabra:
            SI nodo tiene hijo con esa letra:
                nodo = ese hijo
            SI NO:
                no hay coincidencias → pasar a siguiente palabra
                break
 
        recolectar todos los ids del nodo actual
        agregar a resultados
 
    retornar resultados
```
 
### Algoritmo de búsqueda por frase
 
Cuando el usuario busca una frase de varias palabras, se busca cada palabra por separado y se toma la **unión** de todas las películas encontradas. Las películas que aparecen en más palabras buscadas obtienen mayor puntaje de relevancia.
 
```
FUNCIÓN buscarFrase("barco fantasma"):
    palabras = ["barco", "fantasma"]
    resultado = buscar("barco") ∪ buscar("fantasma")
    ordenar resultado por relevancia
    retornar resultado
```
 
### Ejemplo visual de búsqueda
 
```
Usuario busca: "bar"
 
↓ limpiarCampo()  →  ["bar"]
 
↓ recorrer el árbol letra por letra
raiz → 'b' → 'a' → 'r'  ✅ nodo encontrado
 
↓ recolectar ids
ids = [1, 5, 12]
 
↓ traducir a películas
#1  → "Kansas Saloon Smashers"  (contiene "bartender")
#5  → "The Pirate Ship"         (contiene "harbor")
#12 → "The Barber Shop"         (contiene "bar")
 
✅ 3 películas encontradas
```
 
### Ranking de resultados
 
Los resultados se ordenan por relevancia. Cada aparición de la búsqueda en distintos campos suma puntos distintos:
 
```
Aparición en título    →  +3 puntos
Aparición en director  →  +2 puntos
Aparición en plot      →  +1 punto
```
 
---
 
## 🖥️ 4. Avance de la interfaz
 
El programa corre en consola con un menú principal de navegación:
 
```
==================================================
         PLATAFORMA DE STREAMING
==================================================
1. Buscar pelicula por palabras
2. Buscar por director
3. Buscar por genero
4. Buscar por actor
5. Ver 'Ver mas tarde'
6. Ver recomendaciones
7. Salir
Opcion: 1
```
 
**Búsqueda y resultados:**
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
 
**Detalle de película:**
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
 
### Funcionalidades implementadas
 
- **Búsqueda por palabras o frases** — encuentra películas por cualquier subcadena en título o sinopsis
- **Búsqueda por tag** — busca exactamente por director, género o actor sin confundir con el plot
- **Paginación** — muestra 5 resultados a la vez, con opción de ver más
- **Ranking por relevancia** — título vale más que plot, director vale más que sinopsis
- **Likes** — marca películas favoritas
- **Ver más tarde** — guarda películas para ver después
- **Recomendaciones** — sugiere películas similares a las que diste like, basado en palabras en común
 
