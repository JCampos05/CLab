#include "RecorrerArchivo.h"
#include <fstream>
#include <iostream>
using namespace std;

bool RecorrerArchivo(const string& archivoProcesado) {
    ifstream input(archivoProcesado);

    // Validar si el archivo existe
    if (!input.is_open()) {
        cerr << "Error: No se pudo abrir el archivo '" << archivoProcesado << "'. El archivo no existe o no se puede leer." << endl;
        return false;
    }

    cout << "Contenido del archivo '" << archivoProcesado << "':" << endl;
    cout << "----------------------------------------" << endl;
    
    char c;
    // Recorrer el archivo carácter por carácter
    while (input.get(c)) {
        cout << c;
    }

    cout << endl << "----------------------------------------" << endl;
    cout << "Fin del archivo." << endl;
    
    // Cerrar el archivo
    input.close();

    return true;
}