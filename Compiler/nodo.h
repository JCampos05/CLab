#pragma once
#include <string>
#include "token.h"
using namespace std;

//  TipoNodo — cada construcción del lenguaje idealC tiene su tipo de nodo
enum TipoNodo {
    // ── Raíz ──────────────────────────────────────────────────────────────────
    NODO_PROGRAMA,          // raíz del árbol: lista de declaraciones globales
    // ── Declaraciones de nivel superior ──────────────────────────────────────
    NODO_FUNC,              // func tipo nombre ( params ) { bloque }
    NODO_PARAM,             // tipo nombre  (un parámetro formal)
    // ── Bloque y sentencias ───────────────────────────────────────────────────
    NODO_BLOQUE,            // { sentencia* }
    NODO_DECL_VAR,          // tipo nombre = expr ;
    NODO_ASIGNACION,        // nombre = expr ;
    NODO_LLAMADA,           // nombre ( args ) ;  ← sentencia standalone
    // ── Estructuras de control ────────────────────────────────────────────────
    NODO_SI,                // si (cond) bloque [sino bloque]
    NODO_PARA,              // para (init ; cond ; inc) bloque
    NODO_MIENTRAS,          // mientras (cond) bloque
    NODO_RETORNAR,          // retornar expr ;
    // ── E/S integradas ────────────────────────────────────────────────────────
    NODO_ESCRIBIR,          // escribir ( args ) ;
    NODO_LEER,              // leer ( id ) ;
    NODO_LIMPPANTALLA,      // limpPantalla () ;
    // ── Expresiones ───────────────────────────────────────────────────────────
    NODO_OP_BINARIO,        // expr OP expr  (aritmético, relacional o lógico)
    NODO_OP_UNARIO,         // OP expr       (no, ++ prefijo, -- prefijo)
    NODO_INC_POST,          // expr ++
    NODO_DEC_POST,          // expr --
    // ── Literales ─────────────────────────────────────────────────────────────
    NODO_LIT_ENTERO,        // 42
    NODO_LIT_DECIMAL,       // 3.14
    NODO_LIT_CADENA,        // "hola"
    NODO_LIT_VERDADERO,     // verdadero
    NODO_LIT_FALSO,         // falso
    // ── Hoja de variable / llamada en expresión ───────────────────────────────
    NODO_IDENTIFICADOR,     // nombre de variable usado como expresión
    NODO_LLAMADA_EXPR,      // nombre ( args ) usado como expresión
    // ── Lista de argumentos ───────────────────────────────────────────────────
    NODO_ARGS,              // nodo contenedor de argumentos reales
    // ── Nodo de error (para recuperación) ────────────────────────────────────
    NODO_ERROR              // marcador cuando el parser no pudo parsear
};

//  NodoAST — nodo del Árbol de Sintaxis Abstracta
//  Convención de hijos según tipo de nodo:
//  NODO_PROGRAMA    izq=primera_decl,  siguiente encadena el resto
//  NODO_FUNC        izq=params,  der=bloque,  valor=nombre, extra=tipo_retorno(str)
//  NODO_PARAM       valor=nombre_param, extra_str=tipo
//  NODO_BLOQUE      izq=primera_sentencia, siguiente encadena el resto
//  NODO_DECL_VAR    izq=expr_init (puede ser null),  valor=nombre, extra_str=tipo
//  NODO_ASIGNACION  izq=expr_valor, valor=nombre_variable
//  NODO_LLAMADA     izq=args,  valor=nombre_funcion
//  NODO_SI          izq=cond,  der=bloque_entonces,  extra=bloque_sino
//  NODO_PARA        izq=init,  der=cond,  extra=bloque
//                   (el incremento va como hijo izq del bloque, al inicio)
//  NODO_MIENTRAS    izq=cond,  der=bloque
//  NODO_RETORNAR    izq=expr
//  NODO_ESCRIBIR    izq=primer_arg, siguiente encadena el resto
//  NODO_LEER        valor=nombre_variable
//  NODO_OP_BINARIO  izq=operando_izq, der=operando_der, valor=operador
//  NODO_OP_UNARIO   izq=operando, valor=operador
//  NODO_INC_POST    izq=operando
//  NODO_DEC_POST    izq=operando
//  NODO_LIT_*       valor=lexema_original
//  NODO_IDENTIFICADOR  valor=nombre
//  NODO_LLAMADA_EXPR   izq=args, valor=nombre
//  NODO_ARGS        izq=primer_arg, siguiente encadena el resto
// 
struct NodoAST {
    TipoNodo tipo;
    string valor;       // lexema o nombre cuando aplica
    string valorTipo;   // tipo de dato como string ("entero","deci"…)
    string tipoAtrib;   // atributo semantico: tipo calculado por el analizador semantico
    int linea;
    int columna;

    NodoAST* izq;         // hijo principal / operando izquierdo
    NodoAST* der;         // segundo hijo / operando derecho
    NodoAST* extra;       // tercer hijo (bloque_sino, paso para, etc.)
    NodoAST* siguiente;   // enlace de hermanos en listas (sentencias, args, params)
};

//  Funciones de gestión de nodos

// Crea un nodo en el heap con todos los punteros a null
NodoAST* nodo_crear (TipoNodo tipo, string valor, int linea, int columna);

// Libera recursivamente todo el subárbol apuntado por raiz
void nodo_liberar (NodoAST* raiz);

// Imprime el árbol completo con sangría para visualización en consola.
// mostrarAtribs=false omite los atributos de tipo calculados por el semantico.
void nodo_imprimir (const NodoAST* nodo, int profundidad = 0, bool mostrarAtribs = true);