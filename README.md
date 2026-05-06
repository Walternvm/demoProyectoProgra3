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

El siguiente código mejorado incluye la remoción de stopwords y elimina el límite de 3 películas usado para pruebas.

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
    vector<string> palabras = separarEnPalabras(
                                  eliminarPuntuacion(
                                      aMinusculas(texto)));
    vector<string> filtradas;
    for (const string& p : palabras)
        if (stopwords.find(p) == stopwords.end())
            filtradas.push_back(p);
    return filtradas;
}
