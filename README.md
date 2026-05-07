# Plataforma de Streaming – Proyecto Parte 1
Programación III (2026‑1)

**Integrantes**
* Collazos Solis, Maxwell Lupo Gregorio
* Heredia Cadenas Guillermo Arturo
* Noreña Paredes, Steven Daniel
* Vizcardo Chavez, Juan Diego
* Aquino Pachas, Walter Sebastian

---

## Descripción del proyecto
 
Se implementa una plataforma de streaming que permite buscar peliculas por palabras, frases o subcadenas en el titulo y la sinopsis, así como por tags (director, cast, genero, etc.). El sistema muestra las mejores coincidencias ordenadas por relevancia, permite navegar entre paginas de resultados, visualizar la sinopsis completa y marcar "Like" o "Ver mas tarde".
 
---

## 1. Pre-procesamiento de datos
 
### Objetivo
Convertir el texto crudo del CSV en palabras limpias y uniformes, listas para ser ingresadas al Suffix Trie.
 
### Campos procesados
Se procesan todos los campos de cada película: titulo, origen, director, cast, genero y plot. Todos se unen en un solo texto y pasan por el mismo proceso de limpieza.
 
### Pasos de limpieza
 
**Paso 1 — Convertir a minusculas**
 
Todo el texto se convierte a minusculas para que la busqueda no distinga entre mayusculas y minusculas. Sin esto, "Kansas" y "kansas" serían nodos distintos en el arbol.
 
```
"Kansas Saloon Smashers"  ->  "kansas saloon smashers"
```
 
**Paso 2 — Eliminar puntuación y caracteres especiales**
 
Se recorre el texto caracter por caracter. Todo lo que no sea letra o número se reemplaza por un espacio. También se eliminan referencias como [1] que aparecen en los plots, y caracteres especiales como el guión largo.
 
```
"bartender, serving drinks.[1]"  ->  "bartender  serving drinks "
"presidents—abraham"             ->  "presidents abraham"
```
 
**Paso 3 — Separar en palabras**
 
El texto se divide en palabras individuales usando los espacios como separador. Los espacios dobles del paso anterior se ignoran automaticamente. Cada palabra resultante es un token que se insertara en el Suffix Trie.
 
```
"bartender  serving drinks "  ->  ["bartender", "serving", "drinks"]
```
 
**Paso 4 — Eliminar stopwords**
 
Se eliminan palabras que aparecen en casi todas las películas y no aportan valor a la busqueda: artículos, preposiciones, conjunciones, etc.
 
```
["the", "bartender", "is", "serving", "drinks", "a", "customers"]
                 eliminar stopwords
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
 
Paso 1: minusculas
"the martyred presidents—abraham lincoln, james a. garfield[1]"
 
Paso 2: eliminar puntuacion y caracteres especiales
"the martyred presidents abraham lincoln  james a  garfield "
 
Paso 3: separar en palabras
["the", "martyred", "presidents", "abraham", "lincoln", "james", "a", "garfield"]
 
Paso 4: eliminar stopwords  (se eliminan "the" y "a")
["martyred", "presidents", "abraham", "lincoln", "james", "garfield"]
 
Resultado: tokens listos para insertar en el Suffix Trie
```
 
### Codigo del pre-procesamiento
 
```cpp
// Paso 1: minusculas
string aMinusculas(string texto) {
    transform(texto.begin(), texto.end(), texto.begin(), ::tolower);
    return texto;
}
 
// Paso 2: eliminar puntuacion, referencias [1] y caracteres especiales
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
    vector<string> palabras = separarEnPalabras(eliminarPuntuacion(aMinusculas(texto)));
    vector<string> filtradas;
    for (const string& p : palabras)
        if (stopwords.find(p) == stopwords.end())
            filtradas.push_back(p);
    return filtradas;
}
```
 
---
 
## 2. Pseudo-codigo de inserción al arbol
 
Una vez que los tokens están limpios, se insertan en el Suffix Trie. Por cada película se generan todos los sufijos de cada token y se recorre el arbol letra por letra, creando nodos donde no existen.
 
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
 
---
 
## 3. Eleccion de la estructura de datos: **Suffix Trie**
### Justificacion
- La busqueda debe soportar **cualquier subcadena** (palabra completa, fragmento de palabra o frase).
- Un **suffix tree** (o su variante simple, un **suffix trie**) permite encontrar todas las ocurrencias de un patron en tiempo proporcional a la longitud del patron, una vez construido el índice.
- A diferencia de un Trie normal de palabras, un suffix tree almacena todos los sufijos del texto concatenado, por lo que responde de forma natural a consultas como `"bar"` (que encuentra *barco*, *embarcación*, etc.).
- La implementacion puede mantenerse con **sintaxis basica de C++** (arreglos de punteros, vectores) para cumplir con el requisito de simplicidad.

### Construccion del índice generalizado
1. Se pre‑procesa el archivo CSV y se genera un texto limpio por pelicula (titulo + sinopsis + tags, sin stopwords).
2. Se concatena el texto de todas las peliculas, separando cada pelicula con un carácter unico no imprimible (e.g., `'$'`).
3. Se insertan **todos los sufijos** de cada película en un **Suffix Trie**. Cada nodo almacena una lista de IDs de las peliculas en las que aparece ese sufijo.
4. Durante la insercion se mapea cada sufijo a su ID de película (duplicando la referencia en nodos compartidos).

### Búsqueda de patrones
- **Subcadena suelta**: se recorre el trie con los caracteres del patron. Si se llega a un nodo, se devuelven todos los IDs de pelicula almacenados en el y en sus descendientes (todos los sufijos que empiezan con el patron).
- **Frase (varias palabras)**: se divide la frase en palabras, se busca cada palabra por separado y se realiza la **interseccion** de los conjuntos de peliculas resultantes.
- **Tag**: los campos `director`, `cast`, `género`, etc., se concatenan con un prefijo fijo (ej. `director:spielberg`). Al buscar por tag se busca la cadena `"director:" + nombre` y se recuperan las películas que contengan ese prefijo exacto. Esto evita coincidencias en otros campos.

---

### Algoritmo de insercion
 
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
 
### Ejemplo visual de busqueda
 
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
 
El programa corre en consola con un menu principal:
 
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
 
Busqueda y resultados:
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
 
Detalle de pelicula:
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
