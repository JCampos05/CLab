#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "nodo.h"
#include "semantico.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <limits>
using namespace std;

//  Compilar: g++ main.cpp lexer.cpp toke.cpp nodo.cpp parser.cpp semantico.cpp -o main

EstadoLexer    g_lex;
EstadoParser   g_parser;
EstadoSemantico g_sem;
bool g_analizado   = false;
bool g_parseado    = false;
bool g_semanticado = false;

void cabecera() {
    cout << "+-----------------------------------------+" << endl;
    cout << "|---| Analizador Lexico  -  idealC   |----|" << endl;
    cout << "+-----------------------------------------+" << endl;
}

void entradaInvalida() {
    cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
    system("cls"); cout << endl << "  Ingrese un dato valido." << endl;
    system("pause"); system("cls");
}

bool cargarProgram() {
    ifstream archivo("Program.txt");
    if (!archivo.is_open()) { cout << "  Error: No se encontro 'Program.txt'." << endl; system("pause"); return false; }
    ostringstream ss; ss << archivo.rdbuf(); archivo.close();
    if (g_analizado)   lexer_liberar(g_lex);
    if (g_parseado)  { parser_liberar(g_parser);   g_parseado    = false; }
    if (g_semanticado) { semantico_liberar(g_sem); g_semanticado = false; }
    lexer_init(g_lex, ss.str(), "Program.txt"); lexer_analizar(g_lex); g_analizado = true; return true;
}

bool verificarAnalisis() {
    if (!g_analizado) { cout << endl << "  Use la opcion 1 para cargar Program.txt primero." << endl; system("pause"); return false; }
    return true;
}

bool verificarParseado() {
    if (!g_parseado) { cout << endl << "  Use la opcion 5 para ejecutar el analisis sintactico primero." << endl; system("pause"); return false; }
    return true;
}

bool verificarSemanticado() {
    if (!g_semanticado) { cout << endl << "  Use la opcion 8 para ejecutar el analisis semantico primero." << endl; system("pause"); return false; }
    return true;
}

void opcion_cargar() {
    system("cls"); cabecera(); cout << endl;
    cout << "+--------------------------------------------------+" << endl;
    cout << "|       Analizando archivo: Program.txt            |" << endl;
    cout << "+--------------------------------------------------+" << endl << endl;
    if (!cargarProgram()) return;
    cout << "  Tokens encontrados : " << g_lex.tokens.tamanio << endl;
    cout << "  Errores encontrados: " << g_lex.errores.tamanio << endl;
    system("pause");
}

void opcion_tokens() {
    system("cls"); cabecera(); if (!verificarAnalisis()) return;
    cout << endl << "  Fuente: " << g_lex.archivo << endl;
    listaTokens_imprimir(g_lex.tokens); system("pause");
}

void opcion_errores() {
    system("cls"); cabecera(); if (!verificarAnalisis()) return;
    cout << endl << "  Fuente: " << g_lex.archivo << endl;
    if (listaErrores_hayErrores(g_lex.errores)) { listaErrores_imprimir(g_lex.errores); }
    else { cout << "|  Sin errores lexicos en el codigo analizado.  |" << endl; }
    system("pause");
}

void opcion_resumen() {
    system("cls"); cabecera(); if (!verificarAnalisis()) return;
    int kw=0,op=0,lit=0,id=0,del=0,err=0;
    NodoToken* n = g_lex.tokens.cabeza;
    while(n){ TokenType t=n->token.tipo;
        if(t>=PAL_RSV_ENTERO&&t<=PAL_RSV_NO) kw++;
        else if(t>=OP_SUMA&&t<=OP_ASIGNACION) op++;
        else if(t>=LIT_ENTERO&&t<=LIT_CADENA) lit++;
        else if(t==IDENTIFICADOR) id++;
        else if(t>=PUNTO_COMA&&t<=LLAVE_CIE) del++;
        else if(t==TOKEN_ERROR) err++;
        n=n->siguiente; }
    cout << endl;
    cout << "+-------------------------------------------------------+" << endl;
    cout << "|              RESUMEN DEL ANALISIS LEXICO              |" << endl;
    cout << "+-------------------------------------------------------+" << endl;
    cout << "|  Fuente   : " << left << setw(43) << g_lex.archivo << "|" << endl;
    cout << "+-------------------------------------------------------+" << endl;
    cout << "|  Total tokens reconocidos    : " << left << setw(24) << g_lex.tokens.tamanio  << "|" << endl;
    cout << "|  Total errores lexicos       : " << left << setw(24) << g_lex.errores.tamanio << "|" << endl;
    cout << "+-------------------------------------------------------+" << endl;
    cout << "|  Palabras reservadas  (PR)   : " << left << setw(24) << kw  << "|" << endl;
    cout << "|  Operadores           (OP)   : " << left << setw(24) << op  << "|" << endl;
    cout << "|  Literales            (LIT)  : " << left << setw(24) << lit << "|" << endl;
    cout << "|  Identificadores             : " << left << setw(24) << id  << "|" << endl;
    cout << "|  Delimitadores               : " << left << setw(24) << del << "|" << endl;
    cout << "|  Tokens de error             : " << left << setw(24) << err << "|" << endl;
    cout << "+-------------------------------------------------------+" << endl;
    cout << (g_lex.errores.tamanio==0 ? "|  Estado : CODIGO VALIDO  - sin errores lexicos.       |" : "|  Estado : CODIGO CON ERRORES - revise la opcion 3.    |") << endl;
    cout << "+-------------------------------------------------------+" << endl;
    system("pause");
}

void opcion_sintactico() {
    system("cls"); cabecera(); if (!verificarAnalisis()) return;
    if (listaErrores_hayErrores(g_lex.errores)) {
        cout << endl << "+-------------------------------------------------------------+" << endl;
        cout << "|  ADVERTENCIA: el lexico reporto errores en Program.txt.     |" << endl;
        cout << "|  El analisis sintactico puede ser impreciso.                |" << endl;
        cout << "+-------------------------------------------------------------+" << endl << endl; }
    if (g_parseado) { parser_liberar(g_parser); g_parseado = false; }
    cout << endl << "+--------------------------------------------------+" << endl;
    cout << "|     Ejecutando analisis sintactico...            |" << endl;
    cout << "+--------------------------------------------------+" << endl << endl;
    parser_init(g_parser, g_lex.tokens); parser_analizar(g_parser); g_parseado = true;
    parser_imprimirResultado(g_parser);
    cout << "  Errores sintacticos encontrados: " << g_parser.errores.tamanio << endl;
    system("pause");
}

void opcion_errores_sint() {
    system("cls"); cabecera(); if (!verificarParseado()) return;
    cout << endl << "  Fuente: " << g_lex.archivo << endl;
    if (listaErrores_hayErrores(g_parser.errores)) { listaErrores_imprimir(g_parser.errores); }
    else { cout << "+---------------------------------------------------+" << endl;
           cout << "|  Sin errores sintacticos en el codigo analizado.  |" << endl;
           cout << "+---------------------------------------------------+" << endl; }
    system("pause");
}

void opcion_ast() {
    system("cls"); cabecera(); if (!verificarParseado()) return;
    if (!g_parser.raiz) { cout << endl << "  El arbol AST esta vacio." << endl; system("pause"); return; }
    cout << endl << "  Fuente: " << g_lex.archivo << endl;
    if (listaErrores_hayErrores(g_parser.errores))
        cout << "  Nota: arbol puede estar incompleto (" << g_parser.errores.tamanio << " error(es))." << endl;
    nodo_imprimir(g_parser.raiz, 0, false); system("pause");
}

void opcion_ast_sem() {
    system("cls"); cabecera(); if (!verificarSemanticado()) return;
    if (!g_parser.raiz) { cout << endl << "  El arbol AST esta vacio." << endl; system("pause"); return; }
    cout << endl << "  Fuente: " << g_lex.archivo << endl;
    if (listaErrores_hayErrores(g_sem.errores))
        cout << "  Nota: se encontraron " << g_sem.errores.tamanio << " error(es) semantico(s)." << endl;
    nodo_imprimir(g_parser.raiz, 0, true); system("pause");
}

void opcion_semantico() {
    system("cls"); cabecera(); if (!verificarParseado()) return;
    if (listaErrores_hayErrores(g_parser.errores)) {
        cout << endl << "+------------------------------------------------------------+" << endl;
        cout << "|  ADVERTENCIA: el parser reporto errores en Program.txt.    |" << endl;
        cout << "|  El analisis semantico puede ser impreciso o incompleto.   |" << endl;
        cout << "+------------------------------------------------------------+" << endl << endl; }
    if (g_semanticado) { semantico_liberar(g_sem); g_semanticado = false; }
    cout << endl << "+--------------------------------------------------+" << endl;
    cout << "|     Ejecutando analisis semantico...             |" << endl;
    cout << "+--------------------------------------------------+" << endl << endl;
    semantico_init(g_sem, g_parser.raiz);
    semantico_analizar(g_sem);
    g_semanticado = true;
    semantico_imprimirResultado(g_sem);
    cout << "  Errores semanticos encontrados: " << g_sem.errores.tamanio << endl;
    system("pause");
}

void opcion_errores_sem() {
    system("cls"); cabecera(); if (!verificarSemanticado()) return;
    cout << endl << "  Fuente: " << g_lex.archivo << endl;
    if (listaErrores_hayErrores(g_sem.errores)) { listaErrores_imprimir(g_sem.errores); }
    else { cout << "+---------------------------------------------------+" << endl;
           cout << "|  Sin errores semanticos en el codigo analizado.   |" << endl;
           cout << "+---------------------------------------------------+" << endl; }
    system("pause");
}

void opcion_tabla_simbolos() {
    system("cls"); cabecera(); if (!verificarSemanticado()) return;
    cout << endl << "  Fuente: " << g_lex.archivo << endl;
    semantico_imprimirTabla(g_sem);
    system("pause");
}

void interfaz() {
    int opc=0, seguir=1; system("cls");
    do {
        do {
            system("cls"); cabecera();
            cout << endl << "  Seleccione una opcion:" << endl << endl;
            cout << "+---------------------------------------------------------------------+" << endl;
            if (g_analizado) {
                cout << "|  Fuente activa : " << left << setw(52) << g_lex.archivo         << "|" << endl;
                cout << "|  Tokens        : " << left << setw(52) << g_lex.tokens.tamanio  << "|" << endl;
                cout << "|  Err. lexicos  : " << left << setw(52) << g_lex.errores.tamanio << "|" << endl;
                if (g_parseado) {
                    string st=(g_parser.errores.tamanio==0)?"OK - sin errores sintacticos":to_string(g_parser.errores.tamanio)+" error(es) sintactico(s)";
                    cout << "|  Sint.         : " << left << setw(52) << st << "|" << endl;
                } else { cout << "|  Sint.         : " << left << setw(52) << "Sin analizar (use opcion 5)" << "|" << endl; }
                if (g_semanticado) {
                    string ss2=(g_sem.errores.tamanio==0)?"OK - sin errores semanticos":to_string(g_sem.errores.tamanio)+" error(es) semantico(s)";
                    cout << "|  Sem.          : " << left << setw(52) << ss2 << "|" << endl;
                } else { cout << "|  Sem.          : " << left << setw(52) << "Sin analizar (use opcion 8)" << "|" << endl; }
            } else { cout << "|  Sin analisis cargado. Use la opcion 1 para cargar Program.txt.  |" << endl; }
            cout << "+---------------------------------------------------------------------+" << endl;
            cout << "|  1) Cargar y analizar Program.txt  (lexico)                         |" << endl;
            cout << "|  2) Ver lista de Tokens reconocidos                                 |" << endl;
            cout << "|  3) Ver lista de Errores lexicos con posicion                       |" << endl;
            cout << "|  4) Ver resumen del analisis lexico                                 |" << endl;
            cout << "|  5) Ejecutar analisis sintactico                                    |" << endl;
            cout << "|  6) Ver Errores sintacticos                                         |" << endl;
            cout << "|  7) Ver AST  (solo estructura sintactica)                           |" << endl;
            cout << "|  8) Ejecutar analisis semantico                                     |" << endl;
            cout << "|  9) Ver Errores semanticos                                          |" << endl;
            cout << "| 10) Ver Tabla de simbolos                                           |" << endl;
            cout << "| 11) Ver AST  con atributos semanticos de tipo {T}                  |" << endl;
            cout << "| 12) Salir                                                           |" << endl;
            cout << "+---------------------------------------------------------------------+" << endl;
            cout << endl << "  Opcion: "; cin >> opc; cout << endl;
            if (cin.fail()) { entradaInvalida(); continue; }
            switch(opc) {
                case 1:  opcion_cargar();          break;
                case 2:  opcion_tokens();          break;
                case 3:  opcion_errores();         break;
                case 4:  opcion_resumen();         break;
                case 5:  opcion_sintactico();      break;
                case 6:  opcion_errores_sint();    break;
                case 7:  opcion_ast();             break;
                case 8:  opcion_semantico();       break;
                case 9:  opcion_errores_sem();     break;
                case 10: opcion_tabla_simbolos();  break;
                case 11: opcion_ast_sem();         break;
                case 12: break; default: break; }
        } while (opc<=0 || opc>12);
        if (opc==12) break;
        do {
            system("cls"); cabecera();
            cout << endl << "  Desea realizar otra consulta?  1) Si    2) No : ";
            cin >> seguir; cout << endl;
            if (cin.fail()) entradaInvalida();
        } while (cin.fail() || (seguir!=1 && seguir!=2));
        system("cls"); opc=0;
    } while (seguir==1);
}

int main() {
    interfaz();
    if (g_analizado)   lexer_liberar(g_lex);
    if (g_parseado)    parser_liberar(g_parser);
    if (g_semanticado) semantico_liberar(g_sem);
    system("cls");
    cout << endl << "  Hasta la proxima..." << endl;
    cout << "  Pero todo depende de lo que diga el autor..." << endl << endl;
    system("pause"); return 0;
}