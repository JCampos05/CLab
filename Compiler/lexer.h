#pragma once
#include "token.h"
#include <string>
using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  Estado completo del analizador en un momento dado
//  Equivale a los atributos privados que antes estaban en la clase Lexer.
// ─────────────────────────────────────────────────────────────────────────────
struct EstadoLexer {
    string fuente;     // codigo fuente completo
    string archivo;    // nombre del archivo (para mensajes de error)
    int pos;        // posicion actual en el fuente
    int linea;      // linea actual
    int columna;    // columna actual
    ListaTokens tokens;     // lista enlazada de tokens reconocidos
    ListaErrores errores;    // lista enlazada de errores lexicos
};

// ─────────────────────────────────────────────────────────────────────────────
//  Interfaz publica del Lexer
// ─────────────────────────────────────────────────────────────────────────────

// Inicializa el struct con el codigo fuente a analizar
void lexer_init (EstadoLexer& lex, string fuente, string archivo);

// Ejecuta el analisis completo y llena lex.tokens y lex.errores
void lexer_analizar (EstadoLexer& lex);

// Libera la memoria de las listas enlazadas
void lexer_liberar (EstadoLexer& lex);

// ─────────────────────────────────────────────────────────────────────────────
//  Funciones internas — lectura de caracteres
// ─────────────────────────────────────────────────────────────────────────────
char lexer_actual (const EstadoLexer& lex);
char lexer_siguiente(const EstadoLexer& lex);
char lexer_avanzar (EstadoLexer& lex);
bool lexer_finFuente(const EstadoLexer& lex);
void lexer_saltarBlancos (EstadoLexer& lex);
bool lexer_saltarComentario(EstadoLexer& lex);

// ─────────────────────────────────────────────────────────────────────────────
//  AFDs — un AFD por tipo de token
// ─────────────────────────────────────────────────────────────────────────────
Token lexer_afd_identificador (EstadoLexer& lex, int linIni, int colIni);
Token lexer_afd_numero (EstadoLexer& lex, int linIni, int colIni);
Token lexer_afd_cadena (EstadoLexer& lex, int linIni, int colIni);
Token lexer_afd_operador (EstadoLexer& lex, int linIni, int colIni);
Token lexer_afd_delimitador (EstadoLexer& lex, char c, int linIni, int colIni);

// ─────────────────────────────────────────────────────────────────────────────
//  Tabla de palabras reservadas y registro de errores
// ─────────────────────────────────────────────────────────────────────────────
TokenType lexer_buscarPalabraReservada(const string& lexema);
Token lexer_registrarError(
    EstadoLexer& lex, 
    string msg,
    string lexema, 
    int lin, 
    int col
);