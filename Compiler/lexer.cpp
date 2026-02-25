#include "lexer.h"
#include <iostream>
#include <cctype>
using namespace std;

//  AFD 1. Identificadores: [a-zA-Z_][a-zA-Z0-9_]*
//    q0 --letra//_--> q1 --letra//digito//_--> q1 (aceptacion)

//  AFD 2/3. Numeros: [0-9]+ | [0-9]+\.[0-9]+
//    q0 --digito--> q1 --digito--> q1
//    q1 --'.'--> q2 --digito--> q3 --digito--> q3 (aceptacion decimal)
//    q1 sin '.' -> aceptacion como entero

//  AFD 4. Cadenas: "[^"]*"
//    q0 --"--> q1 --[^"]--> q1 --"--> q2 (aceptacion)

//  AFD 5. Operadores: lookahead de 1 caracter
//    q0 --op--> q1 | q0 --op--> q1 --op2--> q2 (aceptacion)

//  AFD 6. Delimitadores: un solo caracter
//    q0 --delim--> q1 (aceptacion inmediata)

// ─────────────────────────────────────────────────────────────────────────────
//  Inicializacion y liberacion
// ─────────────────────────────────────────────────────────────────────────────
void lexer_init(EstadoLexer& lex, string fuente, string archivo) {
    lex.fuente = fuente;
    lex.archivo = archivo;
    lex.pos = 0;
    lex.linea = 1;
    lex.columna = 1;
    listaTokens_init(lex.tokens);
    listaErrores_init(lex.errores);
}

void lexer_liberar(EstadoLexer& lex) {
    listaTokens_liberar(lex.tokens);
    listaErrores_liberar(lex.errores);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Lectura de caracteres
// ─────────────────────────────────────────────────────────────────────────────
bool lexer_finFuente(const EstadoLexer& lex) {
    return lex.pos >= (int)lex.fuente.size();
}

char lexer_actual(const EstadoLexer& lex) {
    return lexer_finFuente(lex) ? '\0' : lex.fuente[lex.pos];
}

char lexer_siguiente(const EstadoLexer& lex) {
    return (lex.pos + 1 < (int)lex.fuente.size()) ? lex.fuente[lex.pos + 1] : '\0';
}

char lexer_avanzar(EstadoLexer& lex) {
    char c = lexer_actual(lex);
    lex.pos++;
    if (c == '\n') { lex.linea++; lex.columna = 1; }
    else { lex.columna++; }
    return c;
}

void lexer_saltarBlancos(EstadoLexer& lex) {
    while (!lexer_finFuente(lex) && isspace((unsigned char)lexer_actual(lex)))
        lexer_avanzar(lex);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Saltar comentarios // y /* */
// ─────────────────────────────────────────────────────────────────────────────
bool lexer_saltarComentario(EstadoLexer& lex) {
    if (lexer_actual(lex) != '/') return false;

    // Comentario de linea: //
    if (lexer_siguiente(lex) == '/') {
        while (!lexer_finFuente(lex) && lexer_actual(lex) != '\n')
            lexer_avanzar(lex);
        return true;
    }

    // Comentario de bloque: /* ... */
    if (lexer_siguiente(lex) == '*') {
        int linIni = lex.linea, colIni = lex.columna;
        lexer_avanzar(lex); lexer_avanzar(lex); // consume '/' y '*'
        while (!lexer_finFuente(lex)) {
            if (lexer_actual(lex) == '*' && lexer_siguiente(lex) == '/') {
                lexer_avanzar(lex); lexer_avanzar(lex); // consume '*' y '/'
                return true;
            }
            lexer_avanzar(lex);
        }
        lexer_registrarError(lex,
            "Comentario de bloque no cerrado (falta */)", "/*", linIni, colIni);
        return true;
    }

    return false; // es un '/' de division, no comentario
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tabla de palabras reservadas — busqueda con switch
// ─────────────────────────────────────────────────────────────────────────────
TokenType lexer_buscarPalabraReservada(const string& lex) {
    if (lex == "entero") return PAL_RSV_ENTERO;
    if (lex == "deci") return PAL_RSV_DECI;
    if (lex == "carac") return PAL_RSV_CARAC;
    if (lex == "cad") return PAL_RSV_CAD;
    if (lex == "logico") return PAL_RSV_LOGICO;
    if (lex == "vacio") return PAL_RSV_VACIO;
    if (lex == "verdadero") return PAL_RSV_VERDADERO;
    if (lex == "falso") return PAL_RSV_FALSO;
    if (lex == "si") return PAL_RSV_SI;
    if (lex == "sino") return PAL_RSV_SINO;
    if (lex == "para") return PAL_RSV_PARA;
    if (lex == "mientras") return PAL_RSV_MIENTRAS;
    if (lex == "func") return PAL_RSV_FUNC;
    if (lex == "retornar") return PAL_RSV_RETORNAR;
    if (lex == "leer") return PAL_RSV_LEER;
    if (lex == "escribir") return PAL_RSV_ESCRIBIR;
    if (lex == "limpPantalla") return PAL_RSV_LIMPPANTALLA;
    if (lex == "suma") return PAL_RSV_SUMA;
    if (lex == "resta") return PAL_RSV_RESTA;
    if (lex == "mul") return PAL_RSV_MUL;
    if (lex == "div") return PAL_RSV_DIV;
    if (lex == "y") return PAL_RSV_Y;
    if (lex == "o") return PAL_RSV_O;
    if (lex == "no") return PAL_RSV_NO;
    return IDENTIFICADOR;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Registrar error lexico en la lista de errores
// ─────────────────────────────────────────────────────────────────────────────
Token lexer_registrarError(
    EstadoLexer& lex, 
    string msg,
    string lexema, 
    int lin, 
    int col
) {
    listaErrores_agregar(lex.errores, msg, lin, col, lexema);
    return crearToken(TOKEN_ERROR, lexema, lin, col);
}

//  AFD 1 — Identificadores y palabras reservadas
Token lexer_afd_identificador(EstadoLexer& lex, int linIni, int colIni) {
    enum Estado { q0, q1 };
    Estado estado = q0;
    string lexema = "";

    while (!lexer_finFuente(lex)) {
        char c = lexer_actual(lex);

        if (estado == q0) {
            if (isalpha((unsigned char)c) || c == '_') {
                lexema += lexer_avanzar(lex);
                estado = q1;
            } else {
                return lexer_registrarError(lex,
                    "Error interno AFD1", lexema, linIni, colIni);
            }
        }
        else if (estado == q1) {
            if (isalnum((unsigned char)c) || c == '_') {
                lexema += lexer_avanzar(lex); // q1 -> q1
            } else {
                break; // aceptar
            }
        }
    }

    // q1 es estado de aceptacion: verificar si es palabra reservada
    TokenType tipo = lexer_buscarPalabraReservada(lexema);
    return crearToken(tipo, lexema, linIni, colIni);
}

//  AFD 2/3 — Numeros enteros y decimales
Token lexer_afd_numero(EstadoLexer& lex, int linIni, int colIni) {
    enum Estado { q0, q1, q2, q3 };
    Estado estado = q0;
    string lexema = "";

    while (!lexer_finFuente(lex)) {
        char c = lexer_actual(lex);

        if (estado == q0) {
            if (isdigit((unsigned char)c)) {
                lexema += lexer_avanzar(lex);
                estado = q1;
            } else {
                return lexer_registrarError(lex,
                    "Error interno AFD2/3", lexema, linIni, colIni);
            }
        }
        else if (estado == q1) {
            if (isdigit((unsigned char)c)) {
                lexema += lexer_avanzar(lex); // q1 -> q1: mas digitos
            }
            else if (c == '.' && isdigit((unsigned char)lexer_siguiente(lex))) {
                lexema += lexer_avanzar(lex); // q1 -> q2: punto con digito
                estado = q2;
            }
            else if (c == '.' && !isdigit((unsigned char)lexer_siguiente(lex))) {
                lexema += lexer_avanzar(lex); // consume el punto
                return lexer_registrarError(lex,
                    "Decimal mal formado: falta digito tras el punto",
                    lexema, linIni, colIni);
            }
            else {
                break; // aceptar como entero
            }
        }
        else if (estado == q2) {
            if (isdigit((unsigned char)c)) {
                lexema += lexer_avanzar(lex); // q2 -> q3
                estado = q3;
            } else {
                return lexer_registrarError(lex,
                    "Decimal mal formado: falta digito tras el punto",
                    lexema, linIni, colIni);
            }
        }
        else if (estado == q3) {
            if (isdigit((unsigned char)c)) {
                lexema += lexer_avanzar(lex); // q3 -> q3: mas decimales
            } else {
                break; // aceptar como decimal
            }
        }
    }

    // Estado de aceptacion
    if (estado == q3) {
        Token tok = crearToken(LIT_DECIMAL, lexema, linIni, colIni);
        tok.valorDecimal = stod(lexema);
        return tok;
    } else {
        Token tok = crearToken(LIT_ENTERO, lexema, linIni, colIni);
        tok.valorEntero = stoll(lexema);
        return tok;
    }
}

//  AFD 4 — Cadenas de texto
Token lexer_afd_cadena(EstadoLexer& lex, int linIni, int colIni) {
    enum Estado { q0, q1, q2 };
    Estado estado   = q0;
    string contenido = "";

    while (!lexer_finFuente(lex)) {
        char c = lexer_actual(lex);

        if (estado == q0) {
            lexer_avanzar(lex); // q0 -> q1: consume '"' de apertura
            estado = q1;
        }
        else if (estado == q1) {
            if (c == '"') {
                lexer_avanzar(lex); // q1 -> q2: consume '"' de cierre
                estado = q2;
                break; // aceptar
            }
            else if (c == '\n') {
                return lexer_registrarError(lex,
                    "Cadena no terminada: salto de linea inesperado",
                    "\"" + contenido, linIni, colIni);
            }
            else {
                char ch = lexer_avanzar(lex);
                if (ch != '\r') contenido += ch; // q1 -> q1 
            }
        }
    }

    if (estado != q2) {
        // Llego a EOF sin cerrar la cadena
        return lexer_registrarError(lex,
            "Cadena no terminada al final del archivo",
            "\"" + contenido, linIni, colIni);
    }

    Token tok = crearToken(LIT_CADENA, "\"" + contenido + "\"", linIni, colIni);
    tok.valorCadena = contenido;
    return tok;
}

//  AFD 5 — Operadores (con lookahead de 1 caracter)
Token lexer_afd_operador(EstadoLexer& lex, int linIni, int colIni) {
    char c = lexer_avanzar(lex); // q0 -> q1: consume primer caracter

    switch (c) {
        case '+':
            if (lexer_actual(lex) == '+') {
                lexer_avanzar(lex); // q1 -> q2
                return crearToken(OP_INC, "++", linIni, colIni);
            }
            return crearToken(OP_SUMA, "+", linIni, colIni);

        case '-':
            if (lexer_actual(lex) == '-') {
                lexer_avanzar(lex);
                return crearToken(OP_DEC, "--", linIni, colIni);
            }
            return crearToken(OP_RESTA, "-", linIni, colIni);

        case '<':
            if (lexer_actual(lex) == '=') {
                lexer_avanzar(lex);
                return crearToken(OP_MENOR_IGUAL, "<=", linIni, colIni);
            }
            return crearToken(OP_MENOR, "<", linIni, colIni);

        case '>':
            if (lexer_actual(lex) == '=') {
                lexer_avanzar(lex);
                return crearToken(OP_MAYOR_IGUAL, ">=", linIni, colIni);
            }
            return crearToken(OP_MAYOR, ">", linIni, colIni);

        case '=':
            if (lexer_actual(lex) == '=') {
                lexer_avanzar(lex);
                return crearToken(OP_IGUAL, "==", linIni, colIni);
            }
            return crearToken(OP_ASIGNACION, "=", linIni, colIni);

        case '!':
            if (lexer_actual(lex) == '=') {
                lexer_avanzar(lex);
                return crearToken(OP_DISTINTO, "!=", linIni, colIni);
            }
            return lexer_registrarError(lex,
                "Caracter '!' sin '=' (operador invalido)",
                "!", linIni, colIni);

        case '*': return crearToken(OP_MUL, "*", linIni, colIni);
        case '/': return crearToken(OP_DIV, "/", linIni, colIni);
        case '%': return crearToken(OP_MOD, "%", linIni, colIni);

        default:
            return lexer_registrarError(lex,
                "Caracter no reconocido: '" + string(1, c) + "'",
                string(1, c), linIni, colIni);
    }
}

//  AFD 6 — Delimitadores
Token lexer_afd_delimitador(EstadoLexer& lex, char c, int linIni, int colIni) {
    lexer_avanzar(lex); // q0 -> q1: consume el delimitador

    switch (c) {
        case ';': return crearToken(PUNTO_COMA, ";", linIni, colIni);
        case ',': return crearToken(COMA,       ",", linIni, colIni);
        case '(': return crearToken(PAREN_ABR,  "(", linIni, colIni);
        case ')': return crearToken(PAREN_CIE,  ")", linIni, colIni);
        case '{': return crearToken(LLAVE_ABR,  "{", linIni, colIni);
        case '}': return crearToken(LLAVE_CIE,  "}", linIni, colIni);
        default:
            return lexer_registrarError(lex,
                "Delimitador desconocido: '" + string(1, c) + "'",
                string(1, c), linIni, colIni);
    }
}

//  lexer_analizar -> bucle principal de despacho a los AFDs
void lexer_analizar(EstadoLexer& lex) {
    while (true) {
        // Saltar blancos y comentarios
        while (true) {
            lexer_saltarBlancos(lex);
            if (!lexer_saltarComentario(lex)) break;
        }

        if (lexer_finFuente(lex)) {
            listaTokens_agregar(lex.tokens,
                crearToken(FIN_ARCHIVO, "", lex.linea, lex.columna));
            break;
        }

        int  linIni = lex.linea;
        int  colIni = lex.columna;
        char c      = lexer_actual(lex);
        Token tok;

        // Despacho al AFD correspondiente segun el primer caracter
        if (isalpha((unsigned char)c) || c == '_') {
            tok = lexer_afd_identificador(lex, linIni, colIni); // AFD 1

        } else if (isdigit((unsigned char)c)) {
            tok = lexer_afd_numero(lex, linIni, colIni); // AFD 2/3

        } else if (c == '"') {
            tok = lexer_afd_cadena(lex, linIni, colIni); // AFD 4

        } else if (c == ';' || c == ',' ||
                    c == '(' || c == ')' ||
                    c == '{' || c == '}') {
            tok = lexer_afd_delimitador(lex, c, linIni, colIni); // AFD 6

        } else {
            tok = lexer_afd_operador(lex, linIni, colIni); // AFD 5
        }

        listaTokens_agregar(lex.tokens, tok);
    }
}