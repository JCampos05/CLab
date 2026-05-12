#pragma once
#include "token.h"
#include "nodo.h"
using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  Estructuras internas de busqueda (tabla de funciones y stack de scopes)
//  Se mantienen separadas de la tabla de simbolos unificada para que el
//  analizador pueda resolver nombres y tipos durante el recorrido del AST.
// ─────────────────────────────────────────────────────────────────────────────

struct NodoParam {
    string     nombre;
    string     tipo;
    NodoParam* siguiente;
};

struct NodoFuncion {
    string       nombre;
    string       tipoRetorno;
    int          aridad;
    NodoParam*   params;
    int          linea;
    NodoFuncion* siguiente;
};

struct TablaFunciones {
    NodoFuncion* cabeza;
    int          tamanio;
};

struct NodoVariable {
    string        nombre;
    string        tipo;
    int           linea;
    NodoVariable* siguiente;
};

struct NodoScope {
    NodoVariable* vars;
    NodoScope*    padre;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Tabla de simbolos unificada  —  salida visible al usuario (opcion 10)
//  Registra en orden de aparicion todos los simbolos del programa:
//  funciones, parametros y variables (locales y globales).
//
//  Campos:
//    id        — numero secuencial de entrada (1, 2, 3 …)
//    nombre    — lexema del identificador
//    tipo      — tipo de dato declarado ("entero", "deci", "vacio", …)
//    valor     — valor literal del inicializador, firma de parametros, o "-"
//    ambito    — "global" o nombre de la funcion contenedora
//    linea     — linea de declaracion en el fuente
//    columna   — columna de declaracion en el fuente
//    categoria — "funcion" | "parametro" | "variable"
// ─────────────────────────────────────────────────────────────────────────────

struct EntradaTabla {
    int           id;
    string        nombre;
    string        tipo;
    string        valor;
    string        ambito;
    int           linea;
    int           columna;
    string        categoria;
    EntradaTabla* siguiente;
};

struct TablaSimbolos {
    EntradaTabla* cabeza;
    int           tamanio;
    int           contadorId;
};

// ─────────────────────────────────────────────────────────────────────────────
//  EstadoSemantico — estado completo del analizador semantico.
//  Recibe el NodoAST* raiz producido por el parser y lo recorre.
//  No libera el AST; esa responsabilidad pertenece al parser.
// ─────────────────────────────────────────────────────────────────────────────

struct EstadoSemantico {
    NodoAST*       raiz;                // AST de entrada (NO liberar aqui)
    ListaErrores   errores;             // misma infraestructura que lexer/parser
    bool           hayError;

    TablaFunciones funciones;           // busqueda interna de funciones (analisis)
    TablaSimbolos  tabla;               // tabla de simbolos unificada (visualizacion)

    NodoScope*     scopeActual;         // tope del stack de scopes de variables
    string         tipoRetFunActual;    // tipo de retorno de la funcion en analisis
    string         ambitoActual;        // nombre de la funcion activa, o "global"
};

// ─────────────────────────────────────────────────────────────────────────────
//  Interfaz publica
// ─────────────────────────────────────────────────────────────────────────────

// Vincula el analizador al AST producido por el parser.
// Debe llamarse antes de semantico_analizar.
void semantico_init             (EstadoSemantico& s, NodoAST* raiz);

// Recorre el AST verificando: declaraciones, tipos, aridades y coherencia de retorno.
// Anota cada nodo de expresion con su tipo calculado (tipoAtrib en NodoAST).
// Construye la tabla de simbolos unificada (s.tabla).
void semantico_analizar         (EstadoSemantico& s);

// Libera la tabla de funciones, la tabla de simbolos y la lista de errores.
// NO libera el AST (responsabilidad del parser).
void semantico_liberar          (EstadoSemantico& s);

// Imprime el resumen del analisis semantico (errores o mensaje de exito).
void semantico_imprimirResultado(const EstadoSemantico& s);

// Imprime la tabla de simbolos unificada: ID/Nombre/Tipo/Valor/Ambito/Ubicacion.
void semantico_imprimirTabla    (const EstadoSemantico& s);
