#include "lexer.h"
#include "token.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <limits>
using namespace std;

//  Compilar:
//  g++ main.cpp lexer.cpp toke.cpp -o main
//  Estado global de la sesion
EstadoLexer g_lex;
bool g_analizado = false;

//  Cabecera del sistema
void cabecera() {
    cout << "+-----------------------------------------+" << endl;
    cout << "|---| Analizador Lexico  -  idealC   |----|" << endl;
    cout << "+-----------------------------------------+" << endl;
}

//  Manejo de entrada invalida
void entradaInvalida() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    system("cls");
    cout << endl;
    cout << "  Ingrese un dato valido." << endl;
    system("pause");
    system("cls");
}

//  cargarProgram — lee Program.txt y ejecuta el analisis lexico
bool cargarProgram() {
    ifstream archivo("Program.txt");
    if (!archivo.is_open()) {
        cout << "  Error: No se encontro \'Program.txt\' en el directorio actual." << endl;
        system("pause");
        return false;
    }

    ostringstream ss;
    ss << archivo.rdbuf();
    archivo.close();

    if (g_analizado) {
        lexer_liberar(g_lex);
    }
    lexer_init(g_lex, ss.str(), "Program.txt");
    lexer_analizar(g_lex);
    g_analizado = true;
    return true;
}

//  Opcion 1 — Cargar y analizar Program.txt
void opcion_cargar() {
    system("cls");
    cabecera();
    cout << endl;
    cout << "+--------------------------------------------------+" << endl;
    cout << "|       Analizando archivo: Program.txt            |" << endl;
    cout << "+--------------------------------------------------+" << endl;
    cout << endl;

    if (!cargarProgram()) return;

    cout << "  Archivo analizado correctamente." << endl;
    cout << "  Tokens encontrados : " << g_lex.tokens.tamanio  << endl;
    cout << "  Errores encontrados: " << g_lex.errores.tamanio << endl;
    system("pause");
}
//  Guarda — verifica que haya un analisis cargado antes de mostrar resultados
bool verificarAnalisis() {
    if (!g_analizado) {
        cout << endl;
        cout << "  No hay analisis disponible." << endl;
        cout << "  Use la opcion 1 para cargar Program.txt primero." << endl;
        system("pause");
        return false;
    }
    return true;
}

//  Opcion 2 — Ver lista de tokens reconocidos
void opcion_tokens() {
    system("cls");
    cabecera();
    if (!verificarAnalisis()) return;

    cout << endl;
    cout << "  Fuente: " << g_lex.archivo << endl;
    listaTokens_imprimir(g_lex.tokens);
    system("pause");
}

//  Opcion 3 — Ver lista de errores lexicos con posicion
void opcion_errores() {
    system("cls");
    cabecera();
    if (!verificarAnalisis()) return;

    cout << endl;
    cout << "  Fuente: " << g_lex.archivo << endl;

    if (listaErrores_hayErrores(g_lex.errores)) {
        listaErrores_imprimir(g_lex.errores);
    } else {
        cout << "+-----------------------------------------------+" << endl;
        cout << "|         LISTA DE ERRORES LEXICOS              |" << endl;
        cout << "+-----------------------------------------------+" << endl;
        cout << "|  Sin errores lexicos en el codigo analizado.  |" << endl;
        cout << "+-----------------------------------------------+" << endl;
    }
    system("pause");
}

//  Opcion 4 — Resumen del analisis
void opcion_resumen() {
    system("cls");
    cabecera();
    if (!verificarAnalisis()) return;

    int kwCount    = 0;
    int opCount    = 0;
    int litCount   = 0;
    int idCount    = 0;
    int delimCount = 0;
    int errCount   = 0;

    NodoToken* nodo = g_lex.tokens.cabeza;
    while (nodo != nullptr) {
        TokenType t = nodo->token.tipo;
        if      (t >= PAL_RSV_ENTERO  && t <= PAL_RSV_NO)         kwCount++;
        else if (t >= OP_SUMA    && t <= OP_ASIGNACION)  opCount++;
        else if (t >= LIT_ENTERO && t <= LIT_CADENA)     litCount++;
        else if (t == IDENTIFICADOR)                     idCount++;
        else if (t >= PUNTO_COMA && t <= LLAVE_CIE)      delimCount++;
        else if (t == TOKEN_ERROR)                       errCount++;
        nodo = nodo->siguiente;
    }

    cout << endl;
    cout << "+-------------------------------------------------------+" << endl;
    cout << "|              RESUMEN DEL ANALISIS LEXICO              |" << endl;
    cout << "+-------------------------------------------------------+" << endl;
    cout << "|  Fuente   : " << left << setw(43) << g_lex.archivo    << "|" << endl;
    cout << "+-------------------------------------------------------+" << endl;
    cout << "|  Total tokens reconocidos    : " << left << setw(24) << g_lex.tokens.tamanio  << "|" << endl;
    cout << "|  Total errores lexicos       : " << left << setw(24) << g_lex.errores.tamanio << "|" << endl;
    cout << "+-------------------------------------------------------+" << endl;
    cout << "|  Palabras reservadas  (KW)   : " << left << setw(24) << kwCount    << "|" << endl;
    cout << "|  Operadores           (OP)   : " << left << setw(24) << opCount    << "|" << endl;
    cout << "|  Literales            (LIT)  : " << left << setw(24) << litCount   << "|" << endl;
    cout << "|  Identificadores             : " << left << setw(24) << idCount    << "|" << endl;
    cout << "|  Delimitadores               : " << left << setw(24) << delimCount << "|" << endl;
    cout << "|  Tokens de error             : " << left << setw(24) << errCount   << "|" << endl;
    cout << "+-------------------------------------------------------+" << endl;

    if (g_lex.errores.tamanio == 0)
        cout << "|  Estado : CODIGO VALIDO  - sin errores lexicos.       |" << endl;
    else
        cout << "|  Estado : CODIGO CON ERRORES - revise la opcion 3.    |" << endl;

    cout << "+-------------------------------------------------------+" << endl;
    system("pause");
}

void interfaz() {
    int opc    = 0;
    int seguir = 1;

    system("cls");

    do {
        do {
            system("cls");
            cabecera();
            cout << endl;
            cout << "  Seleccione una opcion:" << endl;
            cout << endl;

            cout << "+---------------------------------------------------------------------+" << endl;
            if (g_analizado) {
                cout << "|  Fuente activa : " << left << setw(52) << g_lex.archivo            << "|" << endl;
                cout << "|  Tokens        : " << left << setw(52) << g_lex.tokens.tamanio     << "|" << endl;
                cout << "|  Errores       : " << left << setw(52) << g_lex.errores.tamanio    << "|" << endl;
            } else {
                cout << "|  Sin analisis cargado. Use la opcion 1 para cargar Program.txt.  |" << endl;
            }
            cout << "+---------------------------------------------------------------------+" << endl;
            cout << "|  1) Cargar y analizar Program.txt                                   |" << endl;
            cout << "|  2) Ver lista de Tokens reconocidos                                 |" << endl;
            cout << "|  3) Ver lista de Errores lexicos con posicion                       |" << endl;
            cout << "|  4) Ver resumen del analisis                                        |" << endl;
            cout << "|  5) Salir                                                           |" << endl;
            cout << "+---------------------------------------------------------------------+" << endl;
            cout << endl;
            cout << "  Opcion: ";
            cin >> opc;
            cout << endl;

            if (cin.fail()) {
                entradaInvalida();
                continue;
            }

            switch (opc) {
                case 1: opcion_cargar();  break;
                case 2: opcion_tokens();  break;
                case 3: opcion_errores(); break;
                case 4: opcion_resumen(); break;
                case 5: break;
                default: break;
            }

        } while (opc <= 0 || opc > 5);

        if (opc == 5) break;

        do {
            system("cls");
            cabecera();
            cout << endl;
            cout << "  Desea realizar otra consulta?  1) Si    2) No : ";
            cin >> seguir;
            cout << endl;

            if (cin.fail()) {
                entradaInvalida();
            }
        } while (cin.fail() || (seguir != 1 && seguir != 2));

        system("cls");
        opc = 0;

    } while (seguir == 1);
}

int main() {
    interfaz();

    if (g_analizado) {
        lexer_liberar(g_lex);
    }

    system("cls");
    cout << endl;
    cout << "  Hasta la proxima..." << endl;
    cout << "  Pero todo depende de lo que diga el autor..." << endl;
    cout << endl;
    system("pause");
    return 0;
}