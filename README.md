# Plataforma de Streaming – Proyecto Parte 1
Programación III (2026‑1)

**Integrantes**
* Collazos Solis, Maxwell Lupo Gregorio
* Heredia Cadenas Guillermo Arturo
* Noreña Paredes, Steven Daniel
* Vizcardo Chavez, Juan Diego
* Aquino Pachas, Walter Sebastian

---

## Descripcion del proyecto

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

**Paso 2 — Eliminar puntuacion y caracteres especiales**

Se recorre el texto caracter por caracter. Todo lo que no sea letra o numero se reemplaza por un espacio. Tambien se eliminan referencias como [1] que aparecen en los plots, y caracteres especiales como el guion largo.

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

## 2. Pseudo-codigo de insercion al arbol

Una vez que los tokens estan limpios, se insertan en el Suffix Trie. Por cada pelicula se generan todos los sufijos de cada token y se recorre el arbol letra por letra, creando nodos donde no existen.

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

### Ejemplo visual de insercion

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

- La busqueda debe soportar subcadenas, fragmentos de palabras y coincidencias parciales dentro de titulos, generos y sinopsis.
- Un **Suffix Trie** permite buscar patrones recorriendo el arbol caracter por caracter, en tiempo proporcional a la longitud del patron buscado.
- A diferencia de un Trie tradicional de palabras completas, el Suffix Trie almacena todos los sufijos de cada token procesado, permitiendo encontrar coincidencias parciales. Por ejemplo, al buscar `"bar"` se pueden encontrar tokens como `"bartender"` o `"barber"`.
- La estructura permite indexar grandes cantidades de texto y realizar busquedas eficientes sobre un dataset de 34,886 peliculas.
- La implementacion puede mantenerse usando estructuras basicas de C++ como arreglos, punteros y vectores, manteniendo el codigo simple y facil de entender.

### Construccion del indice

1. Se carga el archivo CSV con las peliculas.
2. Cada pelicula pasa por el proceso de limpieza y tokenizacion.
3. Los tokens obtenidos se insertan individualmente en el Suffix Trie.
4. Para cada token se generan todos sus sufijos.
5. Cada nodo almacena los IDs de las peliculas en las que aparece el patron representado por el camino recorrido en el arbol.

### Busqueda de patrones

- **Subcadena suelta**: se recorre el trie siguiendo los caracteres del patron. Si el recorrido existe, el nodo final contiene los IDs de las peliculas donde aparece esa subcadena.
- **Frase (varias palabras)**: la frase se divide en palabras individuales y cada palabra se busca por separado. Luego se realiza la interseccion de resultados para obtener las peliculas que contienen todas las palabras buscadas.
- **Busqueda por tags**: campos como director, cast y genero tambien son tokenizados e insertados en el trie, permitiendo realizar busquedas especificas sobre esos atributos.

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

### Algoritmo de busqueda

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
Usuario busca: "sal"

recorrer el arbol letra por letra:
raiz -> 's' -> 'a' -> 'l'   nodo encontrado

recolectar ids del nodo:
ids = [peliculas que contienen la subcadena "sal"]

algunas coincidencias encontradas:
- "Kansas Saloon Smashers"
- "Salomy Jane"
- "The Mate of the Sally Ann"
- "Salomé"

Total de peliculas encontradas:
132 peliculas
```
### Complejidad (notacion Big-O)

**Insercion:**

Por cada token se generan todos sus sufijos (hasta 25 caracteres cada uno) y se recorre el arbol letra por letra.
- Insertar un token de largo n: $O(n^2)$
- Insertar todas las peliculas: $O(P \times T \times n^2)$
- Donde:
  - P = cantidad de peliculas
  - T = cantidad promedio de tokens por pelicula
  - n = largo promedio de cada token

Este costo se paga una sola vez al construir el indice.

**Busqueda:**

La busqueda recorre el arbol siguiendo los caracteres del patron.

- Recorrer el patron de largo m: $O(m)$
- Recuperar los resultados encontrados: $O(k)$
- Complejidad total de busqueda: $O(m + k)$
- Donde:
  - m = largo del patron buscado
  - k = cantidad de peliculas encontradas

Por ejemplo, buscar `"sal"` requiere recorrer solo 3 nodos del trie, independientemente de si el dataset tiene 100 o 34,886 peliculas.

| Operacion | Complejidad |
|---|---|
| Insertar un token | $O(n^2)$ |
| Buscar un patron | $O(m + k)$ |
| Busqueda lineal sin arbol | $O(P \times largoplot)$ |

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
Ingrese palabra o frase a buscar: great

Buscando: "great"...
Resultados encontrados: 129

--- Resultados 1-5 ---
1. The Great Train Robbery
2. Great Expectations
3. The Great Love
4. The Greatest Thing in Life
5. The Great Gatsby

Opcion (1-5 para ver pelicula, 6 para mas resultados, 0 para volver): 1
```

Detalle de pelicula:
```
==================================================
Titulo:   The Great Train Robbery (1903)
Director: Edwin S. Porter
Cast:
Genero:   western
Origen:   American
Sinopsis: The film opens with two bandits breaking into a...
==================================================
1. Dar Like
2. Ver mas tarde
3. Volver
Opcion:
```
