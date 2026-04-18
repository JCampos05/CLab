#include "parser.h"
#include "nodo.h"
#include "token.h"
#include <iostream>
#include <string>
using namespace std;

//####################################### SECCION 1 -- Estado interno y operaciones primitivas sobre tokens #######################################
//  El parser trabaja con un puntero NodoToken* que avanza por la lista
//  enlazada construida por el lexico. Nunca relee el archivo fuente.

// Puntero al estado activo (se pasa por referencia en todas las funciones,
// pero se usa un puntero global interno para no contaminar las firmas)
static EstadoParser* gP = nullptr;

// Tipo de retorno de la funcion que se esta parseando actualmente.
// Se actualiza en parser_func y se consulta en parser_retornar.
// "" significa que estamos fuera de cualquier funcion.
static string gTipoRetornoActual = "";

// Indica si dentro de la funcion actual ya se encontro al menos un
// 'retornar <expr>;' valido. Solo tiene sentido cuando
// gTipoRetornoActual != "vacio" && gTipoRetornoActual != "".
// Se resetea a false al entrar a cada nueva funcion.
static bool gRetornarVisto = false;

// -- peek ---------------------------------------------------------------------
// Devuelve el token actual sin consumirlo.
// Si llegamos al fin de la lista devuelve un token FIN_ARCHIVO sintetico.
static Token peek() {
    if (gP->actual != nullptr)
        return gP->actual->token;
    Token eof;
    eof.tipo    = FIN_ARCHIVO;
    eof.lexema  = "";
    eof.linea   = -1;
    eof.columna = -1;
    return eof;
}

// -- avanzar ------------------------------------------------------------------
// Mueve el puntero al siguiente token de la lista.
static void avanzar() {
    if (gP->actual != nullptr)
        gP->actual = gP->actual->siguiente;
}

// -- esTipo -------------------------------------------------------------------
// Comprueba el TokenType del token actual.
static bool esTipo(TokenType t) {
    return peek().tipo == t;
}

// -- esLexema -----------------------------------------------------------------
// Comprueba el lexema exacto del token actual.
static bool esLexema(const string& s) {
    return peek().lexema == s;
}

// -- consumir -----------------------------------------------------------------
// Si el token actual tiene el tipo esperado, lo consume y devuelve true.
// En caso contrario no avanza y devuelve false.
static bool consumir(TokenType t) {
    if (esTipo(t)) { avanzar(); return true; }
    return false;
}

// -- consumirLexema ------------------------------------------------------------
// Consume si el lexema coincide exactamente.
static bool consumirLexema(const string& s) {
    if (esLexema(s)) { avanzar(); return true; }
    return false;
}

// -- registrarError ------------------------------------------------------------
// Registra un error sintactico en la lista de errores del parser.
static void registrarError(const string& mensaje) {
    Token t = peek();
    listaErrores_agregar(
        gP->errores,
        mensaje,
        t.linea,
        t.columna,
        t.lexema.empty() ? "<fin de archivo>" : t.lexema
    );
    gP->hayError = true;
}

// -- sincronizar ---------------------------------------------------------------
// Recuperacion -> avanza tokens hasta encontrar ';' o '}'
// o fin de archivo. Permite continuar el analisis tras un error.
static void sincronizar() {
    while (!esTipo(FIN_ARCHIVO)) {
        if (esTipo(PUNTO_COMA)) { avanzar(); return; }
        if (esTipo(LLAVE_CIE))  { return; }
        avanzar();
    }
}

// Recuperacion a nivel de bloque: primero avanza hasta encontrar el '{' de
// apertura, luego cuenta llaves hasta cerrarlo completamente (profundidad=0).
// Util cuando el error ocurre en el encabezado de func/si/para/mientras.
static void sincronizarBloque() {
    // Fase 1: avanzar hasta encontrar el '{' de apertura (o EOF)
    while (!esTipo(FIN_ARCHIVO) && !esTipo(LLAVE_ABR)) {
        avanzar();
    }
    if (esTipo(FIN_ARCHIVO)) return;

    // Fase 2: consumir el bloque completo contando llaves
    int prof = 0;
    while (!esTipo(FIN_ARCHIVO)) {
        if (esTipo(LLAVE_ABR)) {
            prof++; avanzar();
        } else if (esTipo(LLAVE_CIE)) {
            prof--; avanzar();
            if (prof == 0) return;
        } else {
            avanzar();
        }
    }
}

// -- esTiposDato ---------------------------------------------------------------
// Devuelve true si el token actual es una palabra reservada de tipo de dato.
static bool esTipoDato() {
    TokenType t = peek().tipo;
    return t == PAL_RSV_ENTERO  || t == PAL_RSV_DECI   ||
            t == PAL_RSV_CARAC   || t == PAL_RSV_CAD    ||
            t == PAL_RSV_LOGICO  || t == PAL_RSV_VACIO;
}

// -- consumirTipo -------------------------------------------------------------
// Consume el token de tipo de dato y devuelve su lexema ("entero", "deci"...).
// Devuelve "" si no hay tipo de dato en la posicion actual.
static string consumirTipo() {
    if (!esTipoDato()) return "";
    string lex = peek().lexema;
    avanzar();
    return lex;
}

// #######################################  SECCION 2 -- Declaraciones adelantadas de las reglas gramaticales #######################################

static NodoAST* parser_programa();
static NodoAST* parser_func();
static NodoAST* parser_params();
static NodoAST* parser_bloque(const string& contexto = "el bloque");
static NodoAST* parser_sentencia();
static NodoAST* parser_declVar();
static NodoAST* parser_asignacion(const string& nombre, int linea, int col);
static NodoAST* parser_llamadaStmt(const string& nombre, int linea, int col);
static NodoAST* parser_si();
static NodoAST* parser_para();
static NodoAST* parser_mientras();
static NodoAST* parser_retornar();
static NodoAST* parser_escribir();
static NodoAST* parser_leer();
static NodoAST* parser_limpPantalla();
static NodoAST* parser_expr();
static NodoAST* parser_expLogica();
static NodoAST* parser_expRelacional();
static NodoAST* parser_expAritm();
static NodoAST* parser_term();
static NodoAST* parser_unario();
static NodoAST* parser_primario();
static NodoAST* parser_args();


//  ####################################### SECCION 3 -- Reglas gramaticales  #######################################

// -----------------------------------------------------------------------------
//  programa -> declaracion*
//  declaracion -> func | declVar (globales)
//
//  En idealC el nivel superior admite declaraciones de funciones y variables
//  globales. El parser las encadena con ->siguiente.
// -----------------------------------------------------------------------------
static NodoAST* parser_programa() {
    Token t = peek();
    NodoAST* raiz = nodo_crear(NODO_PROGRAMA, "", t.linea, t.columna);

    NodoAST* ultimo = nullptr;

    while (!esTipo(FIN_ARCHIVO)) {
        NodoAST* decl = nullptr;

        if (esTipo(PAL_RSV_FUNC)) {
            decl = parser_func();
        } else if (esTipoDato()) {
            decl = parser_declVar();
        } else {
            // Token inesperado a nivel global -- registrar error y recuperar
            registrarError(
                "Se esperaba 'func' o un tipo de dato, se encontro '"
                + peek().lexema + "'"
            );
            // Avanzar hasta { para parsear el bloque y capturar sus errores internos
            while (!esTipo(FIN_ARCHIVO) && !esTipo(LLAVE_ABR) && !esTipo(PAL_RSV_FUNC))
                avanzar();
            if (esTipo(LLAVE_ABR))
                parser_bloque("el bloque"); // parsear para reportar errores internos
            continue;
        }

        if (decl == nullptr) continue;

        if (raiz->izq == nullptr) {
            raiz->izq = decl;
        } else {
            ultimo->siguiente = decl;
        }
        ultimo = decl;
    }

    return raiz;
}

// -----------------------------------------------------------------------------
//  func -> 'func' tipo IDENT '(' params ')' bloque
// -----------------------------------------------------------------------------
static NodoAST* parser_func() {
    Token tk = peek();
    if (!consumir(PAL_RSV_FUNC)) return nullptr;

    // tipo de retorno
    // tipo de retorno
    string tipoRet = consumirTipo();
    if (tipoRet.empty()) {
        registrarError("Se esperaba un tipo de retorno despues de 'func'");
        // Si el token actual parece un tipo mal escrito (IDENT) seguido del
        // nombre real (otro IDENT o palabra reservada valida como nombre),
        // saltar el token invalido para recuperar el nombre correcto
        if (esTipo(IDENTIFICADOR)) {
            avanzar(); // saltar el "tipo" invalido (ej: "ento")
        }
        // Verificar que ahora tenemos algo que puede ser nombre
        bool hayNombre = esTipo(IDENTIFICADOR) ||
                            esTipo(PAL_RSV_SUMA)  || esTipo(PAL_RSV_RESTA) ||
                            esTipo(PAL_RSV_MUL)   || esTipo(PAL_RSV_DIV);
        if (!hayNombre) {
            sincronizarBloque();
            return nullptr;
        }
        tipoRet = "?"; // tipo desconocido, continuar
    }

    // nombre de la funcion -- acepta IDENTIFICADOR y palabras reservadas
    // que en idealC se usan como funciones nativas (suma, resta, mul, div)
    bool esNombreValido = esTipo(IDENTIFICADOR) ||
                            esTipo(PAL_RSV_SUMA)  || esTipo(PAL_RSV_RESTA) ||
                            esTipo(PAL_RSV_MUL)   || esTipo(PAL_RSV_DIV);
    if (!esNombreValido) {
        registrarError("Se esperaba el nombre de la funcion");
        sincronizarBloque();
        return nullptr;
    }
    string nombre = peek().lexema;
    int linNom = peek().linea, colNom = peek().columna;
    avanzar();

    // parentesis apertura
    if (!consumir(PAREN_ABR)) {
        registrarError("Se esperaba '(' despues del nombre de la funcion '" + nombre + "'");
        // Avanzar hasta ( o { para intentar recuperar
        while (!esTipo(FIN_ARCHIVO) && !esTipo(PAREN_ABR) && !esTipo(LLAVE_ABR))
            avanzar();
        if (esTipo(FIN_ARCHIVO)) return nullptr;
        if (esTipo(LLAVE_ABR)) {
            // Sin parentesis -- parsear bloque directamente para capturar errores
            parser_bloque("el cuerpo de la funcion");
            return nullptr;
        }
        consumir(PAREN_ABR);
    }

    // parametros (pueden ser 0)
    NodoAST* params = nullptr;
    if (!esTipo(PAREN_CIE))
        params = parser_params();

    if (!consumir(PAREN_CIE)) {
        registrarError("Se esperaba ')' al cerrar los parametros de '" + nombre + "'");
        // Avanzar hasta { para parsear el bloque y capturar sus errores
        while (!esTipo(FIN_ARCHIVO) && !esTipo(LLAVE_ABR))
            avanzar();
        if (esTipo(FIN_ARCHIVO)) return nullptr;
    }

    // Registrar el tipo de retorno actual para que parser_retornar lo consulte
    string tipoRetornoAnterior  = gTipoRetornoActual;
    bool   retornarVistoAnterior = gRetornarVisto;

    gTipoRetornoActual = tipoRet;
    gRetornarVisto     = false;   // resetear para esta nueva funcion

    // cuerpo -- siempre intentar parsearlo para reportar errores internos
    NodoAST* cuerpo = parser_bloque("el cuerpo de la funcion '" + nombre + "'");

    // Validar que funciones no-vacio tengan al menos un 'retornar <expr>;'
    // Solo lo reportamos si no hubo ya otros errores en el cuerpo (evitar ruido)
    // y el tipo es conocido (no "?")
    if (tipoRet != "vacio"  &&
        tipoRet != "?"      &&
        tipoRet != ""       &&
        !gRetornarVisto     &&
        cuerpo != nullptr) {
        registrarError(
            "La funcion '" + nombre + "' es de tipo '" + tipoRet
            + "' pero no contiene 'retornar <expresion>;'"
        );
    }

    // Restaurar el tipo de retorno anterior (soporte para funciones anidadas)
    gTipoRetornoActual = tipoRetornoAnterior;
    gRetornarVisto     = retornarVistoAnterior;

    if (cuerpo == nullptr) return nullptr;

    NodoAST* nodo     = nodo_crear(NODO_FUNC, nombre, linNom, colNom);
    nodo->valorTipo   = tipoRet;
    nodo->izq         = params;
    nodo->der         = cuerpo;
    return nodo;
}

// -----------------------------------------------------------------------------
//  params -> tipo IDENT (',' tipo IDENT)*
// -----------------------------------------------------------------------------
static NodoAST* parser_params() {
    NodoAST* primero = nullptr;
    NodoAST* ultimo  = nullptr;

    do {
        string tipo = consumirTipo();
        if (tipo.empty()) {
            registrarError("Se esperaba un tipo de dato en la lista de parametros");
            break;
        }
        if (!esTipo(IDENTIFICADOR)) {
            registrarError("Se esperaba el nombre del parametro despues del tipo '" + tipo + "'");
            break;
        }
        string nombre = peek().lexema;
        int lin = peek().linea, col = peek().columna;
        avanzar();

        NodoAST* param     = nodo_crear(NODO_PARAM, nombre, lin, col);
        param->valorTipo   = tipo;

        if (primero == nullptr) {
            primero = param;
        } else {
            ultimo->siguiente = param;
        }
        ultimo = param;

    } while (consumir(COMA));

    return primero;
}

// -----------------------------------------------------------------------------
//  bloque -> '{' sentencia* '}'
// -----------------------------------------------------------------------------
static NodoAST* parser_bloque(const string& contexto) {
    Token tk = peek();
    if (!consumir(LLAVE_ABR)) {
        registrarError("Se esperaba '{' para abrir " + contexto + " -- se encontro '" + peek().lexema + "'");
        sincronizarBloque();
        return nullptr;
    }

    NodoAST* bloque = nodo_crear(NODO_BLOQUE, "", tk.linea, tk.columna);
    NodoAST* ultimo = nullptr;

    while (!esTipo(LLAVE_CIE) && !esTipo(FIN_ARCHIVO)) {
        NodoAST* sent = parser_sentencia();
        if (sent == nullptr) { sincronizar(); continue; }

        if (bloque->izq == nullptr) {
            bloque->izq = sent;
        } else {
            ultimo->siguiente = sent;
        }
        ultimo = sent;
    }

    if (!consumir(LLAVE_CIE)) {
        registrarError("Se esperaba '}' para cerrar " + contexto);
    }

    return bloque;
}

// -----------------------------------------------------------------------------
//  sentencia -> declVar | si | para | mientras | retornar | escribir
//            | leer | limpPantalla | asignacion | llamada | expAritm ';'
// -----------------------------------------------------------------------------
static NodoAST* parser_sentencia() {
    Token t = peek();

    // -- Declaracion de variable (empieza con tipo de dato) --------------------
    if (esTipoDato()) {
        return parser_declVar();
    }

    // -- Estructuras de control ------------------------------------------------
    if (esTipo(PAL_RSV_SI))         return parser_si();
    if (esTipo(PAL_RSV_PARA))       return parser_para();
    if (esTipo(PAL_RSV_MIENTRAS))   return parser_mientras();
    if (esTipo(PAL_RSV_RETORNAR))   return parser_retornar();

    // -- E/S -------------------------------------------------------------------
    if (esTipo(PAL_RSV_ESCRIBIR))   return parser_escribir();
    if (esTipo(PAL_RSV_LEER))       return parser_leer();
    if (esTipo(PAL_RSV_LIMPPANTALLA)) return parser_limpPantalla();

    // -- IDENT o nombre de func-nativa usado como variable o llamada -----------
    // PAL_RSV_SUMA/RESTA/MUL/DIV pueden ser nombres de variable en idealC
    // cuando el lexico los tokeniza asi por coincidencia con palabras reservadas
    {
        bool esFuncNativaStmt = (esTipo(PAL_RSV_SUMA) || esTipo(PAL_RSV_RESTA) ||
                                    esTipo(PAL_RSV_MUL)  || esTipo(PAL_RSV_DIV));
        if (esTipo(IDENTIFICADOR) || esFuncNativaStmt) {
    string nombre = t.lexema;
        int lin = t.linea, col = t.columna;
        avanzar();

        if (esTipo(OP_ASIGNACION)) {
            // nombre = expr ;
            return parser_asignacion(nombre, lin, col);
        }
        if (esTipo(PAREN_ABR)) {
            // nombre ( args ) ;
            return parser_llamadaStmt(nombre, lin, col);
        }
        // incremento/decremento standalone:  nombre++ ; o nombre-- ;
        if (esTipo(OP_INC) || esTipo(OP_DEC)) {
            TipoNodo tn = esTipo(OP_INC) ? NODO_INC_POST : NODO_DEC_POST;
            avanzar();
            if (!consumir(PUNTO_COMA))
                registrarError("Se esperaba ';' despues de '" + nombre + (tn==NODO_INC_POST?"++":"--") + "'");
            NodoAST* id = nodo_crear(NODO_IDENTIFICADOR, nombre, lin, col);
            NodoAST* op = nodo_crear(tn, "", lin, col);
            op->izq = id;
            return op;
        }

        registrarError("Se esperaba '=', '(' o '++'/'-' despues de '" + nombre + "'");
        sincronizar();
        return nullptr;
    }
    } // fin bloque esFuncNativaStmt

    // -- Expresion matematica standalone (raro pero valido) --------------------
    if (esTipo(LIT_ENTERO) || esTipo(LIT_DECIMAL) || esTipo(PAREN_ABR)) {
        NodoAST* expr = parser_expr();
        if (!consumir(PUNTO_COMA))
            registrarError("Se esperaba ';' al final de la expresion");
        return expr;
    }

    // -- Token inesperado ------------------------------------------------------
    registrarError("Sentencia no reconocida: '" + t.lexema + "'");
    sincronizar();
    return nullptr;
}

// -----------------------------------------------------------------------------
//  declVar -> tipo IDENT ('=' expr)? (',' IDENT ('=' expr)?)* ';'
//  Soporta declaracion multiple: entero a = 1, b, c = 3;
// -----------------------------------------------------------------------------
static NodoAST* parser_declVar() {
    string tipo = consumirTipo();
    if (tipo.empty()) return nullptr;

    // Primera variable obligatoria
    // Acepta IDENTIFICADOR o palabras que el lexico tokeniza como PAL_RSV_*
    // pero que el programador usa como nombre de variable (ej: "suma", "resta")
    bool esNomVar = esTipo(IDENTIFICADOR) ||
                    esTipo(PAL_RSV_SUMA)   || esTipo(PAL_RSV_RESTA) ||
                    esTipo(PAL_RSV_MUL)    || esTipo(PAL_RSV_DIV);
    if (!esNomVar) {
        registrarError("Se esperaba un identificador despues del tipo '" + tipo + "'");
        sincronizar();
        return nullptr;
    }

    string nombre = peek().lexema;
    int lin = peek().linea, col = peek().columna;
    avanzar();

    NodoAST* primero = nodo_crear(NODO_DECL_VAR, nombre, lin, col);
    primero->valorTipo = tipo;

    // Inicializador opcional
    if (consumir(OP_ASIGNACION)) {
        NodoAST* init = parser_expr();
        if (init == nullptr)
            registrarError("Se esperaba una expresion despues de '=' en la declaracion de '" + nombre + "'");
        primero->izq = init;
    }

    // Declaraciones multiples separadas por coma
    NodoAST* ultimo = primero;
    while (consumir(COMA)) {
        bool esNom2 = esTipo(IDENTIFICADOR) ||
                        esTipo(PAL_RSV_SUMA) || esTipo(PAL_RSV_RESTA) ||
                        esTipo(PAL_RSV_MUL)  || esTipo(PAL_RSV_DIV);
        if (!esNom2) {
            registrarError("Se esperaba un identificador despues de ','");
            break;
        }
        string nom2 = peek().lexema;
        int li2 = peek().linea, co2 = peek().columna;
        avanzar();

        NodoAST* siguiente = nodo_crear(NODO_DECL_VAR, nom2, li2, co2);
        siguiente->valorTipo = tipo;

        if (consumir(OP_ASIGNACION)) {
            NodoAST* init2 = parser_expr();
            if (init2 == nullptr)
                registrarError("Se esperaba una expresion en la declaracion de '" + nom2 + "'");
            siguiente->izq = init2;
        }
        ultimo->siguiente = siguiente;
        ultimo = siguiente;
    }

    if (!consumir(PUNTO_COMA))
        registrarError("Se esperaba ';' al final de la declaracion de '" + nombre + "'");

    return primero;
}

// -----------------------------------------------------------------------------
//  asignacion -> IDENT '=' expr ';'
//  (el IDENT ya fue consumido por parser_sentencia)
// -----------------------------------------------------------------------------
static NodoAST* parser_asignacion(const string& nombre, int linea, int col) {
    if (!consumir(OP_ASIGNACION)) {
        registrarError("Se esperaba '=' en la asignacion a '" + nombre + "'");
        sincronizar();
        return nullptr;
    }

    NodoAST* valor = parser_expr();
    if (valor == nullptr) {
        registrarError("Se esperaba una expresion despues de '=' en la asignacion a '" + nombre + "'");
        sincronizar();
        return nullptr;
    }

    if (!consumir(PUNTO_COMA))
        registrarError("Se esperaba ';' al final de la asignacion a '" + nombre + "'");

    NodoAST* nodo = nodo_crear(NODO_ASIGNACION, nombre, linea, col);
    nodo->izq = valor;
    return nodo;
}

// -----------------------------------------------------------------------------
//  llamadaStmt -> IDENT '(' args ')' ';'
//  (el IDENT ya fue consumido por parser_sentencia)
// -----------------------------------------------------------------------------
static NodoAST* parser_llamadaStmt(const string& nombre, int linea, int col) {
    if (!consumir(PAREN_ABR)) {
        registrarError("Se esperaba '(' en la llamada a '" + nombre + "'");
        sincronizar();
        return nullptr;
    }

    NodoAST* args = nullptr;
    if (!esTipo(PAREN_CIE))
        args = parser_args();

    if (!consumir(PAREN_CIE))
        registrarError("Se esperaba ')' al cerrar los argumentos de '" + nombre + "'");

    if (!consumir(PUNTO_COMA))
        registrarError("Se esperaba ';' al final de la llamada a '" + nombre + "'");

    NodoAST* nodo = nodo_crear(NODO_LLAMADA, nombre, linea, col);
    nodo->izq = args;
    return nodo;
}

// -----------------------------------------------------------------------------
//  si -> 'si' '(' expr ')' bloque ('sino' bloque)?
// -----------------------------------------------------------------------------
static NodoAST* parser_si() {
    Token tk = peek();
    if (!consumir(PAL_RSV_SI)) return nullptr;

    if (!consumir(PAREN_ABR)) {
        registrarError("Se esperaba '(' despues de 'si'");
        sincronizarBloque();
        return nullptr;
    }

    NodoAST* cond = parser_expr();
    if (cond == nullptr) {
        registrarError("Se esperaba una expresion de condicion en 'si'");
        sincronizarBloque();
        return nullptr;
    }

    if (!consumir(PAREN_CIE)) {
        registrarError("Se esperaba ')' despues de la condicion del 'si'");
        sincronizarBloque();
        return nullptr;
    }

    NodoAST* entonces = parser_bloque("el cuerpo del 'si'");
    if (entonces == nullptr) return nullptr;

    NodoAST* sinoBloq = nullptr;
    if (consumir(PAL_RSV_SINO)) {
        sinoBloq = parser_bloque("el cuerpo del 'sino'");
    }

    NodoAST* nodo = nodo_crear(NODO_SI, "", tk.linea, tk.columna);
    nodo->izq   = cond;
    nodo->der   = entonces;
    nodo->extra = sinoBloq;
    return nodo;
}

// -----------------------------------------------------------------------------
//  para -> 'para' '(' declVar_sin_punto_coma ';' expr ';' IDENT ('++'|'--') ')' bloque
//
//  La inicializacion del 'para' es una declaracion de variable sin ';' final
//  (el ';' lo consume la propia regla del para).
// -----------------------------------------------------------------------------
static NodoAST* parser_para() {
    Token tk = peek();
    if (!consumir(PAL_RSV_PARA)) return nullptr;

    if (!consumir(PAREN_ABR)) {
        registrarError("Se esperaba '(' despues de 'para'");
        sincronizarBloque();
        return nullptr;
    }

    // -- Inicializacion: [tipo] IDENT = expr  (sin ';') ----------------------
    // Acepta dos formas:
    //   forma A: tipo IDENT = expr   (declara en el for: para (entero i = 0; ...))
    //   forma B: IDENT = expr        (variable ya declarada: para (i = 0; ...))
    NodoAST* init = nullptr;
    {
        string tipo = "";
        if (esTipoDato()) tipo = consumirTipo();  // forma A -- tipo opcional

        if (!esTipo(IDENTIFICADOR)) {
            registrarError("Se esperaba un identificador en la inicializacion del 'para'");
            sincronizarBloque();
            return nullptr;
        }
        string nomVar = peek().lexema;
        int linV = peek().linea, colV = peek().columna;
        avanzar();

        init = nodo_crear(NODO_DECL_VAR, nomVar, linV, colV);
        init->valorTipo = tipo;   // "" si fue forma B

        if (consumir(OP_ASIGNACION)) {
            NodoAST* valInit = parser_expr();
            if (valInit == nullptr)
                registrarError("Se esperaba una expresion de inicializacion en 'para'");
            init->izq = valInit;
        }
    }

    // -- Primer ';' ------------------------------------------------------------
    if (!consumir(PUNTO_COMA)) {
        registrarError("Se esperaba ';' despues de la inicializacion del 'para'");
        sincronizarBloque();
        return nullptr;
    }

    // -- Condicion -------------------------------------------------------------
    NodoAST* cond = parser_expr();
    if (cond == nullptr) {
        registrarError("Se esperaba una expresion de condicion en 'para'");
        sincronizarBloque();
        return nullptr;
    }

    // -- Segundo ';' -----------------------------------------------------------
    if (!consumir(PUNTO_COMA)) {
        registrarError("Se esperaba ';' despues de la condicion del 'para'");
        sincronizarBloque();
        return nullptr;
    }

    // -- Incremento: IDENT ++ o IDENT -- --------------------------------------
    if (!esTipo(IDENTIFICADOR)) {
        registrarError("Se esperaba un identificador en el incremento del 'para'");
        sincronizarBloque();
        return nullptr;
    }
    string nomInc = peek().lexema;
    int linI = peek().linea, colI = peek().columna;
    avanzar();

    NodoAST* inc = nullptr;
    if (esTipo(OP_INC) || esTipo(OP_DEC)) {
        TipoNodo tn = esTipo(OP_INC) ? NODO_INC_POST : NODO_DEC_POST;
        avanzar();
        NodoAST* id = nodo_crear(NODO_IDENTIFICADOR, nomInc, linI, colI);
        inc = nodo_crear(tn, "", linI, colI);
        inc->izq = id;
    } else {
        registrarError("Se esperaba '++' o '--' en el incremento del 'para'");
        sincronizarBloque();
        return nullptr;
    }

    if (!consumir(PAREN_CIE)) {
        registrarError("Se esperaba ')' al cerrar el encabezado del 'para'");
        sincronizarBloque();
        return nullptr;
    }

    // -- Cuerpo ----------------------------------------------------------------
    NodoAST* cuerpo = parser_bloque("el cuerpo del 'para'");
    if (cuerpo == nullptr) return nullptr;

    // Almacenamos: izq=init, der=cond, extra=cuerpo
    // El incremento va como primer hijo del bloque (lo insertamos al frente)
    if (cuerpo->izq != nullptr) {
        inc->siguiente = cuerpo->izq;
        cuerpo->izq = inc;
    } else {
        cuerpo->izq = inc;
    }

    NodoAST* nodo = nodo_crear(NODO_PARA, "", tk.linea, tk.columna);
    nodo->izq   = init;
    nodo->der   = cond;
    nodo->extra = cuerpo;
    return nodo;
}

// -----------------------------------------------------------------------------
//  mientras -> 'mientras' '(' expr ')' bloque
// -----------------------------------------------------------------------------
static NodoAST* parser_mientras() {
    Token tk = peek();
    if (!consumir(PAL_RSV_MIENTRAS)) return nullptr;

    if (!consumir(PAREN_ABR)) {
        registrarError("Se esperaba '(' despues de 'mientras'");
        sincronizarBloque();
        return nullptr;
    }

    NodoAST* cond = parser_expr();
    if (cond == nullptr) {
        registrarError("Se esperaba una condicion en 'mientras'");
        sincronizarBloque();
        return nullptr;
    }

    if (!consumir(PAREN_CIE)) {
        registrarError("Se esperaba ')' despues de la condicion del 'mientras'");
        sincronizarBloque();
        return nullptr;
    }

    NodoAST* cuerpo = parser_bloque("el cuerpo del 'mientras'");
    if (cuerpo == nullptr) return nullptr;

    NodoAST* nodo = nodo_crear(NODO_MIENTRAS, "", tk.linea, tk.columna);
    nodo->izq = cond;
    nodo->der = cuerpo;
    return nodo;
}

// -----------------------------------------------------------------------------
//  retornar -> 'retornar' expr? ';'
//
//  Reglas de validacion:
//    1. Funcion vacio          -> solo acepta  'retornar ;'
//    2. Funcion no-vacio       -> solo acepta  'retornar <expr> ;'
//    3. Tipo desconocido ("?") -> acepta ambas formas sin error adicional
//    4. Fuera de funcion ("")  -> no valida (error semantico, no sintactico)
// -----------------------------------------------------------------------------
static NodoAST* parser_retornar() {
    Token tk = peek();
    if (!consumir(PAL_RSV_RETORNAR)) return nullptr;

    NodoAST* expr = nullptr;
    bool tieneExpr = !esTipo(PUNTO_COMA);

    if (!tieneExpr) {
        // -- retornar ; --------------------------------------------------------
        // Solo es valido en funciones vacio o fuera de funcion (contexto "")
        if (gTipoRetornoActual != "vacio" &&
            gTipoRetornoActual != ""      &&
            gTipoRetornoActual != "?") {
            registrarError(
                "La funcion debe retornar un valor de tipo '"
                + gTipoRetornoActual
                + "' -- 'retornar' requiere una expresion"
            );
            gRetornarVisto = true;  // evitar error duplicado en parser_func
        }
    } else {
        // -- retornar <expr> ; -------------------------------------------------
        expr = parser_expr();

        if (expr == nullptr) {
            // parser_expr devolvio null -> token invalido tras 'retornar'
            registrarError("Se esperaba una expresion despues de 'retornar'");
            sincronizar();
            return nullptr;
        }

        if (gTipoRetornoActual == "vacio") {
            registrarError(
                "La funcion es de tipo 'vacio' -- 'retornar' no debe incluir expresion"
            );
        }

        // Marcar que esta funcion ya tiene al menos un retornar con expresion
        if (gTipoRetornoActual != "" && gTipoRetornoActual != "vacio")
            gRetornarVisto = true;
    }

    if (!consumir(PUNTO_COMA))
        registrarError("Se esperaba ';' despues del 'retornar'");

    NodoAST* nodo = nodo_crear(NODO_RETORNAR, "", tk.linea, tk.columna);
    nodo->izq = expr;
    return nodo;
}

// -----------------------------------------------------------------------------
//  escribir -> 'escribir' '(' args ')' ';'
// -----------------------------------------------------------------------------
static NodoAST* parser_escribir() {
    Token tk = peek();
    if (!consumir(PAL_RSV_ESCRIBIR)) return nullptr;

    if (!consumir(PAREN_ABR)) {
        registrarError("Se esperaba '(' despues de 'escribir'");
        sincronizar();
        return nullptr;
    }

    NodoAST* args = nullptr;
    if (!esTipo(PAREN_CIE))
        args = parser_args();

    if (!consumir(PAREN_CIE))
        registrarError("Se esperaba ')' al cerrar 'escribir'");

    if (!consumir(PUNTO_COMA))
        registrarError("Se esperaba ';' despues de 'escribir(...)'");

    NodoAST* nodo = nodo_crear(NODO_ESCRIBIR, "", tk.linea, tk.columna);
    nodo->izq = args;
    return nodo;
}

// -----------------------------------------------------------------------------
//  leer -> 'leer' '(' IDENT ')' ';'
// -----------------------------------------------------------------------------
static NodoAST* parser_leer() {
    Token tk = peek();
    if (!consumir(PAL_RSV_LEER)) return nullptr;

    if (!consumir(PAREN_ABR)) {
        registrarError("Se esperaba '(' despues de 'leer'");
        sincronizar();
        return nullptr;
    }

    if (!esTipo(IDENTIFICADOR)) {
        registrarError("Se esperaba un identificador dentro de 'leer(...)'");
        sincronizar();
        return nullptr;
    }
    string nombre = peek().lexema;
    int lin = peek().linea, col = peek().columna;
    avanzar();

    if (!consumir(PAREN_CIE))
        registrarError("Se esperaba ')' al cerrar 'leer'");

    if (!consumir(PUNTO_COMA))
        registrarError("Se esperaba ';' despues de 'leer(...)'");

    NodoAST* nodo = nodo_crear(NODO_LEER, nombre, lin, col);
    return nodo;
}

// -----------------------------------------------------------------------------
//  limpPantalla -> 'limpPantalla' '(' ')' ';'
// -----------------------------------------------------------------------------
static NodoAST* parser_limpPantalla() {
    Token tk = peek();
    if (!consumir(PAL_RSV_LIMPPANTALLA)) return nullptr;

    if (!consumir(PAREN_ABR))
        registrarError("Se esperaba '(' despues de 'limpPantalla'");

    if (!consumir(PAREN_CIE))
        registrarError("Se esperaba ')' despues de 'limpPantalla('");

    if (!consumir(PUNTO_COMA))
        registrarError("Se esperaba ';' despues de 'limpPantalla()'");

    return nodo_crear(NODO_LIMPPANTALLA, "", tk.linea, tk.columna);
}

// 
//  ####################################### SECCION 4 -- Expresiones (precedencia ascendente, referencia a Mondongo 7.0.1) #######################################
// 

// -----------------------------------------------------------------------------
//  args -> expr (',' expr)*
// -----------------------------------------------------------------------------
static NodoAST* parser_args() {
    Token tk = peek();
    NodoAST* primero = nullptr;
    NodoAST* ultimo  = nullptr;

    do {
        NodoAST* arg = parser_expr();
        if (arg == nullptr) {
            registrarError("Se esperaba una expresion como argumento");
            break;
        }
        if (primero == nullptr) {
            primero = arg;
        } else {
            ultimo->siguiente = arg;
        }
        ultimo = arg;
    } while (consumir(COMA));

    // Envuelve en NODO_ARGS para que el padre sepa que es lista de argumentos
    if (primero != nullptr) {
        NodoAST* contenedor = nodo_crear(NODO_ARGS, "", tk.linea, tk.columna);
        contenedor->izq = primero;
        return contenedor;
    }
    return nullptr;
}

// -----------------------------------------------------------------------------
//  expr -> expLogica
// -----------------------------------------------------------------------------
static NodoAST* parser_expr() {
    return parser_expLogica();
}

// -----------------------------------------------------------------------------
//  expLogica -> expRelacional (('y' | 'o') expRelacional)*
// -----------------------------------------------------------------------------
static NodoAST* parser_expLogica() {
    NodoAST* izq = parser_expRelacional();
    if (izq == nullptr) return nullptr;

    while (esTipo(PAL_RSV_Y) || esTipo(PAL_RSV_O)) {
        string op = peek().lexema;
        int lin = peek().linea, col = peek().columna;
        avanzar();

        NodoAST* der = parser_expRelacional();
        if (der == nullptr) {
            registrarError("Se esperaba una expresion despues del operador logico '" + op + "'");
            break;
        }
        NodoAST* nodo = nodo_crear(NODO_OP_BINARIO, op, lin, col);
        nodo->izq = izq;
        nodo->der = der;
        izq = nodo;
    }
    return izq;
}

// -----------------------------------------------------------------------------
//  expRelacional -> expAritm (('<'|'>'|'<='|'>='|'=='|'!=') expAritm)?
// -----------------------------------------------------------------------------
static NodoAST* parser_expRelacional() {
    NodoAST* izq = parser_expAritm();
    if (izq == nullptr) return nullptr;

    if (esTipo(OP_MENOR)       || esTipo(OP_MAYOR)       ||
        esTipo(OP_MENOR_IGUAL) || esTipo(OP_MAYOR_IGUAL) ||
        esTipo(OP_IGUAL)       || esTipo(OP_DISTINTO)) {

        string op  = peek().lexema;
        int lin = peek().linea, col = peek().columna;
        avanzar();

        NodoAST* der = parser_expAritm();
        if (der == nullptr) {
            registrarError("Se esperaba una expresion despues del operador relacional '" + op + "'");
            return izq;
        }
        NodoAST* nodo = nodo_crear(NODO_OP_BINARIO, op, lin, col);
        nodo->izq = izq;
        nodo->der = der;
        return nodo;
    }
    return izq;
}

// -----------------------------------------------------------------------------
//  expAritm -> term (('+' | '-') term)*
// -----------------------------------------------------------------------------
static NodoAST* parser_expAritm() {
    NodoAST* izq = parser_term();
    if (izq == nullptr) return nullptr;

    while (esTipo(OP_SUMA) || esTipo(OP_RESTA)) {
        string op = peek().lexema;
        int lin = peek().linea, col = peek().columna;
        avanzar();

        NodoAST* der = parser_term();
        if (der == nullptr) {
            registrarError("Se esperaba un termino despues de '" + op + "'");
            break;
        }
        NodoAST* nodo = nodo_crear(NODO_OP_BINARIO, op, lin, col);
        nodo->izq = izq;
        nodo->der = der;
        izq = nodo;
    }
    return izq;
}

// -----------------------------------------------------------------------------
//  term -> unario (('*' | '/' | '%') unario)*
// -----------------------------------------------------------------------------
static NodoAST* parser_term() {
    NodoAST* izq = parser_unario();
    if (izq == nullptr) return nullptr;

    while (esTipo(OP_MUL) || esTipo(OP_DIV) || esTipo(OP_MOD)) {
        string op = peek().lexema;
        int lin = peek().linea, col = peek().columna;
        avanzar();

        NodoAST* der = parser_unario();
        if (der == nullptr) {
            registrarError("Se esperaba un operando despues de '" + op + "'");
            break;
        }
        NodoAST* nodo = nodo_crear(NODO_OP_BINARIO, op, lin, col);
        nodo->izq = izq;
        nodo->der = der;
        izq = nodo;
    }
    return izq;
}

// -----------------------------------------------------------------------------
//  unario -> 'no' unario | '-' unario | primario ('++'|'--')?
// -----------------------------------------------------------------------------
static NodoAST* parser_unario() {
    // Negacion logica
    if (esTipo(PAL_RSV_NO)) {
        string op = peek().lexema;
        int lin = peek().linea, col = peek().columna;
        avanzar();
        NodoAST* operando = parser_unario();
        if (operando == nullptr) {
            registrarError("Se esperaba una expresion despues de 'no'");
            return nullptr;
        }
        NodoAST* nodo = nodo_crear(NODO_OP_UNARIO, op, lin, col);
        nodo->izq = operando;
        return nodo;
    }

    // Negacion aritmetica unaria
    if (esTipo(OP_RESTA)) {
        string op = peek().lexema;
        int lin = peek().linea, col = peek().columna;
        avanzar();
        NodoAST* operando = parser_unario();
        if (operando == nullptr) {
            registrarError("Se esperaba una expresion despues de '-' unario");
            return nullptr;
        }
        NodoAST* nodo = nodo_crear(NODO_OP_UNARIO, "-", lin, col);
        nodo->izq = operando;
        return nodo;
    }

    // Primario con posible ++ o -- postfijo
    NodoAST* base = parser_primario();
    if (base == nullptr) return nullptr;

    if (esTipo(OP_INC)) {
        int lin = peek().linea, col = peek().columna;
        avanzar();
        NodoAST* post = nodo_crear(NODO_INC_POST, "", lin, col);
        post->izq = base;
        return post;
    }
    if (esTipo(OP_DEC)) {
        int lin = peek().linea, col = peek().columna;
        avanzar();
        NodoAST* post = nodo_crear(NODO_DEC_POST, "", lin, col);
        post->izq = base;
        return post;
    }

    return base;
}

// -----------------------------------------------------------------------------
//  primario -> LIT_ENTERO | LIT_DECIMAL | LIT_CADENA
//           | 'verdadero' | 'falso'
//           | IDENT ('(' args ')')?
//           | '(' expr ')'
// -----------------------------------------------------------------------------
static NodoAST* parser_primario() {
    Token t = peek();

    // -- Literales numericos y cadena -----------------------------------------
    if (esTipo(LIT_ENTERO)) {
        avanzar();
        return nodo_crear(NODO_LIT_ENTERO, t.lexema, t.linea, t.columna);
    }
    if (esTipo(LIT_DECIMAL)) {
        avanzar();
        return nodo_crear(NODO_LIT_DECIMAL, t.lexema, t.linea, t.columna);
    }
    if (esTipo(LIT_CADENA)) {
        avanzar();
        return nodo_crear(NODO_LIT_CADENA, t.lexema, t.linea, t.columna);
    }

    // -- Literales booleanos ---------------------------------------------------
    if (esTipo(PAL_RSV_VERDADERO)) {
        avanzar();
        return nodo_crear(NODO_LIT_VERDADERO, t.lexema, t.linea, t.columna);
    }
    if (esTipo(PAL_RSV_FALSO)) {
        avanzar();
        return nodo_crear(NODO_LIT_FALSO, t.lexema, t.linea, t.columna);
    }

    // -- Identificador o llamada en expresion ---------------------------------
    // PAL_RSV_SUMA/RESTA/MUL/DIV se usan en idealC con sintaxis de funcion:
    // mul(a, b), suma(x, y) -- el lexico los tokeniza como PAL_RSV_* pero
    // el parser los acepta aqui como nombres de funcion callable.
    {
        bool esFuncNativa = (esTipo(PAL_RSV_SUMA) || esTipo(PAL_RSV_RESTA) ||
                                esTipo(PAL_RSV_MUL)  || esTipo(PAL_RSV_DIV));

        if (esTipo(IDENTIFICADOR) || esFuncNativa) {
            string nombre = t.lexema;
            int lin = t.linea, col = t.columna;
            avanzar();

            if (consumir(PAREN_ABR)) {
                NodoAST* args = nullptr;
                if (!esTipo(PAREN_CIE))
                    args = parser_args();
                if (!consumir(PAREN_CIE))
                    registrarError("Se esperaba ')' al cerrar la llamada a '" + nombre + "'");
                NodoAST* nodo = nodo_crear(NODO_LLAMADA_EXPR, nombre, lin, col);
                nodo->izq = args;
                return nodo;
            }
            return nodo_crear(NODO_IDENTIFICADOR, nombre, lin, col);
        }
    }

    // -- Expresion entre parentesis --------------------------------------------
    if (esTipo(PAREN_ABR)) {
        avanzar();
        NodoAST* inner = parser_expr();
        if (inner == nullptr)
            registrarError("Se esperaba una expresion dentro de los parentesis");

        if (!consumir(PAREN_CIE))
            registrarError("Se esperaba ')' para cerrar la expresion entre parentesis");

        return inner;
    }

    // -- Nada reconocible -----------------------------------------------------
    // No registramos error aqui porque el llamador (term/unario) lo maneja
    return nullptr;
}

// #######################################  SECCION 5 -- Funciones publicas de la interfaz  #######################################

// -----------------------------------------------------------------------------
//  parser_init -- vincula el parser a la lista de tokens del lexico
// -----------------------------------------------------------------------------
void parser_init(EstadoParser& p, const ListaTokens& tokens) {
    p.actual   = tokens.cabeza;
    p.raiz     = nullptr;
    p.hayError = false;
    listaErrores_init(p.errores);
}

// -----------------------------------------------------------------------------
//  parser_analizar -- punto de entrada del analisis
// -----------------------------------------------------------------------------
void parser_analizar(EstadoParser& p) {
    gP       = &p;
    p.raiz   = parser_programa();
    gP       = nullptr;
}

// -----------------------------------------------------------------------------
//  parser_liberar -- libera el AST y la lista de errores
// -----------------------------------------------------------------------------
void parser_liberar(EstadoParser& p) {
    nodo_liberar(p.raiz);
    listaErrores_liberar(p.errores);
    p.raiz     = nullptr;
    p.actual   = nullptr;
    p.hayError = false;
}

// -----------------------------------------------------------------------------
//  parser_imprimirResultado -- resumen del analisis sintactico
// -----------------------------------------------------------------------------
void parser_imprimirResultado(const EstadoParser& p) {
    cout << "\n";
    cout << "+-----------------------------------------------------+\n";
    cout << "|     Resultado del analisis sintactico               |\n";
    cout << "+-----------------------------------------------------+\n";

    if (!listaErrores_hayErrores(p.errores)) {
        cout << "  [OK] Analisis sintactico completado sin errores.\n";
        cout << "  El programa es sintacticamente correcto.\n";
    } else {
        listaErrores_imprimir(p.errores);
        cout << "  Total de errores sintacticos: " << p.errores.tamanio << "\n";
        cout << "  [ERROR] El programa contiene errores sintacticos.\n";
    }
    cout << "\n";
}