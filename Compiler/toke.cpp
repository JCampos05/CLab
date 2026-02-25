#include "token.h"
#include <iostream>
#include <iomanip>
using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  tokenTypeStr — convierte TokenType a string legible
// ─────────────────────────────────────────────────────────────────────────────
string tokenTypeStr(TokenType t) {
    switch (t) {
        // Palabras reservadas -> PAL_RSV_
        case PAL_RSV_ENTERO: return "PAL_RSV_ENTERO";
        case PAL_RSV_DECI: return "PAL_RSV_DECI";
        case PAL_RSV_CARAC: return "PAL_RSV_CARAC";
        case PAL_RSV_CAD: return "PAL_RSV_CAD";
        case PAL_RSV_LOGICO: return "PAL_RSV_LOGICO";
        case PAL_RSV_VACIO: return "PAL_RSV_VACIO";
        case PAL_RSV_VERDADERO: return "PAL_RSV_VERDADERO";
        case PAL_RSV_FALSO: return "PAL_RSV_FALSO";
        case PAL_RSV_SI: return "PAL_RSV_SI";
        case PAL_RSV_SINO: return "PAL_RSV_SINO";
        case PAL_RSV_PARA: return "PAL_RSV_PARA";
        case PAL_RSV_MIENTRAS: return "PAL_RSV_MIENTRAS";
        case PAL_RSV_FUNC: return "PAL_RSV_FUNC";
        case PAL_RSV_RETORNAR: return "PAL_RSV_RETORNAR";
        case PAL_RSV_LEER: return "PAL_RSV_LEER";
        case PAL_RSV_ESCRIBIR: return "PAL_RSV_ESCRIBIR";
        case PAL_RSV_LIMPPANTALLA: return "PAL_RSV_LIMPPANTALLA";
        case PAL_RSV_SUMA: return "PAL_RSV_SUMA";
        case PAL_RSV_RESTA: return "PAL_RSV_RESTA";
        case PAL_RSV_MUL: return "PAL_RSV_MUL";
        case PAL_RSV_DIV: return "PAL_RSV_DIV";
        case PAL_RSV_Y: return "PAL_RSV_Y";
        case PAL_RSV_O: return "PAL_RSV_O";
        case PAL_RSV_NO: return "PAL_RSV_NO";
        // Operadores aritmeticos
        case OP_SUMA: return "OP_SUMA";
        case OP_RESTA: return "OP_RESTA";
        case OP_MUL: return "OP_MUL";
        case OP_DIV: return "OP_DIV";
        case OP_MOD: return "OP_MOD";
        case OP_INC: return "OP_INC";
        case OP_DEC: return "OP_DEC";
        // Operadores relacionales
        case OP_MENOR: return "OP_MENOR";
        case OP_MAYOR: return "OP_MAYOR";
        case OP_MENOR_IGUAL: return "OP_MENOR_IGUAL";
        case OP_MAYOR_IGUAL: return "OP_MAYOR_IGUAL";
        case OP_IGUAL: return "OP_IGUAL";
        case OP_DISTINTO: return "OP_DISTINTO";
        case OP_ASIGNACION: return "OP_ASIGNACION";
        // Delimitadores
        case PUNTO_COMA: return "PUNTO_COMA";
        case COMA: return "COMA";
        case PAREN_ABR: return "PAREN_ABR";
        case PAREN_CIE: return "PAREN_CIE";
        case LLAVE_ABR: return "LLAVE_ABR";
        case LLAVE_CIE: return "LLAVE_CIE";
        // Literales
        case LIT_ENTERO: return "LIT_ENTERO";
        case LIT_DECIMAL: return "LIT_DECIMAL";
        case LIT_CADENA: return "LIT_CADENA";
        // Otros
        case IDENTIFICADOR: return "IDENTIFICADOR";
        case FIN_ARCHIVO: return "FIN_ARCHIVO";
        case TOKEN_ERROR: return "TOKEN_ERROR";
        default: return "DESCONOCIDO";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  crearToken — fabrica un token con valores por defecto
// ─────────────────────────────────────────────────────────────────────────────
Token crearToken(TokenType tipo, string lexema, int linea, int columna) {
    Token t;
    t.tipo = tipo;
    t.lexema = lexema;
    t.linea = linea;
    t.columna = columna;
    t.valorEntero = 0;
    t.valorDecimal = 0.0;
    t.valorCadena = "";
    return t;
}

// ─────────────────────────────────────────────────────────────────────────────
//  nombreTipo — retorna el nombre del tipo de un token
// ─────────────────────────────────────────────────────────────────────────────
string nombreTipo(Token t) {
    return tokenTypeStr(t.tipo);
}

//  ListaTokens — lista enlazada simple de tokens
void listaTokens_init(ListaTokens& lista) {
    lista.cabeza = nullptr;
    lista.cola = nullptr;
    lista.tamanio = 0;
}

void listaTokens_agregar(ListaTokens& lista, Token t) {
    NodoToken* nuevo = new NodoToken;
    nuevo->token = t;
    nuevo->siguiente = nullptr;

    if (lista.cabeza == nullptr) {
        lista.cabeza = nuevo;
        lista.cola = nuevo;
    } else {
        lista.cola->siguiente = nuevo;
        lista.cola = nuevo;
    }
    lista.tamanio++;
}

void listaTokens_imprimir(const ListaTokens& lista) {
    cout << "\n+-------+-------+----------------------+-----------------------+" << endl;
    cout << "|                   LISTA DE TOKENS                            |" << endl;
    cout << "+-------+-------+----------------------+-----------------------+" << endl;
    cout << "| LINEA |  COL  |        TIPO          |       LEXEMA          |" << endl;
    cout << "+-------+-------+----------------------+-----------------------+" << endl;

    NodoToken* actual = lista.cabeza;
    while (actual != nullptr) {
        Token& tok = actual->token;
        if (tok.tipo == FIN_ARCHIVO) {
            actual = actual->siguiente;
            continue;
        }
        string lex = tok.lexema;
        if (lex.size() > 21) lex = lex.substr(0, 18) + "...";

        cout << "| " << setw(5)  << tok.linea         << " | "
                        << setw(5)  << tok.columna        << " | "
                        << left
                        << setw(20) << nombreTipo(tok)    << " | "
                        << setw(21) << lex                << " |" << endl;
        actual = actual->siguiente;
    }
    cout << "+-------+-------+----------------------+-----------------------+" << endl;
    cout << "  Total de tokens: " << lista.tamanio << endl;
}

void listaTokens_liberar(ListaTokens& lista) {
    NodoToken* actual = lista.cabeza;
    while (actual != nullptr) {
        NodoToken* sig = actual->siguiente;
        delete actual;
        actual = sig;
    }
    lista.cabeza  = nullptr;
    lista.cola    = nullptr;
    lista.tamanio = 0;
}

//  ListaErrores — lista enlazada simple de errores lexicos
void listaErrores_init(ListaErrores& lista) {
    lista.cabeza  = nullptr;
    lista.cola    = nullptr;
    lista.tamanio = 0;
}

void listaErrores_agregar(ListaErrores& lista, 
    string msg, 
    int linea, 
    int columna, 
    string lexema) 
        {
        NodoError* nuevo        = new NodoError;
        nuevo->error.mensaje    = msg;
        nuevo->error.linea      = linea;
        nuevo->error.columna    = columna;
        nuevo->error.lexema     = lexema;
        nuevo->siguiente        = nullptr;

        if (lista.cabeza == nullptr) {
            lista.cabeza = nuevo;
            lista.cola   = nuevo;
        } else {
            lista.cola->siguiente = nuevo;
            lista.cola            = nuevo;
        }
        lista.tamanio++;
}   

void listaErrores_imprimir(const ListaErrores& lista) {
    cout << "\n+-------+-------+---------------+--------------------------------------------------+" << endl;
    cout << "|                       LISTA DE ERRORES LEXICOS                                    |" << endl;
    cout << "+-------+-------+---------------+--------------------------------------------------+" << endl;
    cout << "| LINEA |  COL  |    LEXEMA     |  DESCRIPCION                                     |" << endl;
    cout << "+-------+-------+---------------+--------------------------------------------------+" << endl;

    NodoError* actual = lista.cabeza;
    while (actual != nullptr) {
        ErrorLexico& e = actual->error;

        cout << "| " << setw(5)  << e.linea   << " | "
                        << setw(5)  << e.columna << " | "
                        << left
                        << setw(13) << e.lexema  << " | "
                        << setw(48) << e.mensaje << " |" << endl;
        actual = actual->siguiente;
    }
    cout << "+-------+-------+---------------+--------------------------------------------------+" << endl;
    cout << "  Total de errores: " << lista.tamanio << endl;
}

void listaErrores_liberar(ListaErrores& lista) {
    NodoError* actual = lista.cabeza;
    while (actual != nullptr) {
        NodoError* sig = actual->siguiente;
        delete actual;
        actual = sig;
    }
    lista.cabeza = nullptr;
    lista.cola = nullptr;
    lista.tamanio = 0;
}

bool listaErrores_hayErrores(const ListaErrores& lista) {
    return lista.tamanio > 0;
}