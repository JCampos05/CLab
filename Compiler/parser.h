#pragma once
#include "token.h"
#include "nodo.h"
using namespace std;

//  EstadoParser — estado completo del analizador sintáctico
//  Recibe la ListaTokens ya construida por el léxico y la recorre con un
//  puntero. No re-lee el archivo fuente; todo el trabajo está en los tokens.
struct EstadoParser {
    NodoToken* actual;      // puntero al token que se está inspeccionando
    ListaErrores errores;     // lista enlazada de errores sintácticos
    NodoAST* raiz;        // raíz del AST generado (null si falla)
    bool hayError;    // true en cuanto se registra algún error
};

//  Interfaz pública — 3 funciones, igual que el léxico 

// Vincula el parser a una lista de tokens ya construida por el léxico.
// Debe llamarse antes de parser_analizar.
void parser_init (EstadoParser& p, const ListaTokens& tokens);

// Recorre la lista de tokens aplicando las reglas de la gramática de idealC.
// Construye el AST en p.raiz y registra errores en p.errores.
// Usa recuperación por pánico: ante un error avanza hasta ';' o '}' y continúa.
void parser_analizar (EstadoParser& p);

// Libera el AST y la lista de errores del parser.
void parser_liberar (EstadoParser& p);

// Imprime el resumen del análisis sintáctico (errores o mensaje de éxito).
void parser_imprimirResultado(const EstadoParser& p);