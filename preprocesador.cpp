#include "preprocesador.h"
#include <algorithm>

const unordered_set<string> stopwords = {
    "a", "an", "the", "and", "or", "but", "in", "on", "at", "to", "for",
    "of", "with", "by", "from", "is", "are", "was", "were", "be", "been",
    "being", "have", "has", "had", "do", "does", "did", "will", "would",
    "could", "should", "may", "might", "can", "shall", "you", "he", "she",
    "it", "we", "they", "his", "her", "its", "our", "their", "this", "that",
    "these", "those", "not", "no", "nor", "so", "if", "then", "than",
    "too", "just", "about", "also", "very", "s", "t", "ll", "ve", "re"
};

string aMinusculas(string texto) {
    transform(texto.begin(), texto.end(), texto.begin(), ::tolower);
    return texto;
}

string eliminarPuntuacion(string texto) {
    string resultado;
    bool dentroDeCorchete = false;
    for (unsigned char letra : texto) {
        if (letra == '[') {
            dentroDeCorchete = true;
        }
        else if (letra == ']') {
            dentroDeCorchete = false;
        }
        else if (dentroDeCorchete) {
            continue;
        }
        else if (letra >= 128){
            resultado += ' ';
        }
        else if (ispunct(letra)) {
            resultado += ' ';
        }
        else {
            resultado += letra;
        }
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
        }
        else {
            palabra += letra;
        }
    }
    if (!palabra.empty()) {
        palabras.push_back(palabra);
    }
    return palabras;
}

vector<string> limpiarCampo(string texto) {
    vector<string> palabras = separarEnPalabras(eliminarPuntuacion(aMinusculas(texto)));
    vector<string> filtradas;
    for (const string& p : palabras) {
        if (stopwords.find(p) == stopwords.end()) {
            filtradas.push_back(p);
        }
    }
    return filtradas;
}