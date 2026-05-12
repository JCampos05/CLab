#include "semantico.h"
#include <iostream>
#include <iomanip>
#include <string>
using namespace std;

// ##############################################################################
//  SECCION 1 — Estado interno, utilidades de tipo y extraccion de valores
// ##############################################################################

// Puntero global al estado activo; se asigna en semantico_analizar y se limpia
// al salir, igual que gP en parser.cpp.
static EstadoSemantico* gS = nullptr;

static void registrarError(const string& msg, int linea, int col, const string& lex) {
    listaErrores_agregar(gS->errores, msg, linea, col, lex);
    gS->hayError = true;
}

static bool esNumerico(const string& tipo) {
    return tipo == "entero" || tipo == "deci";
}

// Regla de compatibilidad de tipos:
//   T     <- T        OK  (identico)
//   deci  <- entero   OK  (promocion implicita por el tipado del lenguaje)
//   T     <- "?"      OK  (error ya reportado en la subexpresion — suprime cascada)
//   cualquier otro    ERROR
static bool tiposCompatibles(const string& esperado, const string& dado) {
    if (dado == "?")                             return true;
    if (esperado == dado)                        return true;
    if (esperado == "deci" && dado == "entero")  return true;
    return false;
}

// Extrae el valor literal de un nodo inicializador para la tabla de simbolos.
// Si la expresion no es un literal simple, retorna "<expr>".
// Si no hay inicializador, retorna "-".
static string extraerValorInit(const NodoAST* init) {
    if (!init) return "-";
    switch (init->tipo) {
        case NODO_LIT_ENTERO:    return init->valor;
        case NODO_LIT_DECIMAL:   return init->valor;
        case NODO_LIT_CADENA:    return "\"" + init->valor + "\"";
        case NODO_LIT_VERDADERO: return "verdadero";
        case NODO_LIT_FALSO:     return "falso";
        default:                 return "<expr>";
    }
}

// ##############################################################################
//  SECCION 2 — Tabla de simbolos unificada (EntradaTabla / TablaSimbolos)
// ##############################################################################

// Agrega una entrada al final de la tabla unificada.
static void tabla_agregar(const string& nombre, const string& tipo,
                           const string& valor,  const string& ambito,
                           int linea, int columna, const string& categoria) {
    EntradaTabla* e = new EntradaTabla;
    e->id        = ++gS->tabla.contadorId;
    e->nombre    = nombre;
    e->tipo      = tipo;
    e->valor     = valor;
    e->ambito    = ambito;
    e->linea     = linea;
    e->columna   = columna;
    e->categoria = categoria;
    e->siguiente = nullptr;

    if (!gS->tabla.cabeza) {
        gS->tabla.cabeza = e;
    } else {
        EntradaTabla* ult = gS->tabla.cabeza;
        while (ult->siguiente) ult = ult->siguiente;
        ult->siguiente = e;
    }
    gS->tabla.tamanio++;
}

// Actualiza el campo Valor de la declaracion de una variable cuando se le asigna
// un literal por primera vez (solo si el valor actual es "-").
// Busca desde el final para encontrar la declaracion mas reciente del nombre en
// el ambito actual.
static void tabla_actualizarValor(const string& nombre, const string& nuevoValor) {
    EntradaTabla* ultima = nullptr;
    EntradaTabla* e      = gS->tabla.cabeza;
    while (e) {
        if (e->nombre    == nombre          &&
            e->ambito    == gS->ambitoActual &&
            e->categoria == "variable"       &&
            e->valor     == "-") {
            ultima = e;
        }
        e = e->siguiente;
    }
    if (ultima) ultima->valor = nuevoValor;
}

static void tabla_liberar(TablaSimbolos& t) {
    EntradaTabla* e = t.cabeza;
    while (e) {
        EntradaTabla* sig = e->siguiente;
        delete e;
        e = sig;
    }
    t.cabeza      = nullptr;
    t.tamanio     = 0;
    t.contadorId  = 0;
}

// ##############################################################################
//  SECCION 3 — Tabla interna de funciones (para resolucion durante el analisis)
// ##############################################################################

static void liberar_params(NodoParam* p) {
    while (p) {
        NodoParam* sig = p->siguiente;
        delete p;
        p = sig;
    }
}

static bool tablaFunc_agregar(const string& nombre, const string& tipoRet,
                               NodoParam* params, int aridad, int linea) {
    NodoFuncion* f = gS->funciones.cabeza;
    while (f) {
        if (f->nombre == nombre) return false;
        f = f->siguiente;
    }
    NodoFuncion* nf  = new NodoFuncion;
    nf->nombre       = nombre;
    nf->tipoRetorno  = tipoRet;
    nf->aridad       = aridad;
    nf->params       = params;
    nf->linea        = linea;
    nf->siguiente    = nullptr;

    if (!gS->funciones.cabeza) {
        gS->funciones.cabeza = nf;
    } else {
        NodoFuncion* ult = gS->funciones.cabeza;
        while (ult->siguiente) ult = ult->siguiente;
        ult->siguiente = nf;
    }
    gS->funciones.tamanio++;
    return true;
}

static NodoFuncion* tablaFunc_buscar(const string& nombre) {
    NodoFuncion* f = gS->funciones.cabeza;
    while (f) {
        if (f->nombre == nombre) return f;
        f = f->siguiente;
    }
    return nullptr;
}

static void tablaFunc_liberar(TablaFunciones& t) {
    NodoFuncion* f = t.cabeza;
    while (f) {
        NodoFuncion* sig = f->siguiente;
        liberar_params(f->params);
        delete f;
        f = sig;
    }
    t.cabeza   = nullptr;
    t.tamanio  = 0;
}

// ##############################################################################
//  SECCION 4 — Stack de scopes de variables
// ##############################################################################

static void scope_abrir() {
    NodoScope* nuevo = new NodoScope;
    nuevo->vars      = nullptr;
    nuevo->padre     = gS->scopeActual;
    gS->scopeActual  = nuevo;
}

static void scope_cerrar() {
    if (!gS->scopeActual) return;
    NodoScope* viejo  = gS->scopeActual;
    gS->scopeActual   = viejo->padre;
    NodoVariable* v   = viejo->vars;
    while (v) {
        NodoVariable* sig = v->siguiente;
        delete v;
        v = sig;
    }
    delete viejo;
}

// Declara en el scope actual; false si el nombre ya existe en ESTE nivel.
static bool scope_declarar(const string& nombre, const string& tipo, int linea) {
    if (!gS->scopeActual) return false;
    NodoVariable* v = gS->scopeActual->vars;
    while (v) {
        if (v->nombre == nombre) return false;
        v = v->siguiente;
    }
    NodoVariable* nueva   = new NodoVariable;
    nueva->nombre         = nombre;
    nueva->tipo           = tipo;
    nueva->linea          = linea;
    nueva->siguiente      = gS->scopeActual->vars;
    gS->scopeActual->vars = nueva;
    return true;
}

// Busca en el scope actual y todos sus ancestros; null si no existe en ningun nivel.
static NodoVariable* scope_buscar(const string& nombre) {
    NodoScope* s = gS->scopeActual;
    while (s) {
        NodoVariable* v = s->vars;
        while (v) {
            if (v->nombre == nombre) return v;
            v = v->siguiente;
        }
        s = s->padre;
    }
    return nullptr;
}

// ##############################################################################
//  SECCION 5 — Funciones nativas pre-registradas (builtins)
// ##############################################################################

// Registra una funcion nativa de dos parametros entero con retorno entero.
static void registrar_builtin2e(const string& nombre) {
    NodoFuncion* f = gS->funciones.cabeza;
    while (f) { if (f->nombre == nombre) return; f = f->siguiente; }

    NodoParam* p2   = new NodoParam;
    p2->nombre      = "b";
    p2->tipo        = "entero";
    p2->siguiente   = nullptr;

    NodoParam* p1   = new NodoParam;
    p1->nombre      = "a";
    p1->tipo        = "entero";
    p1->siguiente   = p2;

    NodoFuncion* nf = new NodoFuncion;
    nf->nombre      = nombre;
    nf->tipoRetorno = "entero";
    nf->aridad      = 2;
    nf->params      = p1;
    nf->linea       = 0;
    nf->siguiente   = nullptr;

    if (!gS->funciones.cabeza) {
        gS->funciones.cabeza = nf;
    } else {
        NodoFuncion* ult = gS->funciones.cabeza;
        while (ult->siguiente) ult = ult->siguiente;
        ult->siguiente = nf;
    }
    gS->funciones.tamanio++;
    // No se agrega a TablaSimbolos: las funciones nativas no son declaraciones del usuario.
}

static void registrar_builtins() {
    registrar_builtin2e("mul");
    registrar_builtin2e("suma");
    registrar_builtin2e("resta");
    registrar_builtin2e("div");
}

// ##############################################################################
//  SECCION 6 — Analisis de expresiones con propagacion de atributos (postorden)
//
//  ALGORITMO DE ATRIBUTOS SINTETIZADOS:
//    Cada nodo de expresion computa su tipo como atributo sintetizado: el tipo
//    se calcula a partir de los tipos de los hijos (bottom-up / postorden).
//    Al terminar, el tipo calculado se almacena en nodo->tipoAtrib para que
//    quede anotado en el AST y sea visible en la impresion del arbol.
//
//  ATRIBUTOS HEREDADOS presentes en el analizador:
//    - tipoRetFunActual: heredado hacia abajo desde la declaracion de funcion
//      hasta las sentencias 'retornar' dentro de su cuerpo.
//    - ambitoActual: heredado hacia abajo para registrar en que funcion vive
//      cada variable declarada.
//    - scopeActual: heredado como contexto de busqueda de nombres.
//
//  Tipos validos: "entero" | "deci" | "logico" | "cad" | "carac" | "?"
//  "?" es el centinela de error: suprime errores en cascada en nodos padre.
// ##############################################################################

// Declaracion adelantada — mutua recursion con las funciones de sentencias
static string analizar_expresion(NodoAST* nodo);

static int contarArgs(const NodoAST* argsNodo) {
    if (!argsNodo) return 0;
    int count        = 0;
    const NodoAST* a = argsNodo->izq;
    while (a) { count++; a = a->siguiente; }
    return count;
}

// Verifica existencia, aridad y tipos de una llamada a funcion.
// nodo->valor = nombre; nodo->izq = NODO_ARGS (o null si sin argumentos).
static void verificar_llamada(NodoAST* nodo) {
    NodoFuncion* func = tablaFunc_buscar(nodo->valor);
    if (!func) {
        registrarError("Funcion '" + nodo->valor + "' no declarada",
                       nodo->linea, nodo->columna, nodo->valor);
        return;
    }
    int numArgs = contarArgs(nodo->izq);
    if (numArgs != func->aridad) {
        registrarError("Funcion '" + nodo->valor + "' espera " + to_string(func->aridad) +
                       " argumento(s), se dieron " + to_string(numArgs),
                       nodo->linea, nodo->columna, nodo->valor);
    }
    NodoParam* param = func->params;
    NodoAST*   arg   = nodo->izq ? nodo->izq->izq : nullptr;
    while (param && arg) {
        string tipoArg = analizar_expresion(arg);
        if (!tiposCompatibles(param->tipo, tipoArg)) {
            registrarError("Argumento de tipo '" + tipoArg +
                           "' no compatible con parametro de tipo '" + param->tipo +
                           "' en llamada a '" + nodo->valor + "'",
                           arg->linea, arg->columna, nodo->valor);
        }
        param = param->siguiente;
        arg   = arg->siguiente;
    }
}

// Analiza una expresion en postorden y retorna su tipo como atributo sintetizado.
// Tambien anota nodo->tipoAtrib con el resultado para que quede en el AST.
static string analizar_expresion(NodoAST* nodo) {
    if (!nodo) return "?";
    string result;

    switch (nodo->tipo) {

    // ── Literales — tipo fijo, atributo sintetizado directo ───────────────────
    case NODO_LIT_ENTERO:    result = "entero"; break;
    case NODO_LIT_DECIMAL:   result = "deci";   break;
    case NODO_LIT_CADENA:    result = "cad";    break;
    case NODO_LIT_VERDADERO: result = "logico"; break;
    case NODO_LIT_FALSO:     result = "logico"; break;

    // ── Identificador — busca en scopes (atributo heredado del ambiente) ──────
    case NODO_IDENTIFICADOR: {
        NodoVariable* var = scope_buscar(nodo->valor);
        if (!var) {
            registrarError("Variable '" + nodo->valor + "' no declarada en este scope",
                           nodo->linea, nodo->columna, nodo->valor);
            result = "?";
        } else {
            result = var->tipo;
        }
        break;
    }

    // ── Llamada como expresion — tipo sintetizado = tipo de retorno ───────────
    case NODO_LLAMADA_EXPR: {
        NodoFuncion* func = tablaFunc_buscar(nodo->valor);
        if (!func) {
            registrarError("Funcion '" + nodo->valor + "' no declarada",
                           nodo->linea, nodo->columna, nodo->valor);
            if (nodo->izq) {
                NodoAST* arg = nodo->izq->izq;
                while (arg) { analizar_expresion(arg); arg = arg->siguiente; }
            }
            result = "?";
            break;
        }
        int numArgs = contarArgs(nodo->izq);
        if (numArgs != func->aridad) {
            registrarError("Funcion '" + nodo->valor + "' espera " + to_string(func->aridad) +
                           " argumento(s), se dieron " + to_string(numArgs),
                           nodo->linea, nodo->columna, nodo->valor);
        }
        NodoParam* param = func->params;
        NodoAST*   arg   = nodo->izq ? nodo->izq->izq : nullptr;
        while (arg) {
            string tipoArg = analizar_expresion(arg);
            if (param) {
                if (!tiposCompatibles(param->tipo, tipoArg)) {
                    registrarError("Argumento de tipo '" + tipoArg +
                                   "' no compatible con parametro de tipo '" + param->tipo +
                                   "' en llamada a '" + nodo->valor + "'",
                                   arg->linea, arg->columna, nodo->valor);
                }
                param = param->siguiente;
            }
            arg = arg->siguiente;
        }
        result = func->tipoRetorno;
        break;
    }

    // ── Operador binario — tipo sintetizado segun operador y tipos de hijos ──
    case NODO_OP_BINARIO: {
        string tipoIzq    = analizar_expresion(nodo->izq);
        string tipoDer    = analizar_expresion(nodo->der);
        const string& op  = nodo->valor;

        if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
            if (tipoIzq == "?" || tipoDer == "?") { result = "?"; break; }

            // Tabla de operaciones aritmeticas por tipo:
            //          cad    entero  deci   logico
            // cad   |  cad*  |  E   |  E   |  E   |   (*solo con '+')
            // entero|   E    | ent  | deci  |  E   |
            // deci  |   E    | deci | deci  |  E   |
            // logico|   E    |  E   |  E   |  E   |

            // Caso especial: concatenacion de cadenas solo con '+'
            if (tipoIzq == "cad" && tipoDer == "cad") {
                if (op == "+") { result = "cad"; break; }
                registrarError("Operador '" + op + "' no aplica a tipo 'cad'",
                               nodo->linea, nodo->columna, op);
                result = "?"; break;
            }
            if (!esNumerico(tipoIzq)) {
                registrarError("Operador '" + op + "' no aplica a tipo '" + tipoIzq + "'",
                               nodo->linea, nodo->columna, op);
                result = "?"; break;
            }
            if (!esNumerico(tipoDer)) {
                registrarError("Operador '" + op + "' no aplica a tipo '" + tipoDer + "'",
                               nodo->linea, nodo->columna, op);
                result = "?"; break;
            }
            // Promocion: si alguno es deci el resultado es deci
            result = (tipoIzq == "deci" || tipoDer == "deci") ? "deci" : "entero";

        } else if (op == "<" || op == ">" || op == "<=" || op == ">=" ||
                   op == "==" || op == "!=") {
            if (tipoIzq != "?" && tipoDer != "?") {
                if (!tiposCompatibles(tipoIzq, tipoDer) && !tiposCompatibles(tipoDer, tipoIzq)) {
                    registrarError("Operador '" + op + "' requiere operandos compatibles, "
                                   "se encontraron '" + tipoIzq + "' y '" + tipoDer + "'",
                                   nodo->linea, nodo->columna, op);
                }
            }
            result = "logico";

        } else if (op == "y" || op == "o") {
            if (tipoIzq != "?" && tipoIzq != "logico") {
                registrarError("Operador '" + op + "' no aplica a tipo '" + tipoIzq + "'",
                               nodo->linea, nodo->columna, op);
            }
            if (tipoDer != "?" && tipoDer != "logico") {
                registrarError("Operador '" + op + "' no aplica a tipo '" + tipoDer + "'",
                               nodo->linea, nodo->columna, op);
            }
            result = "logico";

        } else {
            result = "?";
        }
        break;
    }

    // ── Operador unario — tipo sintetizado desde el operando ─────────────────
    case NODO_OP_UNARIO: {
        const string& op     = nodo->valor;
        string tipoOperando  = analizar_expresion(nodo->izq);

        if (op == "no") {
            if (tipoOperando != "?" && tipoOperando != "logico") {
                registrarError("Operador 'no' no aplica a tipo '" + tipoOperando + "'",
                               nodo->linea, nodo->columna, op);
            }
            result = "logico";
        } else if (op == "-") {
            if (tipoOperando != "?" && !esNumerico(tipoOperando)) {
                registrarError("Operador '-' unario no aplica a tipo '" + tipoOperando + "'",
                               nodo->linea, nodo->columna, op);
                result = "?";
            } else {
                result = tipoOperando;
            }
        } else {
            result = "?";
        }
        break;
    }

    // ── Incremento / decremento postfijo ─────────────────────────────────────
    case NODO_INC_POST:
    case NODO_DEC_POST: {
        if (!nodo->izq) { result = "?"; break; }
        NodoVariable* var = scope_buscar(nodo->izq->valor);
        if (!var) {
            registrarError("Variable '" + nodo->izq->valor + "' no declarada en este scope",
                           nodo->izq->linea, nodo->izq->columna, nodo->izq->valor);
            result = "?";
        } else if (!esNumerico(var->tipo)) {
            string opStr = (nodo->tipo == NODO_INC_POST) ? "++" : "--";
            registrarError("Operador '" + opStr + "' no aplica a tipo '" + var->tipo + "'",
                           nodo->linea, nodo->columna, nodo->izq->valor);
            result = "?";
        } else {
            result = var->tipo;
        }
        break;
    }

    default:
        result = "?";
        break;
    }

    // ── Anotacion del atributo en el nodo AST ────────────────────────────────
    nodo->tipoAtrib = result;
    return result;
}

// ##############################################################################
//  SECCION 7 — Recorrido del AST: sentencias, bloques y funciones
// ##############################################################################

static void analizar_sentencia(NodoAST* nodo);

// Recorre las sentencias de un NODO_BLOQUE.
// abrirScope=false cuando el scope ya fue abierto por el llamador
// (cuerpo de funcion, cuerpo de para).
static void analizar_bloque(NodoAST* nodo, bool abrirScope) {
    if (!nodo) return;
    if (abrirScope) scope_abrir();

    NodoAST* sent = nodo->izq;
    while (sent) {
        analizar_sentencia(sent);
        sent = sent->siguiente;
    }

    if (abrirScope) scope_cerrar();
}

static void analizar_sentencia(NodoAST* nodo) {
    if (!nodo) return;

    switch (nodo->tipo) {

    // ── Declaracion de variable ───────────────────────────────────────────────
    //    Algoritmo:
    //    1. Declarar en scope actual (detecta redeclaracion en mismo nivel)
    //    2. Si hay inicializador, sintetizar su tipo y verificar compatibilidad
    //    3. Registrar en tabla de simbolos unificada con el valor inicial
    case NODO_DECL_VAR: {
        string valorInit = extraerValorInit(nodo->izq);
        if (!scope_declarar(nodo->valor, nodo->valorTipo, nodo->linea)) {
            registrarError("Variable '" + nodo->valor + "' ya declarada en este scope",
                           nodo->linea, nodo->columna, nodo->valor);
        }
        if (nodo->izq) {
            string tipoExpr = analizar_expresion(nodo->izq);
            if (!tiposCompatibles(nodo->valorTipo, tipoExpr)) {
                registrarError("No se puede asignar tipo '" + tipoExpr +
                               "' a variable de tipo '" + nodo->valorTipo + "'",
                               nodo->linea, nodo->columna, nodo->valor);
            }
        }
        tabla_agregar(nodo->valor, nodo->valorTipo, valorInit,
                      gS->ambitoActual, nodo->linea, nodo->columna, "variable");
        break;
    }

    // ── Asignacion ────────────────────────────────────────────────────────────
    //    Algoritmo:
    //    1. Buscar la variable en los scopes visibles (identificadores fuera de ambito)
    //    2. Sintetizar el tipo de la expresion derecha
    //    3. Verificar compatibilidad con el tipo declarado de la variable
    case NODO_ASIGNACION: {
        NodoVariable* var = scope_buscar(nodo->valor);
        if (!var) {
            registrarError("Variable '" + nodo->valor + "' no declarada en este scope",
                           nodo->linea, nodo->columna, nodo->valor);
        }
        if (nodo->izq) {
            string tipoExpr = analizar_expresion(nodo->izq);
            if (var && !tiposCompatibles(var->tipo, tipoExpr)) {
                registrarError("No se puede asignar tipo '" + tipoExpr +
                               "' a variable de tipo '" + var->tipo + "'",
                               nodo->linea, nodo->columna, nodo->valor);
            }
            // Si el RHS es un literal, actualizar el valor en la tabla de simbolos
            string valLit = extraerValorInit(nodo->izq);
            if (valLit != "<expr>") tabla_actualizarValor(nodo->valor, valLit);
        }
        break;
    }

    // ── Llamada a funcion como sentencia ──────────────────────────────────────
    //    Algoritmo: verificar existencia, aridad y tipos de argumentos
    case NODO_LLAMADA: {
        verificar_llamada(nodo);
        break;
    }

    // ── Sentencia si ──────────────────────────────────────────────────────────
    //    Algoritmo:
    //    1. La condicion debe tener tipo 'logico' (atributo sintetizado)
    //    2. Cada bloque abre su propio scope
    case NODO_SI: {
        string tipoCond = analizar_expresion(nodo->izq);
        if (tipoCond != "?" && tipoCond != "logico") {
            int lin = nodo->izq ? nodo->izq->linea   : nodo->linea;
            int col = nodo->izq ? nodo->izq->columna : nodo->columna;
            registrarError("La condicion debe ser de tipo 'logico', se encontro '" + tipoCond + "'",
                           lin, col, "si");
        }
        analizar_bloque(nodo->der,   true);
        analizar_bloque(nodo->extra, true);
        break;
    }

    // ── Sentencia mientras ────────────────────────────────────────────────────
    case NODO_MIENTRAS: {
        string tipoCond = analizar_expresion(nodo->izq);
        if (tipoCond != "?" && tipoCond != "logico") {
            int lin = nodo->izq ? nodo->izq->linea   : nodo->linea;
            int col = nodo->izq ? nodo->izq->columna : nodo->columna;
            registrarError("La condicion debe ser de tipo 'logico', se encontro '" + tipoCond + "'",
                           lin, col, "mientras");
        }
        analizar_bloque(nodo->der, true);
        break;
    }

    // ── Sentencia para ────────────────────────────────────────────────────────
    //    Algoritmo:
    //    1. Abrir scope del para (comparte con el cuerpo via abrirScope=false)
    //    2. Forma A (valorTipo != ""): declarar la variable de control en este scope
    //    3. Forma B (valorTipo == ""): verificar que la variable ya este declarada
    //    4. La condicion debe ser logico
    //    5. El cuerpo incluye el incremento/decremento como primer hijo del bloque
    case NODO_PARA: {
        scope_abrir();
        const NodoAST* init = nodo->izq;
        if (init) {
            if (init->valorTipo.empty()) {
                // Forma B: variable declarada externamente — verificar que exista
                NodoVariable* var = scope_buscar(init->valor);
                if (!var) {
                    registrarError("Variable '" + init->valor + "' no declarada en este scope",
                                   init->linea, init->columna, init->valor);
                }
                if (init->izq && var) {
                    string tipoExpr = analizar_expresion(const_cast<NodoAST*>(init->izq));
                    if (!tiposCompatibles(var->tipo, tipoExpr)) {
                        registrarError("No se puede asignar tipo '" + tipoExpr +
                                       "' a variable de tipo '" + var->tipo + "'",
                                       init->linea, init->columna, init->valor);
                    }
                }
            } else {
                // Forma A: declarar la variable de control en el scope del para
                string valorInit = extraerValorInit(init->izq);
                if (!scope_declarar(init->valor, init->valorTipo, init->linea)) {
                    registrarError("Variable '" + init->valor + "' ya declarada en este scope",
                                   init->linea, init->columna, init->valor);
                }
                if (init->izq) {
                    string tipoExpr = analizar_expresion(const_cast<NodoAST*>(init->izq));
                    if (!tiposCompatibles(init->valorTipo, tipoExpr)) {
                        registrarError("No se puede asignar tipo '" + tipoExpr +
                                       "' a variable de tipo '" + init->valorTipo + "'",
                                       init->linea, init->columna, init->valor);
                    }
                }
                tabla_agregar(init->valor, init->valorTipo, valorInit,
                              gS->ambitoActual, init->linea, init->columna, "variable");
            }
        }
        string tipoCond = analizar_expresion(nodo->der);
        if (tipoCond != "?" && tipoCond != "logico") {
            int lin = nodo->der ? nodo->der->linea   : nodo->linea;
            int col = nodo->der ? nodo->der->columna : nodo->columna;
            registrarError("La condicion debe ser de tipo 'logico', se encontro '" + tipoCond + "'",
                           lin, col, "para");
        }
        analizar_bloque(nodo->extra, false);
        scope_cerrar();
        break;
    }

    // ── Sentencia retornar ────────────────────────────────────────────────────
    //    Algoritmo:
    //    El tipo de retorno esperado se recibe como atributo heredado
    //    (tipoRetFunActual). Se verifica:
    //    - Funcion vacio  -> no debe haber expresion de retorno
    //    - Funcion no-vacio -> debe haber expresion; su tipo debe ser compatible
    case NODO_RETORNAR: {
        const string& tipoFun = gS->tipoRetFunActual;
        if (tipoFun == "vacio") {
            if (nodo->izq) {
                registrarError("Funcion 'vacio' no debe retornar un valor",
                               nodo->linea, nodo->columna, "retornar");
            }
        } else {
            if (!nodo->izq) {
                registrarError("Funcion no-vacio debe retornar un valor",
                               nodo->linea, nodo->columna, "retornar");
            } else {
                string tipoExpr = analizar_expresion(nodo->izq);
                if (!tiposCompatibles(tipoFun, tipoExpr)) {
                    registrarError("Funcion retorna '" + tipoFun +
                                   "' pero se devuelve '" + tipoExpr + "'",
                                   nodo->linea, nodo->columna, "retornar");
                }
            }
        }
        break;
    }

    // ── Sentencia escribir — acepta cualquier tipo y cantidad de argumentos ──
    case NODO_ESCRIBIR: {
        NodoAST* arg = nodo->izq ? nodo->izq->izq : nullptr;
        while (arg) {
            analizar_expresion(arg);
            arg = arg->siguiente;
        }
        break;
    }

    // ── Sentencia leer — la variable debe estar declarada ────────────────────
    case NODO_LEER: {
        if (!scope_buscar(nodo->valor)) {
            registrarError("Variable '" + nodo->valor + "' no declarada en este scope",
                           nodo->linea, nodo->columna, nodo->valor);
        }
        break;
    }

    // ── limpPantalla — sin validacion semantica ───────────────────────────────
    case NODO_LIMPPANTALLA:
        break;

    // ── Incremento / decremento standalone ───────────────────────────────────
    case NODO_INC_POST:
    case NODO_DEC_POST: {
        if (!nodo->izq) break;
        NodoVariable* var = scope_buscar(nodo->izq->valor);
        if (!var) {
            registrarError("Variable '" + nodo->izq->valor + "' no declarada en este scope",
                           nodo->izq->linea, nodo->izq->columna, nodo->izq->valor);
            break;
        }
        if (!esNumerico(var->tipo)) {
            string opStr = (nodo->tipo == NODO_INC_POST) ? "++" : "--";
            registrarError("Operador '" + opStr + "' no aplica a tipo '" + var->tipo + "'",
                           nodo->linea, nodo->columna, nodo->izq->valor);
        }
        break;
    }

    case NODO_ERROR:
    default:
        break;
    }
}

// Analiza la declaracion de una funcion:
// 1. Construir la lista de parametros
// 2. Registrar en tabla interna (deteccion de redefinicion)
// 3. Registrar en tabla de simbolos unificada (funcion + cada parametro)
// 4. Abrir scope, declarar parametros, analizar cuerpo, cerrar scope
static void analizar_funcion(NodoAST* nodo) {
    if (!nodo) return;

    // Construir lista de parametros para la tabla interna
    int        aridad       = 0;
    NodoParam* primerParam  = nullptr;
    NodoParam* ultimoParam  = nullptr;

    const NodoAST* p = nodo->izq;
    while (p) {
        NodoParam* np = new NodoParam;
        np->nombre    = p->valor;
        np->tipo      = p->valorTipo;
        np->siguiente = nullptr;
        if (!primerParam) primerParam = np;
        else              ultimoParam->siguiente = np;
        ultimoParam = np;
        aridad++;
        p = p->siguiente;
    }

    // Construir firma de parametros para la tabla de simbolos
    string firma = "(";
    const NodoAST* pp = nodo->izq;
    bool primero = true;
    while (pp) {
        if (!primero) firma += ", ";
        firma += pp->valorTipo + " " + pp->valor;
        primero = false;
        pp = pp->siguiente;
    }
    firma += ")";

    // Registrar en tabla interna de funciones
    if (!tablaFunc_agregar(nodo->valor, nodo->valorTipo, primerParam, aridad, nodo->linea)) {
        NodoFuncion* existente = tablaFunc_buscar(nodo->valor);
        registrarError("Funcion '" + nodo->valor + "' ya fue declarada (linea " +
                       to_string(existente ? existente->linea : 0) + ")",
                       nodo->linea, nodo->columna, nodo->valor);
        liberar_params(primerParam);
    }

    // Registrar la funcion en la tabla de simbolos unificada
    tabla_agregar(nodo->valor, nodo->valorTipo, firma,
                  "global", nodo->linea, nodo->columna, "funcion");

    // Abrir scope de la funcion, registrar parametros y analizar cuerpo
    scope_abrir();
    string ambitoAnterior  = gS->ambitoActual;
    gS->ambitoActual       = nodo->valor;

    p = nodo->izq;
    while (p) {
        scope_declarar(p->valor, p->valorTipo, p->linea);
        tabla_agregar(p->valor, p->valorTipo, "-",
                      gS->ambitoActual, p->linea, p->columna, "parametro");
        p = p->siguiente;
    }

    string tipoRetAnterior = gS->tipoRetFunActual;
    gS->tipoRetFunActual   = nodo->valorTipo;

    analizar_bloque(nodo->der, false);

    gS->tipoRetFunActual = tipoRetAnterior;
    gS->ambitoActual     = ambitoAnterior;
    scope_cerrar();
}

// Recorre el programa completo (declaraciones de nivel superior).
static void analizar_programa(NodoAST* nodo) {
    if (!nodo) return;
    scope_abrir();

    NodoAST* decl = nodo->izq;
    while (decl) {
        if (decl->tipo == NODO_FUNC) {
            analizar_funcion(decl);
        } else if (decl->tipo == NODO_DECL_VAR) {
            analizar_sentencia(decl);
        }
        decl = decl->siguiente;
    }

    scope_cerrar();
}

// ##############################################################################
//  SECCION 8 — Funciones publicas de la interfaz
// ##############################################################################

void semantico_init(EstadoSemantico& s, NodoAST* raiz) {
    s.raiz               = raiz;
    s.hayError           = false;
    s.funciones.cabeza   = nullptr;
    s.funciones.tamanio  = 0;
    s.tabla.cabeza       = nullptr;
    s.tabla.tamanio      = 0;
    s.tabla.contadorId   = 0;
    s.scopeActual        = nullptr;
    s.tipoRetFunActual   = "";
    s.ambitoActual       = "global";
    listaErrores_init(s.errores);
}

void semantico_analizar(EstadoSemantico& s) {
    gS = &s;
    registrar_builtins();
    if (s.raiz) analizar_programa(s.raiz);
    gS = nullptr;
}

void semantico_liberar(EstadoSemantico& s) {
    tablaFunc_liberar(s.funciones);
    tabla_liberar(s.tabla);
    listaErrores_liberar(s.errores);
    while (s.scopeActual) {
        NodoScope* viejo  = s.scopeActual;
        s.scopeActual     = viejo->padre;
        NodoVariable* v   = viejo->vars;
        while (v) { NodoVariable* sig = v->siguiente; delete v; v = sig; }
        delete viejo;
    }
    s.raiz             = nullptr;
    s.hayError         = false;
    s.tipoRetFunActual = "";
    s.ambitoActual     = "";
}

void semantico_imprimirResultado(const EstadoSemantico& s) {
    cout << "\n";
    cout << "+-----------------------------------------------------+\n";
    cout << "|     Resultado del analisis semantico                |\n";
    cout << "+-----------------------------------------------------+\n";
    if (!listaErrores_hayErrores(s.errores)) {
        cout << "  [OK] Analisis semantico completado sin errores.\n";
        cout << "  El programa es semanticamente correcto.\n";
    } else {
        listaErrores_imprimir(s.errores);
        cout << "  Total de errores semanticos: " << s.errores.tamanio << "\n";
        cout << "  [ERROR] El programa contiene errores semanticos.\n";
    }
    cout << "\n";
}

// Imprime la tabla de simbolos unificada con los campos:
//   ID | Nombre | Tipo | Valor | Ambito | Ubicacion | Categoria
void semantico_imprimirTabla(const EstadoSemantico& s) {
    // Anchos de columna fijos
    const int wId     = 4;
    const int wNom    = 18;
    const int wTipo   = 9;
    const int wValor  = 22;
    const int wAmb    = 14;
    const int wUbic   = 9;
    const int wCat    = 10;

    string sep = "+" + string(wId,'-') + "+" + string(wNom,'-') + "+"
                     + string(wTipo,'-') + "+" + string(wValor,'-') + "+"
                     + string(wAmb,'-') + "+" + string(wUbic,'-') + "+"
                     + string(wCat,'-') + "+";

    cout << "\n" << sep << "\n";
    cout << "|" << left << setw(wId)   << " ID"
         << "|" << left << setw(wNom)  << " Nombre"
         << "|" << left << setw(wTipo) << " Tipo"
         << "|" << left << setw(wValor)<< " Valor"
         << "|" << left << setw(wAmb)  << " Ambito"
         << "|" << left << setw(wUbic) << " Ubic."
         << "|" << left << setw(wCat)  << " Categ."
         << "|\n";
    cout << sep << "\n";

    if (!s.tabla.cabeza) {
        cout << "|  (tabla vacia)                                                              |\n";
    } else {
        const EntradaTabla* e = s.tabla.cabeza;
        while (e) {
            string ubic   = "L" + to_string(e->linea) + ",C" + to_string(e->columna);
            string ambMos = (e->ambito == "global") ? "global" : "local";
            // Truncar campos largos para que quepan en la columna
            string nom   = e->nombre; if ((int)nom.size()   > wNom-2)   nom   = nom.substr(0, wNom-2);
            string valor = e->valor;  if ((int)valor.size() > wValor-2) valor = valor.substr(0, wValor-2);

            cout << "|" << right << setw(wId-1)   << e->id    << " "
                 << "|" << " "   << left  << setw(wNom-1)  << nom
                 << "|" << " "   << left  << setw(wTipo-1) << e->tipo
                 << "|" << " "   << left  << setw(wValor-1)<< valor
                 << "|" << " "   << left  << setw(wAmb-1)  << ambMos
                 << "|" << " "   << left  << setw(wUbic-1) << ubic
                 << "|" << " "   << left  << setw(wCat-1)  << e->categoria
                 << "|\n";
            e = e->siguiente;
        }
    }
    cout << sep << "\n";
    cout << "  Total de simbolos registrados: " << s.tabla.tamanio << "\n\n";
}
