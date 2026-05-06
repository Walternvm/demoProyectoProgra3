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
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
using namespace std;

struct Movie {
    string anho;
    string titulo;
    string origen;
    string director;
    string cast;
    string genero;
    string plot;
    vector<string> palabras;     // texto limpio sin stopwords
};

// ---- Funciones auxiliares ----
string aMinusculas(string texto) {
    transform(texto.begin(), texto.end(), texto.begin(), ::tolower);
    return texto;
}

string eliminarPuntuacion(string texto) {
    string resultado;
    bool dentroDeCorchete = false;
    for (unsigned char letra : texto) {
        if (letra == '[') dentroDeCorchete = true;
        else if (letra == ']') dentroDeCorchete = false;
        else if (dentroDeCorchete) continue;
        else if (letra >= 128) resultado += ' ';
        else if (ispunct(letra)) resultado += ' ';
        else resultado += letra;
    }
    return resultado;
}

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

// Remueve palabras comunes en inglés (stopwords)
const unordered_set<string> stopwords = {
    "a", "an", "the", "and", "or", "but", "in", "on", "at", "to", "for",
    "of", "with", "by", "from", "is", "are", "was", "were", "be", "been",
    "being", "have", "has", "had", "do", "does", "did", "will", "would",
    "could", "should", "may", "might", "can", "shall", "you", "he", "she",
    "it", "we", "they", "his", "her", "its", "our", "their", "this", "that",
    "these", "those", "not", "no", "nor", "so", "if", "then", "than",
    "too", "just", "about", "also", "very", "s", "t", "ll", "ve", "re"
};

vector<string> limpiarCampo(string texto) {
    vector<string> palabras = separarEnPalabras(eliminarPuntuacion(aMinusculas(texto)));
    // Eliminar stopwords
    vector<string> filtradas;
    for (const string& p : palabras) {
        if (stopwords.find(p) == stopwords.end())
            filtradas.push_back(p);
    }
    return filtradas;
}

int main() {
    ifstream archivo("data_movies.csv");
    if (!archivo.is_open()) {
        cout << "No se pudo abrir el archivo" << endl;
        return 1;
    }

    char letra;
    bool enComillas = false;
    bool esPrimeraLinea = true;
    string campoActual;
    vector<string> fila;
    vector<Movie> movies;

    while (archivo.get(letra)) {
        if (letra == '"') {
            enComillas = !enComillas;
        }
        else if (letra == ',' && !enComillas) {
            fila.push_back(campoActual);
            campoActual = "";
        }
        else if (letra == '\n') {
            fila.push_back(campoActual);
            campoActual = "";

            if (esPrimeraLinea) {
                esPrimeraLinea = false;
                fila.clear();
                continue;
            }

            if (fila.size() >= 7) {
                Movie m;
                m.anho     = fila[0];
                m.titulo   = fila[1];
                m.origen   = fila[2];
                m.director = fila[3];
                m.cast     = fila[4];
                m.genero   = fila[5];
                m.plot     = fila[6];

                string todoElTexto = m.titulo   + " " + m.origen   + " " +
                                     m.director + " " + m.cast     + " " +
                                     m.genero   + " " + m.plot;
                m.palabras = limpiarCampo(todoElTexto);
                movies.push_back(m);
            }
            fila.clear();
            // no hay límite: se lee todo el archivo
        }
        else if (letra != '\r') {
            campoActual += letra;
        }
    }
    archivo.close();

    // A partir de aquí se construye el suffix tree y se inicia la interfaz...
    cout << "Películas cargadas: " << movies.size() << endl;
    return 0;
}
