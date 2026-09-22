# Sistema de autómatas C++17 — versión 3, corrección de compilación en Windows

Implementación completa con clases, getters, setters, consultas booleanas y algoritmos funcionales. Sin librerías externas. El proyecto conserva compatibilidad con los nombres de clases y los archivos de la primera versión.

## Corrección de MSB4006 en Visual Studio

La biblioteca ahora se llama `AutomatasCore`, y el ejecutable sigue siendo `Automatas`. Antes los targets `automatas` y `Automatas` generaban proyectos cuyos nombres colisionaban en Windows, provocando la dependencia circular `ResolveProjectReferences`.

Reemplazá los archivos del proyecto con esta versión y ejecutá el BAT. Este usa `build_windows_v3` para generar proyectos nuevos sin reutilizar la carpeta `build` anterior. No necesitás borrar nada manualmente. El ejecutable del BAT queda en `build_windows_v3/Release/Automatas.exe` con Visual Studio.

La corrección de nombres y rutas se verificó en los archivos; no se ejecutó MSBuild en este entorno Linux.

## Ejecutar

En Windows, extraé todo y ejecutá **compilar_y_ejecutar.bat**. Requiere CMake 3.16+ y un compilador C++17, por ejemplo Visual Studio con **Desarrollo para el escritorio con C++**. El BAT compila, ejecuta las pruebas y abre la consola. Si no detecta el compilador, usá Developer Command Prompt de Visual Studio.

En cualquier plataforma con CMake:

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Con los comandos manuales anteriores, el ejecutable queda en `build/Release/Automatas.exe` con Visual Studio; con un generador de una configuración queda en `build/Automatas.exe` o `build/Automatas`.

Alternativa Linux con GCC, desde la carpeta del proyecto:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude src/*.cpp -o Automatas
./Automatas
```

Validado con GCC en Linux, incluyendo AddressSanitizer y UndefinedBehaviorSanitizer. LeakSanitizer se desactivó por la restricción de ptrace del entorno. No se realizó una compilación de Windows ni de CMake en este entorno y no se incluye un ejecutable precompilado.

## Correspondencia con el diagrama

| Elemento | Implementación |
|---|---|
| `Main` | `main(args)`, `ejecutar()`. La función global de C++ delega en esta clase. |
| `InterfazUsuario` | `iniciar()`, `mostrarMenu()`, `procesarOpcion(int)`, comandos y acceso al autómata. |
| `ConversorAFND` | `convertirA_AFD()`, `clausuraEpsilon()`, `mover()`. |
| `MinimizadorAFD` | `minimizar()`, `particionarEstados()`, `eliminarEstadosInaccesibles()`. |
| `AdministradorDeAutomatas` | `cargarAutomata()`, `guardarAutomata()`. |
| `TesterEquivalencia` | `probarEquivalencia()`, `generarCadenasDePrueba()`, comparación exacta adicional. |
| `Automata` | Estados, alfabeto, inicial, tipo, validación y operaciones de edición. |
| `Estado` | Id, inicial, final y transiciones. |
| `Transicion` | Símbolo y destinos. |
| `TipoAutomata` | `enum class TipoAutomata { AFD, AFND };`. |

También se conserva `ConstruirAutomata` para crear y configurar autómatas. Cada clase principal tiene `.h` y `.cpp`; el enum está en su `.h`.

### Diferencia intencional respecto de la imagen

El diagrama muestra un único destino por transición. Se conserva tu requisito original de **una cantidad dinámica de punteros a estados por transición**:

```cpp
// Automata posee los estados:
std::vector<std::unique_ptr<Estado>> estados_;
// Estado posee las transiciones:
std::vector<Transicion> transiciones_;
// Transicion referencia sus destinos:
std::vector<Estado*> destinos_;
```

`getDestino()` y `setDestino()` permiten el uso de un único destino, como en el diagrama. `getDestino()` devuelve `nullptr` si no hay destinos y lanza `std::logic_error` si hay varios, para evitar seleccionar uno silenciosamente. `setDestino(nullptr)` vacía los destinos. Para varios destinos se usan `getDestinos()` y `setDestinos()`.

Los contenedores crecen dinámicamente, limitados por la memoria. No se establecieron máximos fijos de estados, transiciones ni destinos. Se conservan ciclos y bucles. Los destinos duplicados dentro de una transición se unifican.

## Getters, setters y funciones simples

### Estado

| Consulta | Modificación |
|---|---|
| `getId()` | `setId(id)` |
| `esEstadoInicial()`, `isEstadoInicial()` | `setEstadoInicial(bool)`, `setEsInicial(bool)` |
| `esEstadoFinal()`, `isEstadoFinal()` | `setEstadoFinal(bool)`, `setEsFinal(bool)` |
| `getTransiciones()` | `setTransiciones(vector)` |
| `getTransicion(indice)` | Permite modificar una transición mediante sus setters. |
| `getCantidadTransiciones()` | `agregarTransicion(t)`, `eliminarTransicion(indice)`, `limpiarTransiciones()` |

Constructores: `Estado(id, final=false)` y `Estado(id, inicial, final)`. El primero conserva la interpretación de la versión anterior.

Marcar un estado perteneciente a un autómata como inicial desmarca el anterior y actualiza `Automata::getEstadoInicial()`. Desmarcar el inicial deja el autómata temporalmente sin inicial. `setId()` rechaza identificadores vacíos o duplicados, y no rompe las transiciones ni el puntero inicial.

### Transicion

| Consulta | Modificación |
|---|---|
| `getSimbolo()` | `setSimbolo(simbolo)` |
| `getDestino()` | `setDestino(Estado*)` |
| `getDestinos()` | `setDestinos(vector<Estado*>)` |
| `contieneDestino(Estado*)`, `getCantidadDestinos()` | `agregarDestino()`, `eliminarDestino()`, `limpiarDestinos()` |
| `esEpsilon()`, `isEpsilon()` | Se determina a partir del símbolo vacío `""`. |

Constructores: `Transicion(simbolo)`, `Transicion(simbolo, Estado*)`, `Transicion(simbolo, vector<Estado*>)`. `nullptr` no se acepta como elemento de una lista de destinos. Al editar una transición incorporada a un autómata se rechazan destinos de otro autómata.

### Automata

| Consulta | Modificación |
|---|---|
| `getEstados()` | `setEstados(vector<unique_ptr<Estado>>)` |
| `buscarEstado(id)`, `contieneEstado(ptr)`, `getCantidadEstados()` | `agregarEstado(id, final)`, `agregarEstado(unique_ptr<Estado>)`, `eliminarEstado(id)` |
| `getEstadoInicial()` | `setEstadoInicial(Estado*)`, `setEstadoInicial(id)` |
| `getAlfabeto()` | `setAlfabeto(set<string>)`, `agregarSimbolo()`, `eliminarSimbolo()` |
| `getTipo()` | `setTipo(TipoAutomata)` con validación de coherencia. |
| `esDeterminista()` | Se determina a partir de las transiciones. |
| `validarCadena(string)`, `validarCadena(vector<string>)` | Simulación del autómata. |
| `validar()`, `clonar()` | `limpiar()` |

`getEstados()` devuelve un conjunto de punteros observadores; el autómata conserva la propiedad. La versión const devuelve punteros const. Modificar el conjunto retornado no agrega ni elimina estados del autómata.

`setEstados()` transfiere la propiedad del conjunto completo, valida referencias internas, identificadores únicos y como máximo un inicial. Si falla, el autómata receptor conserva su contenido anterior, pero los objetos transferidos al argumento se destruyen. Se pueden construir primero estados sueltos con referencias entre ellos y luego transferir todo el conjunto.

Eliminar un estado limpia todos los destinos que lo referenciaban dentro del autómata. Eliminar el inicial deja el autómata sin inicial hasta elegir otro. Las operaciones de simulación, transformación y guardado requieren un inicial válido.

El alfabeto combina símbolos declarados explícitamente con símbolos de las transiciones. Nunca contiene epsilon. `setAlfabeto()` y `eliminarSimbolo()` rechazan quitar símbolos usados por transiciones. Cambiar un símbolo de transición agrega automáticamente el nuevo al alfabeto inferido; un símbolo declarado explícitamente se mantiene hasta quitarlo.

**El tipo no es una etiqueta arbitraria:** `getTipo()` calcula AFD/AFND según las transiciones. `setTipo()` comprueba que el valor solicitado coincida y lanza una excepción si no coincide; no transforma el autómata ni permite mentir sobre su estructura. Para convertir usá `ConversorAFND::convertirA_AFD()`. Un AFD puede ser parcial. Consultar el tipo no sustituye `validar()`.

### InterfazUsuario y Main

`InterfazUsuario` ofrece `getAutomata()` mutable/const y `setAutomata(Automata)`. Para pasar una copia usá `ui.setAutomata(a.clonar())`; para transferirlo, `ui.setAutomata(std::move(a))`. También existen sobrecargas con `istream/ostream` para pruebas automatizadas.

`Main::main(vector<string>)` conserva la forma del diagrama; actualmente los argumentos no se interpretan. `src/PuntoEntrada.cpp` contiene la función global `main()` requerida por C++.

## Ejemplo usando los nuevos métodos

```cpp
#include "Automata.h"
#include "ConversorAFND.h"
#include "MinimizadorAFD.h"
#include "TesterEquivalencia.h"

Automata a;
a.setAlfabeto({"a", "b"});
Estado& q0 = a.agregarEstado("q0");
Estado& q1 = a.agregarEstado("q1");
q0.setEstadoInicial(true);
q1.setEstadoFinal(true);
q0.agregarTransicion(Transicion("a", std::vector<Estado*>{&q0, &q1}));
q0.agregarTransicion(Transicion("b", &q0));

bool inicial = q0.isEstadoInicial(); // true
bool aceptada = a.validarCadena(std::string("ba")); // true
q1.setId("aceptacion"); // Los punteros existentes siguen siendo validos.

Automata afd = ConversorAFND{}.convertirA_AFD(a);
Automata minimo = MinimizadorAFD{}.minimizar(afd);
ResultadoEquivalencia resultado = TesterEquivalencia{}.comparar(a, minimo);
// resultado.equivalentes == true
```

## Propiedad y vida útil

`Automata` es el propietario de los estados mediante `unique_ptr`; cada estado posee sus transiciones por valor. Los punteros `Estado*` de destinos son observadores y **no se liberan con delete**. Las direcciones de estados se mantienen estables al agregar otros estados. Copiar o mover el vector de transiciones no invalida las direcciones de los estados.

No se permite copiar un `Automata` directamente: `clonar()` hace copia profunda y reconstruye los destinos. Moverlo conserva direcciones y actualiza los propietarios usados para validar setters. Las copias de una `Transicion` son copias de sus punteros, no de los estados; cuando se agregan a un estado se comprueban y vinculan al propietario receptor.

Eliminar, reemplazar o destruir estados invalida los punteros externos que los señalaban. Las referencias a transiciones pueden invalidarse al agregar/eliminar transiciones o reemplazar la colección. No conserves referencias a transiciones a través de esas operaciones. En estados/transiciones independientes, antes de incorporarlos a un autómata, quien construye los objetos debe mantener vivos los destinos.

## Algoritmos y semántica

- `ConversorAFND`: construcción por subconjuntos con cierre epsilon y sumidero cuando corresponde. Produce AFD completo y alcanzable, con estados D0, D1, etc. `mover()` retorna solo destinos directos del símbolo; `clausuraEpsilon()` incluye los estados recibidos y todos los alcanzables por epsilon.
- `MinimizadorAFD`: elimina inaccesibles, determiniza y usa particiones estables. Devuelve estados M0, M1, etc. `particionarEstados()` requiere AFD, admite AFD parcial usando un sumidero virtual y devuelve grupos de punteros a los estados del argumento. Incluye estados inaccesibles si están en el argumento; el método de eliminación es independiente.
- `TesterEquivalencia::comparar()`: demostración exacta mediante BFS del producto sobre la unión de alfabetos. Si difieren, devuelve una palabra mínima en cantidad de símbolos. Contraejemplo vacío con `equivalentes=false` significa que difieren al aceptar epsilon.
- `probarEquivalencia()`: compara solamente las cadenas proporcionadas. Un resultado true **no demuestra equivalencia universal**. `generarCadenasDePrueba()` incluye epsilon y todas las cadenas hasta la longitud indicada. La UI distingue esta prueba limitada de la comparación exacta.

El símbolo vacío `""` representa epsilon y nunca un símbolo de entrada. La palabra vacía por tokens es `std::vector<std::string>{}`.

**Cadenas frente a tokens:** la sobrecarga `validarCadena(string)` interpreta cada byte como un símbolo y requiere un alfabeto de símbolos de un byte. Para símbolos de varios caracteres o UTF-8 usá `validarCadena(vector<string>)`, `generarPalabrasDePrueba()` y `probarEquivalenciaPorSimbolos()`. Así no se confunden `{"a","b"}` y `{"ab"}`. `comparar()` siempre compara lenguajes de tokens.

La determinización y la generación exhaustiva de cadenas pueden consumir memoria exponencial. La búsqueda de estados por id es lineal: el objetivo de esta implementación es claridad y funcionamiento, no optimización para millones de estados.

## Interfaz

El programa inicia con un ejemplo que acepta palabras sobre `{a,b}` terminadas en `a`. Ofrece un menú numérico y comandos de texto. `ayuda` muestra ambos.

Menú: 0 salir, 1 mostrar, 2 nuevo, 3 cargar, 4 guardar, 5 determinizar, 6 minimizar, 7 equivalencia exacta, 8 validar cadena, 9 ejemplo, 10 quitar inaccesibles, 11 mostrar particiones, 12 pruebas hasta una longitud. Las rutas que solicita el menú se escriben sin comillas; admite espacios.

Sesión de ejemplo:

```text
nuevo
estado q0 0
estado q1 1
inicial q0
transicion q0 a q0 q1
transicion q0 b q0
renombrar q1 final
mostrar
probar b a
cadena ba
guardar original.automata
determinizar
minimizar
equivalencia original.automata
salir
```

Comandos adicionales: `ejemplo`, `validar`, `final id 0/1`, `eliminar id`, `alfabeto simbolos...`, `cargar ruta`. `transicion q0 "" final` agrega epsilon. `probar` sin argumentos prueba la palabra vacía. Las rutas y nombres con espacios se entrecomillan en los comandos de texto.

Determinizar/minimizar/quitar inaccesibles reemplazan el autómata de trabajo. Guardá el original antes si querés conservarlo. Guardar sobrescribe la ruta indicada. Cargar solo reemplaza el actual si termina correctamente.

## Archivos y compatibilidad

Ahora se guarda `AUTOMATA_V2`, que también conserva símbolos declarados sin transiciones. Se siguen leyendo los archivos `AUTOMATA_V1` incluidos en `ejemplos` y creados con la primera versión. Los destinos se guardan por id y se reconstruyen como punteros al cargar; nunca se serializan direcciones de memoria.

Formato V2: cabecera, cantidad de símbolos, símbolos entrecomillados, cantidad de estados, estados con bandera final 0/1, id inicial, cantidad de transiciones, y transiciones con origen, símbolo, cantidad de destinos e ids de destino. El tipo se calcula al cargar y la bandera inicial se reconstruye desde el id inicial.

Nombres originales disponibles como alias en sus respectivos headers:

| Antes | Ahora |
|---|---|
| `UserInterface` | `InterfazUsuario` |
| `ConvertirDeterministico` | `ConversorAFND` |
| `LogicaMinimizar` | `MinimizadorAFD` |
| `GestorDeArchivos` | `AdministradorDeAutomatas` |
| `DemostrarEquivalencia` | `TesterEquivalencia` |

Se mantienen `convertir()`, `guardar()`, `cargar()`, `acepta()`, `estados()`, `inicial()`, `nombre()`, `esFinal()`, `transiciones()`, `simbolo()`, `destinos()` y los métodos de construcción anteriores. Los antiguos headers son alias, no implementaciones duplicadas.

## Pruebas incluidas

`tests/pruebas.cpp`: regresión de la primera versión, estabilidad de punteros, copia/movimiento, AFN y epsilon, archivos, equivalencia y enumeración de palabras de longitud 0 a 5 sobre 80 AFN con semilla fija.

`tests/pruebas_api.cpp`: setters inicial/final/id, sincronización, rechazo de ids repetidos y punteros ajenos, edición y reemplazo de transiciones, eliminación de estados, reemplazo de estados con referencias cruzadas, alfabeto explícito, tipo, métodos del diagrama, particiones, persistencia V2, menú numérico y compatibilidad de nombres.
