#include "PreprocesarArchivo.h"
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

// funcion principal
bool PreprocesarArchivo(const string& archivo1, const string& archivoProcesado) {
    ifstream input(archivo1);

    // validar si el archivo existe
    if (!input.is_open()) {
        cerr << "Error: No se pudo abrir el archivo '" << archivo1 << "'. El archivo no existe o no se puede leer." << endl;
        return false;
    }

    ofstream output(archivoProcesado);

    if (!output.is_open()) {
        cerr << "Error: No se pudo crear el archivo de salida '" << archivoProcesado << "'." << endl;
        input.close();
        return false;
    }

    string linea;
    bool dentroComentario = false;

    // Recorrer el archivo línea por línea
    while (getline(input, linea)) {
        string lineaProcesada = "";
        bool dentroComentarioLinea = false;
        
        for (size_t i = 0; i < linea.length(); i++) {
            char c = linea[i];
            
            // Manejar comentarios multilínea /* */
            if (!dentroComentarioLinea && i + 1 < linea.length()) {
                if (c == '/' && linea[i + 1] == '*') {
                    dentroComentario = true;
                    i++; // Saltar el siguiente carácter
                    continue;
                }
            }
            
            if (dentroComentario) {
                if (i + 1 < linea.length() && c == '*' && linea[i + 1] == '/') {
                    dentroComentario = false;
                    i++; // Saltar el siguiente carácter
                }
                continue;
            }
            
            // Manejar comentarios de una línea //
            if (!dentroComentarioLinea && i + 1 < linea.length()) {
                if (c == '/' && linea[i + 1] == '/') {
                    dentroComentarioLinea = true;
                    break; // Saltar el resto de la línea
                }
            }
            
            // Eliminar espacios en blanco y tabuladores
            if (c == ' ' || c == '\t') {
                continue;
            }
            
            // Agregar el carácter a la línea procesada
            lineaProcesada += c;
        }
        
        // Solo escribir la línea si no está vacía
        if (!lineaProcesada.empty()) {
            output << lineaProcesada << endl;
        }
    }
    
    input.close();
    output.close();
    cout << endl;
    cout << "Archivo preprocesado exitosamente. Resultado guardado en: " << archivoProcesado << endl;
    cout << endl;

    return true;
}