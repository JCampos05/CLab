#include <iostream>
#include <fstream>
#include <limits>
#include "RecorrerArchivo.h"
#include "PreprocesarArchivo.h"
using namespace std;

void error(){
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    system("cls");
    cout << endl;
    cout << "Ingrese un dato válido" << endl;
    system("pause");
    system("cls");
}

void mainProcess(){
    string archivo1 = "archivo1.txt", archivoProcesado = "archivo2.txt";

    cout << "Fase 1. Preprocesar archivo1.txt" << endl;
    
    if (PreprocesarArchivo(archivo1, archivoProcesado)){
        cout << "Fase 2. Recorriendo y procesando el archivo" << endl;
        RecorrerArchivo(archivoProcesado);
    } else {
        cout << "Ha ocurrido un error en el preprocesamiento del archivo" << endl;
    }
    
}

int main(int argc, char const *argv[]) {
    cout << "Bienvenido..." << endl;
    cout << endl;
    mainProcess();
    system("pause");
    return 0;
}