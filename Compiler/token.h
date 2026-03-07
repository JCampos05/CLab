#pragma once
#include <string>
using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  TokenType — todos los tipos de token del lenguaje 
// ─────────────────────────────────────────────────────────────────────────────
enum TokenType {
    // KW — Palabras reservadas: tipos de dato
    PAL_RSV_ENTERO, PAL_RSV_DECI, PAL_RSV_CARAC, PAL_RSV_CAD, PAL_RSV_LOGICO, PAL_RSV_VACIO,
    // KW — Palabras reservadas: valores logicos
    PAL_RSV_VERDADERO, PAL_RSV_FALSO,
    // KW — Palabras reservadas: control de flujo
    PAL_RSV_SI, PAL_RSV_SINO, PAL_RSV_PARA, PAL_RSV_MIENTRAS,
    // KW — Palabras reservadas: declaracion y retorno
    PAL_RSV_FUNC, PAL_RSV_RETORNAR, 
    // KW — Palabras reservadas: funciones de E/S
    PAL_RSV_LEER, PAL_RSV_ESCRIBIR, PAL_RSV_LIMPPANTALLA,
    // KW — Palabras reservadas: aritmetica nativa
    PAL_RSV_SUMA, PAL_RSV_RESTA, PAL_RSV_MUL, PAL_RSV_DIV,
    // KW — Palabras reservadas: operadores logicos
    PAL_RSV_Y, PAL_RSV_O, PAL_RSV_NO,
    // Operadores aritmeticos
    OP_SUMA, OP_RESTA, OP_MUL, OP_DIV, OP_MOD, OP_INC, OP_DEC,
    // Operadores relacionales
    OP_MENOR, OP_MAYOR, OP_MENOR_IGUAL, OP_MAYOR_IGUAL, OP_IGUAL, OP_DISTINTO,
    // Operador de asignacion
    OP_ASIGNACION,
    // Delimitadores
    PUNTO_COMA, COMA, PAREN_ABR, PAREN_CIE, LLAVE_ABR, LLAVE_CIE,
    // Literales
    LIT_ENTERO, LIT_DECIMAL, LIT_CADENA,
    // Identificador definido por el usuario
    IDENTIFICADOR,
    // Especiales
    FIN_ARCHIVO,
    TOKEN_ERROR
};

// Convierte un TokenType a su nombre legible (para impresion y depuracion)
string tokenTypeStr(TokenType t);

// ─────────────────────────────────────────────────────────────────────────────
//  Token — unidad lexica minima del analizador
// ─────────────────────────────────────────────────────────────────────────────
struct Token {
    TokenType tipo;
    string  lexema;    // texto original del codigo fuente
    int linea;
    int columna;
    long long valorEntero;  // valido cuando tipo == LIT_ENTERO
    double valorDecimal; // valido cuando tipo == LIT_DECIMAL
    string valorCadena;  // valido cuando tipo == LIT_CADENA
};

Token  crearToken (TokenType tipo, string lexema, int linea, int columna);
string nombreTipo (Token t);

// ─────────────────────────────────────────────────────────────────────────────
//  Lista enlazada de tokens
// ─────────────────────────────────────────────────────────────────────────────
struct NodoToken {
    Token token;
    NodoToken* siguiente;
};

struct ListaTokens {
    NodoToken* cabeza;
    NodoToken* cola;
    int tamanio;
};

void listaTokens_init    (ListaTokens& lista);
void listaTokens_agregar (ListaTokens& lista, Token t);
void listaTokens_imprimir(const ListaTokens& lista);
void listaTokens_liberar (ListaTokens& lista);

// ─────────────────────────────────────────────────────────────────────────────
//  Lista enlazada de errores lexicos con posicion
// ─────────────────────────────────────────────────────────────────────────────
struct ErrorLexico {
    string mensaje;
    int linea;
    int columna;
    string lexema;
};

struct NodoError {
    ErrorLexico error;
    NodoError* siguiente;
};

struct ListaErrores {
    NodoError* cabeza;
    NodoError* cola;
    int tamanio;
};

void listaErrores_init (ListaErrores& lista);
void listaErrores_agregar (
    ListaErrores& lista, 
    string msg, int linea, 
    int columna, 
    string lexema
);
void listaErrores_imprimir(const ListaErrores& lista);
void listaErrores_liberar (ListaErrores& lista);
bool listaErrores_hayErrores(const ListaErrores& lista);