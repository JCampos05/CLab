# idealC — Especificación del lenguaje y compilador

**idealC** es un lenguaje de programación imperativo educativo con sintaxis inspirada en C y palabras clave en español. Este documento sirve como referencia formal de sus reglas gramaticales, sistema de tipos, reglas semánticas y catálogo de errores.

---

## Tabla de contenidos

- [Descripción del lenguaje](#descripción-del-lenguaje)
- [Tipos de dato](#tipos-de-dato)
- [Tabla de operaciones por tipo](#tabla-de-operaciones-por-tipo)
- [Reglas gramaticales (BNF)](#reglas-gramaticales-bnf)
- [Sentencias](#sentencias)
- [Operadores y precedencia](#operadores-y-precedencia)
- [Reglas semánticas](#reglas-semánticas)
- [Tabla de símbolos](#tabla-de-símbolos)
- [Errores semánticos](#errores-semánticos)
- [Arquitectura del compilador](#arquitectura-del-compilador)
- [Archivos del proyecto](#archivos-del-proyecto)
- [Compilación y uso](#compilación-y-uso)
- [Estado del proyecto](#estado-del-proyecto)

---

## Descripción del lenguaje

idealC es un lenguaje:

- **Imperativo y estructurado** — organizado en funciones con bloques delimitados por llaves.
- **Fuertemente tipado** — no hay conversiones implícitas arbitrarias; solo se permite la promoción `entero → deci`.
- **Tipado estáticamente** — los tipos se declaran y verifican en tiempo de compilación.
- **LL(1)** — gramática analizable con un token de anticipación mediante Parseo Descendente Recursivo (PDR).
- **Palabras clave en español** — `func`, `si`, `sino`, `para`, `mientras`, `retornar`, `escribir`, `leer`, `limpPantalla`, `verdadero`, `falso`, `y`, `o`, `no`.

---

## Tipos de dato

| Tipo | Descripción | Literales válidos |
|---|---|---|
| `entero` | Número entero con signo | `42`, `0`, `-7` |
| `deci` | Número de punto flotante | `3.14`, `0.5`, `2.0` |
| `cad` | Cadena de texto | `"Hola mundo"` |
| `carac` | Carácter individual | — |
| `logico` | Valor booleano | `verdadero`, `falso` |
| `vacio` | Sin valor de retorno | solo en funciones |

### Regla de promoción implícita

| Asignación | ¿Válida? | Razón |
|---|---|---|
| `entero ← entero` | Sí | Tipos idénticos |
| `deci ← deci` | Sí | Tipos idénticos |
| `deci ← entero` | Sí | Promoción implícita (no hay pérdida) |
| `entero ← deci` | **No** | Pérdida de precisión |
| `logico ← logico` | Sí | Tipos idénticos |
| `cad ← cad` | Sí | Tipos idénticos |
| `carac ← carac` | Sí | Tipos idénticos |
| cualquier ← distinto | **No** | Error semántico de tipo |

---

## Tabla de operaciones por tipo

Las siguientes tablas definen el tipo resultante de aplicar un operador a dos operandos. **E** indica error semántico.

### Operadores aritméticos: `+` `-` `*` `/` `%`

| Izq \ Der | `cad` | `entero` | `deci` | `logico` |
|---|---|---|---|---|
| `cad` | `cad` (solo `+`) | **E** | **E** | **E** |
| `entero` | **E** | `entero` | `deci` | **E** |
| `deci` | **E** | `deci` | `deci` | **E** |
| `logico` | **E** | **E** | **E** | **E** |

> La concatenación `cad + cad` solo es válida con el operador `+`. Los operadores `-`, `*`, `/`, `%` no aplican a cadenas.

### Operadores relacionales: `<` `>` `<=` `>=` `==` `!=`

| Izq \ Der | `cad` | `entero` | `deci` | `logico` |
|---|---|---|---|---|
| `cad` | `logico` | **E** | **E** | **E** |
| `entero` | **E** | `logico` | `logico` | **E** |
| `deci` | **E** | `logico` | `logico` | **E** |
| `logico` | **E** | **E** | **E** | `logico` (solo `==` `!=`) |

> El resultado siempre es `logico`. Los operandos deben ser del mismo tipo o compatibles (promoción `entero → deci`).

### Operadores lógicos: `y` `o`

| Izq \ Der | `logico` | otros |
|---|---|---|
| `logico` | `logico` | **E** |
| otros | **E** | **E** |

> Ambos operandos **deben** ser `logico`. No hay conversión implícita.

### Operador unario `no`

| Operando | Resultado |
|---|---|
| `logico` | `logico` |
| otros | **E** |

### Operador unario `-` (negación)

| Operando | Resultado |
|---|---|
| `entero` | `entero` |
| `deci` | `deci` |
| otros | **E** |

### Operadores postfijos `++` `--`

| Operando | Resultado |
|---|---|
| `entero` | `entero` |
| `deci` | `deci` |
| otros | **E** |

---

## Reglas gramaticales (BNF)

La gramática completa de idealC en notación BNF extendida:

```
<programa>       ::= { <declaracion> }

<declaracion>    ::= <declFunc> | <declVariable>

<declFunc>       ::= 'func' <tipo> IDENT '(' <params> ')' <bloque>

<params>         ::= ε
                   | <tipo> IDENT { ',' <tipo> IDENT }

<tipo>           ::= 'entero' | 'deci' | 'cad' | 'carac' | 'logico' | 'vacio'

<bloque>         ::= '{' { <sentencia> } '}'

<sentencia>      ::= <declVariable>
                   | <asignacion>
                   | <llamadaStmt>
                   | <sentSi>
                   | <sentPara>
                   | <sentMientras>
                   | <sentRetornar>
                   | <sentEscribir>
                   | <sentLeer>
                   | <sentLimpPantalla>
                   | <incDecStmt>

<declVariable>   ::= <tipo> IDENT [ '=' <expresion> ]
                     { ',' IDENT [ '=' <expresion> ] } ';'

<asignacion>     ::= IDENT '=' <expresion> ';'

<llamadaStmt>    ::= IDENT '(' <args> ')' ';'

<sentSi>         ::= 'si' '(' <expresion> ')' <bloque>
                     [ 'sino' <bloque> ]

<sentPara>       ::= 'para' '(' <initPara> ';' <expresion> ';' <incPara> ')' <bloque>

<initPara>       ::= <tipo> IDENT '=' <expresion>    (Forma A — declara variable)
                   | IDENT '=' <expresion>            (Forma B — variable externa)

<incPara>        ::= IDENT '++' | IDENT '--'

<sentMientras>   ::= 'mientras' '(' <expresion> ')' <bloque>

<sentRetornar>   ::= 'retornar' [ <expresion> ] ';'

<sentEscribir>   ::= 'escribir' '(' <args> ')' ';'

<sentLeer>       ::= 'leer' '(' IDENT ')' ';'

<sentLimpPantalla> ::= 'limpPantalla' '(' ')' ';'

<incDecStmt>     ::= IDENT '++' ';' | IDENT '--' ';'

<args>           ::= ε | <expresion> { ',' <expresion> }

<expresion>      ::= <expLogica>

<expLogica>      ::= <expRelacional> { ( 'y' | 'o' ) <expRelacional> }

<expRelacional>  ::= <expAritm> [ opRel <expAritm> ]

opRel            ::= '<' | '>' | '<=' | '>=' | '==' | '!='

<expAritm>       ::= <term> { ( '+' | '-' ) <term> }

<term>           ::= <unario> { ( '*' | '/' | '%' ) <unario> }

<unario>         ::= 'no' <unario>
                   | '-' <unario>
                   | <primario> [ '++' | '--' ]

<primario>       ::= LIT_ENTERO
                   | LIT_DECIMAL
                   | LIT_CADENA
                   | 'verdadero'
                   | 'falso'
                   | IDENT [ '(' <args> ')' ]
                   | '(' <expresion> ')'
```

> La gramática es **LL(1)**: cada alternativa se elige con un solo token de anticipación. El único caso de ambigüedad (sino colgante) se resuelve asociando el `sino` al `si` más cercano.

---

## Sentencias

### Declaración de función

```
func entero suma(entero a, entero b) { ... }
func vacio imprimir(cad mensaje) { ... }
func logico esPar(entero n) { ... }
```

### Declaración de variable

```
entero x;
entero a = 10, b = 20, c;
deci pi = 3.14;
cad nombre = "idealC";
logico activo = verdadero;
```

### Asignación

```
x = 42;
nombre = "nuevo valor";
```

### Sentencia Si

```
si (x > 0) {
    escribir("positivo");
} sino {
    escribir("no positivo");
}
```

La rama `sino` es opcional. El `sino` colgante se asocia al `si` más cercano.

### Sentencia Para

```
para (entero i = 0; i < 10; i++) {   // Forma A: declara i
    escribir(i);
}

para (i = 0; i < 10; i++) {           // Forma B: i ya declarada
    escribir(i);
}
```

### Sentencia Mientras

```
mientras (x > 0) {
    x--;
}
```

### Retornar

```
retornar a + b;    // función no-vacio — expresión obligatoria
retornar;          // función vacio    — sin expresión
```

### Escribir / Leer / LimpPantalla

```
escribir("Resultado: ", x, x * 2);
leer(nombre);
limpPantalla();
```

### Incremento / Decremento postfijo

```
i++;
contador--;
```

---

## Operadores y precedencia

| Nivel | Categoría | Operadores | Asociatividad |
|---|---|---|---|
| 1 (menor) | Lógicos binarios | `y` `o` | Izquierda |
| 2 | Relacionales | `<` `>` `<=` `>=` `==` `!=` | No asociativo |
| 3 | Aditivos | `+` `-` | Izquierda |
| 4 | Multiplicativos | `*` `/` `%` | Izquierda |
| 5 (mayor) | Unarios / postfijo | `no` `-` `++` `--` | Derecha / Postfijo |

---

## Reglas semánticas

El análisis semántico verifica que el programa sea correcto en significado, no solo en forma. Las reglas son:

### 1. Variables no declaradas

Toda variable debe ser declarada antes de usarse. La búsqueda recorre el scope actual y todos los scopes ancestros.

```
entero x;
x = 10;      // OK — x declarada
y = 5;       // ERROR — y no declarada
```

### 2. Redeclaración en el mismo scope

No se puede declarar la misma variable dos veces en el mismo nivel de scope.

```
entero x;
entero x;    // ERROR — x ya declarada en este scope
```

El **shadowing** (ocultar una variable del scope padre) sí está permitido:

```
entero x;    // scope externo
si (...) {
    entero x;  // OK — nuevo scope, oculta el externo
}
```

### 3. Redefinición de función

No se puede declarar la misma función dos veces.

```
func entero f(entero n) { ... }
func entero f(entero n) { ... }   // ERROR
```

### 4. Operaciones entre tipos incompatibles

Los operadores solo aceptan los tipos definidos en la tabla de operaciones. Combinaciones no listadas como válidas generan error.

```
entero x = 5;
logico b = verdadero;
entero r = x + b;    // ERROR — '+' no aplica a 'logico'
```

### 5. Tipos incompatibles en asignación / inicialización

El tipo de la expresión debe ser compatible con el tipo de la variable.

```
entero x;
deci d = 3.14;
x = d;       // ERROR — no se puede asignar 'deci' a 'entero'
d = x;       // OK    — promoción implícita entero → deci
```

### 6. Identificadores fuera de ámbito

Una variable declarada dentro de un bloque no es visible fuera de él.

```
si (verdadero) {
    entero local = 1;
}
local = 2;   // ERROR — 'local' no declarada en este scope
```

### 7. Parámetros inválidos en llamadas a función

La llamada debe coincidir en número de argumentos y tipos con la declaración.

```
func entero suma(entero a, entero b) { ... }

suma(1);           // ERROR — espera 2 argumento(s), se dieron 1
suma(1, 2, 3);     // ERROR — espera 2 argumento(s), se dieron 3
suma(1, verdadero);// ERROR — tipo 'logico' no compatible con 'entero'
```

### 8. Función no declarada

Toda función llamada debe haber sido declarada o ser un builtin.

```
resultado = miFuncion(x);   // ERROR si miFuncion no fue declarada
```

### 9. Coherencia de retorno

```
func entero f() {
    retornar;           // ERROR — función no-vacio debe retornar un valor
}

func vacio g() {
    retornar 42;        // ERROR — función 'vacio' no debe retornar un valor
}

func logico h() {
    retornar 1;         // ERROR — función retorna 'logico' pero se devuelve 'entero'
}
```

### 10. Condiciones deben ser lógicas

Las condiciones de `si`, `mientras` y `para` deben ser de tipo `logico`.

```
entero x = 5;
si (x) { ... }          // ERROR — condición debe ser 'logico', se encontró 'entero'
mientras (x) { ... }    // ERROR
```

---

## Tabla de símbolos

El analizador semántico construye una tabla de símbolos unificada que registra, en orden de aparición, todos los identificadores del programa.

### Estructura de la tabla

| Campo | Descripción |
|---|---|
| **ID** | Número secuencial de entrada (1, 2, 3…) |
| **Nombre** | Lexema del identificador |
| **Tipo** | Tipo de dato declarado (`entero`, `deci`, `logico`, etc.) |
| **Valor** | Valor literal asignado, firma de parámetros (funciones), o `-` si no hay |
| **Ámbito** | `global` (funciones y builtins) o `local` (parámetros y variables) |
| **Ubicación** | Línea y columna de la declaración (`L5,C12`) |
| **Categoría** | `funcion`, `parametro` o `variable` |

### Campo Valor según categoría

| Categoría | Contenido del campo Valor |
|---|---|
| `funcion` | Firma de parámetros: `(entero a, deci b)` o `()` |
| `parametro` | `-` (no tiene valor de inicialización) |
| `variable` | Literal del inicializador (`42`, `"hola"`, `verdadero`), `<expr>` si es una expresión compleja, o `-` si no tiene inicializador. Si se asigna un literal posteriormente (`x = 10`), se actualiza al valor del literal. |

### Funciones nativas pre-registradas (builtins)

Antes de analizar el programa, el compilador registra automáticamente estas funciones:

| Función | Firma | Tipo retorno |
|---|---|---|
| `mul(a, b)` | `(entero a, entero b)` | `entero` |
| `suma(a, b)` | `(entero a, entero b)` | `entero` |
| `resta(a, b)` | `(entero a, entero b)` | `entero` |
| `div(a, b)` | `(entero a, entero b)` | `entero` |

### Scopes y visibilidad

- Cada bloque `{ }` abre un scope nuevo (stack de scopes).
- La búsqueda de una variable recorre el scope actual y todos sus ancestros.
- El **shadowing** (variable local con el mismo nombre que una del scope padre) está permitido; la interna oculta a la externa.
- La redeclaración en el **mismo** scope es un error.

---

## Errores semánticos

Catálogo completo de errores detectados por el analizador semántico:

| # | Categoría (CLAUDE.md) | Mensaje de error |
|---|---|---|
| 1 | Variables no declaradas | `Variable '<nombre>' no declarada en este scope` |
| 2 | Redeclaración en mismo ámbito | `Variable '<nombre>' ya declarada en este scope` |
| 3 | Redefinición de función | `Funcion '<nombre>' ya fue declarada (linea <N>)` |
| 4 | Función no declarada | `Funcion '<nombre>' no declarada` |
| 5 | Parámetros inválidos — aridad | `Funcion '<nombre>' espera <N> argumento(s), se dieron <M>` |
| 6 | Parámetros inválidos — tipo | `Argumento de tipo '<T>' no compatible con parametro de tipo '<P>' en llamada a '<f>'` |
| 7 | Tipos incompatibles en asignación | `No se puede asignar tipo '<T2>' a variable de tipo '<T1>'` |
| 8 | Tipos incompatibles en retorno | `Funcion retorna '<T1>' pero se devuelve '<T2>'` |
| 9 | Condición no lógica | `La condicion debe ser de tipo 'logico', se encontro '<T>'` |
| 10 | Operandos/operador incompatibles | `Operador '<op>' no aplica a tipo '<T>'` |
| 11 | Retorno con valor en función vacio | `Funcion 'vacio' no debe retornar un valor` |
| 12 | Retorno sin valor en función no-vacio | `Funcion no-vacio debe retornar un valor` |

### Supresión de errores en cascada

Cuando una subexpresión ya generó un error, retorna el centinela `"?"`. Los nodos padre que reciben `"?"` omiten su propia validación de tipos, evitando errores duplicados para un mismo problema origen.

---

## Árbol de Sintaxis Abstracta (AST) con atributos

El AST se construye durante el análisis sintáctico y se **anota** durante el análisis semántico. Cada nodo de expresión recibe un **atributo semántico** (`tipoAtrib`) que indica el tipo calculado.

```cpp
struct NodoAST {
    TipoNodo tipo;
    string   valor;      // nombre, operador o lexema
    string   valorTipo;  // tipo declarado (en DECL_VAR, PARAM, FUNC)
    string   tipoAtrib;  // tipo calculado por el analizador semantico
    int      linea, columna;
    NodoAST* izq;
    NodoAST* der;
    NodoAST* extra;
    NodoAST* siguiente;
};
```

### Atributos sintetizados (bottom-up)

El tipo de cada expresión se calcula en **postorden**: primero los hijos, luego el padre. Ejemplos:

| Expresión | Cálculo |
|---|---|
| `42` | `NODO_LIT_ENTERO → tipoAtrib = "entero"` |
| `3.14` | `NODO_LIT_DECIMAL → tipoAtrib = "deci"` |
| `x + y` donde `x,y:entero` | `NODO_OP_BINARIO "+" → tipoAtrib = "entero"` |
| `x + y` donde `x:entero, y:deci` | `NODO_OP_BINARIO "+" → tipoAtrib = "deci"` (promoción) |
| `x > 0` | `NODO_OP_BINARIO ">" → tipoAtrib = "logico"` |
| `esPar(n)` | `NODO_LLAMADA_EXPR → tipoAtrib = tipo de retorno de esPar` |

### Atributos heredados (top-down)

Algunos valores se pasan hacia abajo durante el recorrido:

| Atributo heredado | Dirección | Uso |
|---|---|---|
| `tipoRetFunActual` | función → sentencias `retornar` | Verifica que el valor retornado sea compatible |
| `ambitoActual` | función → declaraciones internas | Registra a qué función pertenece cada símbolo |
| `scopeActual` | bloque → subexpresiones | Contexto de resolución de nombres |

---

## Arquitectura del compilador

```
Program.txt
    │
    ▼
┌─────────────────────┐
│   Analizador Léxico  │  lexer.h / lexer.cpp / token.h / toke.cpp
│   (lexer_analizar)   │  → Produce: ListaTokens
└─────────────────────┘
    │
    ▼
┌──────────────────────────┐
│  Analizador Sintáctico    │  parser.h / parser.cpp / nodo.h / nodo.cpp
│  PDR LL(1)               │  → Produce: NodoAST* (raíz del AST)
└──────────────────────────┘
    │
    ▼
┌──────────────────────────┐
│  Analizador Semántico     │  semantico.h / semantico.cpp
│  (semantico_analizar)     │  → Produce: TablaSimbolos + AST anotado
└──────────────────────────┘
    │
    ▼
┌──────────────────────────┐
│  Generación de código     │  pendiente
└──────────────────────────┘
```

Cada fase sigue el mismo patrón: struct de estado plano (`EstadoX`), funciones `_init` / `_analizar` / `_liberar` / `_imprimirResultado`, y colecciones como listas enlazadas manuales (sin STL de contenedores).

---

## Archivos del proyecto

```
token.h         — TokenType, Token, ListaTokens, ListaErrores
toke.cpp        — Operaciones sobre tokens y listas
lexer.h         — Interfaz del analizador léxico
lexer.cpp       — Implementación del analizador léxico
nodo.h          — TipoNodo, NodoAST (con tipoAtrib), convención de hijos
nodo.cpp        — nodo_crear, nodo_imprimir, nodo_liberar
parser.h        — Interfaz del analizador sintáctico
parser.cpp      — Parser PDR LL(1) completo
semantico.h     — EstadoSemantico, TablaSimbolos, interfaz pública
semantico.cpp   — Analizador semántico completo con tablas y atributos
main.cpp        — Menú interactivo (11 opciones)
Program.txt     — Archivo fuente de entrada
tests/          — 20 casos de prueba semánticos
```

---

## Compilación y uso

### Requisitos

- Compilador C++ con soporte C++11 o superior (g++ recomendado).
- Windows (el menú usa `system("cls")` y `system("pause")`).

### Compilar

```bash
g++ main.cpp lexer.cpp toke.cpp nodo.cpp parser.cpp semantico.cpp -o main
```

### Ejecutar

```bash
./main
```

El programa busca automáticamente **`Program.txt`** en el mismo directorio.

### Menú de opciones

```
 1) Cargar y analizar Program.txt      — análisis léxico
 2) Ver lista de tokens reconocidos
 3) Ver errores léxicos
 4) Ver resumen del análisis léxico
 5) Ejecutar análisis sintáctico       — construye el AST
 6) Ver errores sintácticos
 7) Ver Árbol de Sintaxis Abstracta    — con atributos semánticos
 8) Ejecutar análisis semántico        — verifica tipos, scopes y aridades
 9) Ver errores semánticos
10) Ver Tabla de símbolos              — ID/Nombre/Tipo/Valor/Ámbito/Ubic/Categ.
11) Salir
```

> Orden requerido: **1 → 5 → 8**. Cada fase depende de la anterior.

---

## Estado del proyecto

| Fase | Estado |
|---|---|
| Analizador léxico | Completo |
| Analizador sintáctico PDR LL(1) | Completo |
| Construcción del AST | Completo |
| Analizador semántico | Completo |
| Tabla de símbolos unificada | Completo |
| AST con atributos semánticos | Completo |
| Generación de código intermedio | Pendiente |
