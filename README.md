# idealC — Compilador

**idealC** es un lenguaje de programación imperativo de propósito educativo, con sintaxis inspirada en C pero con palabras clave en español. Este repositorio contiene la implementación de su compilador en C++, actualmente con el analizador léxico y el analizador sintáctico completos.

---

## Tabla de contenidos

- [Descripción del lenguaje](#descripción-del-lenguaje)
- [Archivos del proyecto](#archivos-del-proyecto)
- [Compilación y uso](#compilación-y-uso)
- [Tipos de dato](#tipos-de-dato)
- [Estructura de un programa](#estructura-de-un-programa)
- [Sentencias](#sentencias)
- [Operadores](#operadores)
- [Ejemplos de código](#ejemplos-de-código)
- [Arquitectura del compilador](#arquitectura-del-compilador)
- [Analizador léxico](#analizador-léxico)
- [Analizador sintáctico](#analizador-sintáctico)
- [Árbol de Sintaxis Abstracta (AST)](#árbol-de-sintaxis-abstracta-ast)
- [Manejo de errores](#manejo-de-errores)
- [Estado del proyecto](#estado-del-proyecto)

---

## Descripción del lenguaje

idealC es un lenguaje:

- **Imperativo y estructurado** — organizado en funciones con bloques delimitados por llaves.
- **Tipado estáticamente** — cada variable y función tiene un tipo declarado explícitamente.
- **LL(1)** — su gramática fue diseñada para ser analizable con un solo token de anticipación, lo que permite el uso de Parseo Descendente Recursivo (PDR).
- **Con palabras clave en español** — `func`, `si`, `sino`, `para`, `mientras`, `retornar`, `escribir`, `leer`, `limpPantalla`, `verdadero`, `falso`, `y`, `o`, `no`.

---

## Archivos del proyecto

```
token.h        — Definición de TokenType, struct Token, ListaTokens, ListaErrores
toke.cpp       — Implementación de operaciones sobre tokens y listas
lexer.h        — Interfaz pública del analizador léxico (EstadoLexer)
lexer.cpp      — Implementación del analizador léxico
nodo.h         — Definición de TipoNodo, struct NodoAST, convención de hijos
nodo.cpp       — Implementación de nodo_crear, nodo_imprimir, nodo_liberar
parser.h       — Interfaz pública del analizador sintáctico (EstadoParser)
parser.cpp     — Implementación del parser PDR completo
main.cpp       — Menú interactivo con 8 opciones
Program.txt    — Archivo fuente de entrada (nombre fijo)
```

### Documentación generada

```
Gramatica_idealC.txt                  — Gramática BNF extendida completa
AnalizadorSintactico_idealC_v2.docx   — Documento Word con especificación,
                                        gramática y diagramas sintácticos
```

---

## Compilación y uso

### Requisitos

- Compilador C++ con soporte C++11 o superior (g++ recomendado).
- Windows (el menú usa `system("cls")` y `system("pause")`).

### Compilar

```bash
g++ main.cpp lexer.cpp toke.cpp nodo.cpp parser.cpp -o main
```

### Ejecutar

```bash
./main
```

El programa busca automáticamente el archivo **`Program.txt`** en el mismo directorio del ejecutable. No acepta argumentos por línea de comandos.

### Menú de opciones

```
1) Cargar y analizar Program.txt      — ejecuta el análisis léxico
2) Ver lista de tokens reconocidos    — imprime todos los tokens
3) Ver errores léxicos                — imprime errores con línea y columna
4) Ver resumen del análisis léxico    — resumen estadístico
5) Ejecutar análisis sintáctico       — construye el AST
6) Ver errores sintácticos            — lista de errores del parser
7) Ver Árbol de Sintaxis Abstracta    — imprime el AST completo
8) Salir
```

> El análisis léxico (opción 1) debe ejecutarse antes del sintáctico (opción 5).

---

## Tipos de dato

| Tipo | Descripción | Ejemplo literal |
|---|---|---|
| `entero` | Número entero | `42`, `0`, `100` |
| `deci` | Número decimal | `3.14`, `0.5` |
| `carac` | Carácter individual | — |
| `cad` | Cadena de texto | `"Hola mundo"` |
| `logico` | Valor booleano | `verdadero`, `falso` |
| `vacio` | Sin valor de retorno | solo en funciones |

---

## Estructura de un programa

Un programa idealC es una secuencia de declaraciones globales. Cada declaración es una función o una variable global.

```
func <tipo_retorno> <nombre>(<params>) {
    <sentencias>
}
```

### Ejemplo

```
func entero factorial(entero n) {
    si (n <= 1) {
        retornar 1;
    } sino {
        retornar mul(n, factorial(n - 1));
    }
}

func vacio main() {
    entero resultado = factorial(5);
    escribir("Factorial de 5: ", resultado);
}
```

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

La rama `sino` es opcional. El `sino` colgante se asocia siempre al `si` más cercano.

### Sentencia Para

```
para (entero i = 0; i < 10; i++) {
    escribir(i);
}
```

La variable de control puede declararse en el encabezado (`entero i`) o ser una ya existente.

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

> El parser verifica que la forma usada coincida con el tipo de retorno declarado de la función. La compatibilidad de tipos entre la expresión y el tipo de retorno es responsabilidad del analizador semántico (pendiente).

### Escribir

```
escribir("Resultado: ", x, x * 2);
```

### Leer

```
leer(nombre);
leer(edad);
```

Solo acepta un identificador por sentencia. Para leer múltiples valores se usan múltiples sentencias `leer`.

### LimpPantalla

```
limpPantalla();
```

### Incremento / Decremento

```
i++;
contador--;
```

También aparecen como parte del encabezado del `para` y como operadores postfijos dentro de expresiones.

---

## Operadores

### Tabla de precedencia (de menor a mayor)

| Nivel | Categoría | Operadores | Asociatividad |
|---|---|---|---|
| 1 (menor) | Lógicos binarios | `y` `o` | Izquierda |
| 2 | Relacionales | `<` `>` `<=` `>=` `==` `!=` | No asociativo |
| 3 | Aritméticos aditivos | `+` `-` | Izquierda |
| 4 | Aritméticos multiplicativos | `*` `/` `%` | Izquierda |
| 5 (mayor) | Unarios | `no` `-` `++` `--` | Derecha / Postfijo |

### Aritméticos

`+` `-` `*` `/` `%` — operan sobre `entero` o `deci`.

### Relacionales

`<` `>` `<=` `>=` `==` `!=` — producen un valor `logico`. No son asociativos.

### Lógicos

`y` (AND), `o` (OR) — binarios sobre `logico`.  
`no` (NOT) — unario, niega un valor lógico.

### Unarios

| Operador | Tipo | Ejemplo |
|---|---|---|
| `no` | Prefijo lógico | `no activo` |
| `-` | Prefijo aritmético | `-x`, `-(a+b)` |
| `++` | Postfijo | `i++` |
| `--` | Postfijo | `i--` |

---

## Ejemplos de código

### Fibonacci

```
func entero fibonacci(entero n) {
    si (n <= 1) {
        retornar n;
    } sino {
        retornar suma(fibonacci(n - 1), fibonacci(n - 2));
    }
}
```

### Factorial iterativo

```
func entero factorial(entero n) {
    entero resultado = 1;
    para (entero i = 2; i <= n; i++) {
        resultado = mul(resultado, i);
    }
    retornar resultado;
}
```

### Verificar par o impar

```
func logico esPar(entero n) {
    si (n % 2 == 0) {
        retornar verdadero;
    } sino {
        retornar falso;
    }
}
```

---

## Arquitectura del compilador

```
Program.txt
    │
    ▼
┌─────────────────────┐
│   Analizador Léxico  │  lexer.h / lexer.cpp
│   (lexer_analizar)   │  toke.cpp / token.h
└─────────────────────┘
    │  ListaTokens
    ▼
┌──────────────────────────┐
│  Analizador Sintáctico    │  parser.h / parser.cpp
│  PDR LL(1)               │  nodo.h / nodo.cpp
│  (parser_analizar)        │
└──────────────────────────┘
    │  NodoAST* (raíz del AST)
    ▼
┌──────────────────────────┐
│  Analizador Semántico     │   pendiente
└──────────────────────────┘
    │
    ▼
┌──────────────────────────┐
│  Generación de código     │   pendiente
└──────────────────────────┘
```

---

## Analizador léxico

Implementado en `lexer.cpp`. Lee el archivo fuente carácter a carácter y produce una `ListaTokens` (lista enlazada de `NodoToken`).

### Tokens reconocidos

| Categoría | Tokens |
|---|---|
| Tipos de dato | `entero` `deci` `carac` `cad` `logico` `vacio` |
| Booleanos | `verdadero` `falso` |
| Control de flujo | `si` `sino` `para` `mientras` |
| Función | `func` `retornar` |
| E/S | `leer` `escribir` `limpPantalla` |
| Lógicos | `y` `o` `no` |
| Aritméticos | `+` `-` `*` `/` `%` `++` `--` |
| Relacionales | `<` `>` `<=` `>=` `==` `!=` |
| Asignación | `=` |
| Delimitadores | `;` `,` `(` `)` `{` `}` |
| Literales | `LIT_ENTERO` `LIT_DECIMAL` `LIT_CADENA` |
| Identificador | `IDENTIFICADOR` |

Las palabras reservadas se reconocen con prioridad sobre los identificadores.

---

## Analizador sintáctico

Implementado en `parser.cpp` mediante **Parseo Descendente Recursivo (PDR) LL(1)**. Recibe la `ListaTokens` del léxico y construye un **AST** (`NodoAST*`).

### Gramática resumida

```
<programa>      ::= { <declaracion> }
<declaracion>   ::= <declFunc> | <declVariable>
<declFunc>      ::= 'func' <tipo> IDENT '(' <params> ')' <bloque>
<bloque>        ::= '{' { <sentencia> } '}'
<sentencia>     ::= <declVariable> | <asignacion> | <llamadaStmt>
                  | <sentSi> | <sentPara> | <sentMientras>
                  | <sentRetornar> | <sentEscribir> | <sentLeer>
                  | <sentLimpPantalla> | <incDecStmt>
<expresion>     ::= <expLogica>
<expLogica>     ::= <expRelacional> { ('y'|'o') <expRelacional> }
<expRelacional> ::= <expAritm> [ opRel <expAritm> ]
<expAritm>      ::= <term> { ('+'|'-') <term> }
<term>          ::= <unario> { ('*'|'/'|'%') <unario> }
<unario>        ::= 'no' <unario> | '-' <unario> | <primario> ['++' | '--']
<primario>      ::= LIT_ENTERO | LIT_DECIMAL | LIT_CADENA
                  | 'verdadero' | 'falso'
                  | IDENT ['(' <args> ')']
                  | '(' <expresion> ')'
```

La gramática completa se encuentra en `Gramatica_idealC.txt`.

---

## Árbol de Sintaxis Abstracta (AST)

Cada nodo del AST es un `struct NodoAST` con los siguientes campos:

```cpp
struct NodoAST {
    TipoNodo  tipo;
    string    valor;       // nombre, operador, lexema según el tipo
    string    valorTipo;   // tipo de dato (en declaraciones y funciones)
    int       linea, columna;
    NodoAST*  izq;         // hijo izquierdo / primer hijo
    NodoAST*  der;         // hijo derecho / segundo hijo
    NodoAST*  extra;       // hijo adicional (bloque sino, cuerpo para, etc.)
    NodoAST*  siguiente;   // siguiente nodo en una lista (params, args, stmts)
};
```

### Tipos de nodo

| Nodo | Uso |
|---|---|
| `NODO_PROGRAMA` | Raíz — lista de declaraciones globales |
| `NODO_FUNC` | Declaración de función |
| `NODO_PARAM` | Parámetro formal |
| `NODO_BLOQUE` | Bloque `{ ... }` |
| `NODO_DECL_VAR` | Declaración de variable |
| `NODO_ASIGNACION` | Asignación `x = expr` |
| `NODO_LLAMADA` | Llamada a función como sentencia |
| `NODO_LLAMADA_EXPR` | Llamada a función como expresión |
| `NODO_SI` | Sentencia `si / sino` |
| `NODO_PARA` | Sentencia `para` |
| `NODO_MIENTRAS` | Sentencia `mientras` |
| `NODO_RETORNAR` | Sentencia `retornar` |
| `NODO_ESCRIBIR` | Sentencia `escribir` |
| `NODO_LEER` | Sentencia `leer` |
| `NODO_LIMPPANTALLA` | Sentencia `limpPantalla` |
| `NODO_OP_BINARIO` | Operador binario (`+`, `-`, `==`, `y`, etc.) |
| `NODO_OP_UNARIO` | Operador unario prefijo (`no`, `-`) |
| `NODO_INC_POST` | Incremento postfijo `++` |
| `NODO_DEC_POST` | Decremento postfijo `--` |
| `NODO_LIT_ENTERO` | Literal entero |
| `NODO_LIT_DECIMAL` | Literal decimal |
| `NODO_LIT_CADENA` | Literal cadena |
| `NODO_LIT_VERDADERO` | Literal `verdadero` |
| `NODO_LIT_FALSO` | Literal `falso` |
| `NODO_IDENTIFICADOR` | Variable usada como expresión |
| `NODO_ARGS` | Contenedor de argumentos reales |
| `NODO_ERROR` | Marcador de error de parseo |

---

## Manejo de errores

### Errores léxicos

Se reportan con línea, columna y lexema. No detienen el análisis — el léxico continúa reconociendo tokens válidos.

### Errores sintácticos

El parser implementa **recuperación por pánico** con dos estrategias:

- **`sincronizar()`** — avanza hasta el próximo `;` o `}`. Se usa en errores dentro de sentencias simples.
- **`sincronizarBloque()`** — busca el `{` de apertura y cuenta llaves hasta cerrar el bloque completo. Se usa cuando falla el encabezado de `func`, `si`, `para` o `mientras`.

Cuando hay un error a nivel global (token inesperado fuera de toda función), el parser registra un solo error y avanza hasta el próximo `func` o fin de archivo, intentando siempre parsear el bloque asociado para reportar también los errores internos.

### Verificación de `retornar`

El parser verifica en tiempo de análisis que:
- Funciones de tipo no-`vacio` usen `retornar <expresion>;`
- Funciones de tipo `vacio` usen `retornar;` sin expresión

---

## Estado del proyecto

| Fase | Estado |
|---|---|
| Analizador léxico |  Completo |
| Analizador sintáctico (PDR LL(1)) |  Completo |
| Construcción del AST |  Completo |
| Verificación de `retornar` por tipo |  Implementada en el parser |
| Analizador semántico |  Pendiente |
| Generación de código intermedio |  Pendiente |

### Próximos pasos sugeridos

1. **Analizador semántico** — tabla de símbolos, verificación de tipos en expresiones, verificación de que variables y funciones estén declaradas antes de usarse.
2. **Generación de código intermedio** — código de tres direcciones o bytecode.
