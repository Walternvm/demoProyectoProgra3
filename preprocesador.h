#ifndef PREPROCESADOR_H
#define PREPROCESADOR_H
#include <string>
#include <vector>
#include <unordered_set>
using namespace std;

string aMinusculas(string texto);
string eliminarPuntuacion(string texto);
vector<string> separarEnPalabras(string texto);
vector<string> limpiarCampo(string texto);

#endif //PREPROCESADOR_H
