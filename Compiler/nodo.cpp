#include "nodo.h"
#include <iostream>
#include <string>
using namespace std;

//  nodo_crear — reserva un NodoAST en el heap con todos los punteros a null
NodoAST* nodo_crear(TipoNodo tipo, string valor, int linea, int columna) {
    NodoAST* n = new NodoAST;
    n->tipo = tipo;
    n->valor = valor;
    n->valorTipo = "";
    n->tipoAtrib = "";
    n->linea = linea;
    n->columna = columna;
    n->izq = nullptr;
    n->der = nullptr;
    n->extra = nullptr;
    n->siguiente = nullptr;
    return n;
}

//  nodo_liberar — libera recursivamente todo el subárbol
//  Orden: primero los hijos, luego el nodo mismo (postorden)
void nodo_liberar(NodoAST* nodo) {
    if (nodo == nullptr) return;
    nodo_liberar(nodo->izq);
    nodo_liberar(nodo->der);
    nodo_liberar(nodo->extra);
    nodo_liberar(nodo->siguiente);
    delete nodo;
}

//  Auxiliar interna — convierte TipoNodo a string legible para impresión
static string tipoNodoStr(TipoNodo t) {
    switch (t) {
        case NODO_PROGRAMA:       return "PROGRAMA";
        case NODO_FUNC:           return "FUNC";
        case NODO_PARAM:          return "PARAM";
        case NODO_BLOQUE:         return "BLOQUE";
        case NODO_DECL_VAR:       return "DECL_VAR";
        case NODO_ASIGNACION:     return "ASIGNACION";
        case NODO_LLAMADA:        return "LLAMADA";
        case NODO_SI:             return "SI";
        case NODO_PARA:           return "PARA";
        case NODO_MIENTRAS:       return "MIENTRAS";
        case NODO_RETORNAR:       return "RETORNAR";
        case NODO_ESCRIBIR:       return "ESCRIBIR";
        case NODO_LEER:           return "LEER";
        case NODO_LIMPPANTALLA:   return "LIMPPANTALLA";
        case NODO_OP_BINARIO:     return "OP_BINARIO";
        case NODO_OP_UNARIO:      return "OP_UNARIO";
        case NODO_INC_POST:       return "INC_POST";
        case NODO_DEC_POST:       return "DEC_POST";
        case NODO_LIT_ENTERO:     return "LIT_ENTERO";
        case NODO_LIT_DECIMAL:    return "LIT_DECIMAL";
        case NODO_LIT_CADENA:     return "LIT_CADENA";
        case NODO_LIT_VERDADERO:  return "LIT_VERDADERO";
        case NODO_LIT_FALSO:      return "LIT_FALSO";
        case NODO_IDENTIFICADOR:  return "IDENTIFICADOR";
        case NODO_LLAMADA_EXPR:   return "LLAMADA_EXPR";
        case NODO_ARGS:           return "ARGS";
        case NODO_ERROR:          return "ERROR";
        default:                  return "DESCONOCIDO";
    }
}

//  Auxiliar interna — genera la sangría visual para el árbol
static string sangria(int profundidad, bool esUltimo) {
    string s = "";
    for (int i = 0; i < profundidad - 1; i++) s += "|   ";
    if (profundidad > 0) s += (esUltimo ? "`-- " : "+-- ");
    return s;
}

//  nodo_imprimir_rec — recorrido preorden con formato de árbol
static void nodo_imprimir_rec(const NodoAST* nodo, int prof, bool esUltimo, bool mostrarAtribs) {
    if (nodo == nullptr) return;

    // ── Línea principal del nodo ─────────────────────────────────────────────
    cout << sangria(prof, esUltimo);
    cout << "[" << tipoNodoStr(nodo->tipo) << "]";

    // Mostrar valor si tiene contenido relevante
    if (!nodo->valor.empty())
        cout << " \"" << nodo->valor << "\"";

    // Mostrar tipo de dato cuando aplica (DECL_VAR, PARAM, FUNC)
    if (!nodo->valorTipo.empty())
        cout << " : " << nodo->valorTipo;

    // Mostrar atributo semantico calculado por el analizador semantico
    if (mostrarAtribs && !nodo->tipoAtrib.empty())
        cout << " {" << nodo->tipoAtrib << "}";

    // Mostrar posición en código fuente
    cout << "  (L" << nodo->linea << ",C" << nodo->columna << ")";
    cout << "\n";

    // ── Hijos con etiquetas semánticas según tipo de nodo ───────────────────
    // Se determina cuántos hijos directos tiene para saber quién es "último"
    bool tieneIzq   = (nodo->izq   != nullptr);
    bool tieneDer   = (nodo->der   != nullptr);
    bool tieneExtra = (nodo->extra != nullptr);
    bool tieneSig   = (nodo->siguiente != nullptr);

    // Calcular cuántos hijos se van a imprimir para marcar el último
    int totalHijos = (tieneIzq?1:0) + (tieneDer?1:0) +
                        (tieneExtra?1:0) + (tieneSig?1:0);
    int hijoActual = 0;

    if (tieneIzq) {
        hijoActual++;
        // Etiqueta semántica según tipo de nodo padre
        string etiqueta = "";
        switch (nodo->tipo) {
            case NODO_FUNC:        etiqueta = "params: ";    break;
            case NODO_SI:          etiqueta = "condicion: "; break;
            case NODO_PARA:        etiqueta = "init: ";      break;
            case NODO_MIENTRAS:    etiqueta = "condicion: "; break;
            case NODO_RETORNAR:    etiqueta = "expr: ";      break;
            case NODO_DECL_VAR:    etiqueta = "init: ";      break;
            case NODO_ASIGNACION:  etiqueta = "valor: ";     break;
            case NODO_OP_BINARIO:  etiqueta = "izq: ";       break;
            case NODO_OP_UNARIO:   etiqueta = "operando: ";  break;
            case NODO_INC_POST:    etiqueta = "operando: ";  break;
            case NODO_DEC_POST:    etiqueta = "operando: ";  break;
            case NODO_LLAMADA:     etiqueta = "args: ";      break;
            case NODO_LLAMADA_EXPR:etiqueta = "args: ";      break;
            case NODO_ESCRIBIR:    etiqueta = "args: ";      break;
            case NODO_ARGS:        etiqueta = "arg: ";       break;
            case NODO_BLOQUE:      etiqueta = "sent: ";      break;
            case NODO_PROGRAMA:    etiqueta = "decl: ";      break;
            default:               etiqueta = "izq: ";       break;
        }
        if (!etiqueta.empty()) {
            cout << sangria(prof + 1, hijoActual == totalHijos);
            cout << etiqueta << "\n";
            // imprimir el hijo con un nivel más de profundidad
            nodo_imprimir_rec(nodo->izq, prof + 2, true, mostrarAtribs);
        } else {
            nodo_imprimir_rec(nodo->izq, prof + 1, hijoActual == totalHijos, mostrarAtribs);
        }
    }

    if (tieneDer) {
        hijoActual++;
        string etiqueta = "";
        switch (nodo->tipo) {
            case NODO_FUNC:       etiqueta = "cuerpo: ";     break;
            case NODO_SI:         etiqueta = "entonces: ";   break;
            case NODO_PARA:       etiqueta = "condicion: ";  break;
            case NODO_MIENTRAS:   etiqueta = "cuerpo: ";     break;
            case NODO_OP_BINARIO: etiqueta = "der: ";        break;
            default:              etiqueta = "der: ";        break;
        }
        if (!etiqueta.empty()) {
            cout << sangria(prof + 1, hijoActual == totalHijos);
            cout << etiqueta << "\n";
            nodo_imprimir_rec(nodo->der, prof + 2, true, mostrarAtribs);
        } else {
            nodo_imprimir_rec(nodo->der, prof + 1, hijoActual == totalHijos, mostrarAtribs);
        }
    }

    if (tieneExtra) {
        hijoActual++;
        string etiqueta = "";
        switch (nodo->tipo) {
            case NODO_SI:   etiqueta = "sino: ";     break;
            case NODO_PARA: etiqueta = "cuerpo: ";   break;
            default:        etiqueta = "extra: ";    break;
        }
        if (!etiqueta.empty()) {
            cout << sangria(prof + 1, hijoActual == totalHijos);
            cout << etiqueta << "\n";
            nodo_imprimir_rec(nodo->extra, prof + 2, true, mostrarAtribs);
        } else {
            nodo_imprimir_rec(nodo->extra, prof + 1, hijoActual == totalHijos, mostrarAtribs);
        }
    }

    // siguiente se imprime como hermano al mismo nivel (lista enlazada)
    if (tieneSig) {
        nodo_imprimir_rec(nodo->siguiente, prof, false, mostrarAtribs);
    }
}

//  nodo_imprimir — punto de entrada público
void nodo_imprimir(const NodoAST* nodo, int profundidad, bool mostrarAtribs) {
    if (nodo == nullptr) {
        cout << "(arbol vacio)\n";
        return;
    }
    cout << "\n";
    if (mostrarAtribs) {
        cout << "+-----------------------------------------------------+\n";
        cout << "|     AST con atributos semanticos de tipo {T}        |\n";
        cout << "+-----------------------------------------------------+\n";
    } else {
        cout << "+-----------------------------------------------------+\n";
        cout << "|         Arbol de Sintaxis Abstracta (AST)           |\n";
        cout << "+-----------------------------------------------------+\n";
    }
    nodo_imprimir_rec(nodo, profundidad, true, mostrarAtribs);
    cout << "\n";
}