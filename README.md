# AutomataLife — transiciones unitarias + State Error `SR`

Esta version cambia el modelo interno para que coincida con la idea del principio del palomar:

- **Cada objeto `Transicion` representa una sola arista:** `simbolo -> destino`.
- Si una entrada vieja declara varios destinos para un mismo simbolo, se crean varias `Transicion` independientes.
- Todo automata con estados posee un estado interno reservado llamado **`SR` (State Error)**.
- Por cada simbolo del alfabeto que falte en un estado, se agrega automaticamente `simbolo -> SR`.
- `SR` es no inicial, no final y tiene un bucle a si mismo por cada simbolo.
- Si luego se agrega una transicion real para un simbolo que estaba apuntando a `SR`, el fallback a `SR` se elimina.
- Las aristas exactamente duplicadas `(origen, simbolo, destino)` se deduplican.

## Por que ahora funciona el principio del palomar

Despues de `completarEstadoError()`, para un alfabeto de tamano `N`, cada estado tiene **como minimo `N` transiciones no-epsilon**, una por simbolo.

Por lo tanto:

- `N` transiciones: hay exactamente una salida por simbolo -> determinista.
- mas de `N`: sobra al menos una transicion; por palomar algun simbolo se repite o existe una epsilon -> no determinista.

La implementacion es:

```cpp
bool Automata::esDeterminista() const {
    const auto cantidadSimbolos = getAlfabeto().size();

    for (const auto& estado : estados_) {
        const auto cantidadTransiciones = estado->getTransiciones().size();
        if (cantidadTransiciones > cantidadSimbolos) return false;
        if (cantidadTransiciones < cantidadSimbolos) return false; // defensa: estructura no normalizada
    }
    return true;
}
```

La segunda comparacion es defensiva: usando el API normal del proyecto no deberia ocurrir porque `SR` completa automaticamente los huecos.

## Ejemplo

Con alfabeto `{a,b,c}` y entrada:

```text
S0:
  a -> S1
  a -> S2
  c -> S3
```

el sistema completa:

```text
S0:
  a -> S1
  a -> S2
  b -> SR
  c -> S3

SR:
  a -> SR
  b -> SR
  c -> SR
```

`S0` tiene 4 transiciones para 3 simbolos, por lo que es AFND.

## API de `Transicion`

El almacenamiento real ahora es:

```cpp
std::string simbolo_;
Estado* destino_;
```

Ya no existe un vector de destinos dentro de una transicion. Se conservaron `getDestinos()`, `setDestinos()` y algunos nombres viejos solo como compatibilidad; una `Transicion` individual admite cero o un destino mientras esta suelta y exactamente uno cuando forma parte de un automata.

La operacion de alto nivel sigue aceptando una lista:

```cpp
a.agregarTransicion("S0", "a", {"S1", "S2"});
```

pero internamente queda como:

```text
a -> S1
a -> S2
```

## Estado `SR`

`SR` esta reservado para el sistema. No puede:

- crearse manualmente con `agregarEstado("SR")`;
- ser inicial;
- ser final;
- renombrarse;
- editarse manualmente.

`completarEstadoError()` lo crea y reconstruye sus bucles cuando cambia el alfabeto o la estructura del automata.

`SR` aparece al mostrar el automata en memoria, pero **no se guarda en disco**. Al cargar un archivo se genera otra vez. Esto evita persistir transiciones auxiliares.

## Archivos

Los archivos nuevos se guardan como **`AUTOMATA_V3`**. Cada linea de transicion contiene un solo destino.

Se siguen leyendo:

- `AUTOMATA_V1`;
- `AUTOMATA_V2`;
- XML anteriores donde una `<transicion>` tenga varios `<destino>`.

Al cargar formatos antiguos, una transicion con varios destinos se separa automaticamente en varias aristas unitarias.

El XML nuevo tambien guarda una `<transicion>` por arista. `SR` y los fallbacks hacia `SR` no se serializan.

## Algoritmos

`ConversorAFND`, cierre epsilon, `mover`, minimizacion, equivalencia, clonacion, carga/guardado e interfaz fueron adaptados al destino unico.

En los algoritmos de simulacion/subconjuntos, `SR` se trata como el conjunto vacio cuando corresponde, para que el estado de error interno no infle innecesariamente la determinización. En los AFD finales, `SR` sigue siendo el sumidero materializado.

## Compilar en Windows

Ejecuta:

```text
compilar_y_ejecutar.bat
```

El instalador comprueba CMake, MSVC C++ y Windows SDK. Si falta algo intenta instalarlo. Antes de compilar borra la carpeta de build anterior para evitar reutilizar `.obj` o `.exe` viejos despues de un pull.

Manualmente:

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Pruebas

Las pruebas cubren:

- transiciones unitarias;
- separacion de multiples destinos antiguos;
- generacion de `SR`;
- completado automatico;
- deduplicacion de aristas exactas;
- AFND por simbolos repetidos;
- AFND epsilon;
- determinización;
- minimizacion;
- equivalencia;
- V1/V2/V3 y XML;
- interfaz de consola.
