#include "Automata.h"
#include "TesterEquivalencia.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <iomanip>
#include <limits>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

namespace {

struct ConfiguracionStress {
    // CODIGO VIEJO: stress fijo de 100000 no equivalentes + 100000 equivalentes.
    /*
    std::uint32_t porSubgrupoNoEquivalente = 25000;
    std::uint32_t porSubgrupoEquivalente = 50000;
    */
    std::uint32_t porSubgrupoNoEquivalente = 0;
    std::uint32_t porSubgrupoEquivalente = 0;
    std::uint32_t hilos = 6;
};

struct DetallePares {
    std::uint64_t mismoAlfabetoSoloAFD = 0;
    std::uint64_t mismoAlfabetoConAFND = 0;
    std::uint64_t distintoAlfabetoSoloAFD = 0;
    std::uint64_t distintoAlfabetoConAFND = 0;
    std::uint64_t alfabetoIncompatibleSoloAFD = 0;
    std::uint64_t alfabetoIncompatibleConAFND = 0;
};

struct Conteo {
    std::uint64_t comparaciones = 0;
    std::uint64_t equivalentes = 0;
    std::uint64_t noEquivalentes = 0;
    std::uint64_t alfabetosIncompatibles = 0;
    DetallePares detalle;
};

void registrarDetalle(Conteo& conteo, bool mismoAlfabeto, bool mismoTamanoAlfabeto, bool hayAFND) {
    if (!mismoTamanoAlfabeto) {
        if (hayAFND) {++conteo.detalle.alfabetoIncompatibleConAFND;}
        else {++conteo.detalle.alfabetoIncompatibleSoloAFD;}
        return;
    }

    if (mismoAlfabeto) {
        if (hayAFND) {++conteo.detalle.mismoAlfabetoConAFND;}
        else {++conteo.detalle.mismoAlfabetoSoloAFD;}
        return;
    }

    if (hayAFND) {++conteo.detalle.distintoAlfabetoConAFND;}
    else {++conteo.detalle.distintoAlfabetoSoloAFD;}
}

void sumarDetalle(DetallePares& destino, const DetallePares& origen) {
    destino.mismoAlfabetoSoloAFD += origen.mismoAlfabetoSoloAFD;
    destino.mismoAlfabetoConAFND += origen.mismoAlfabetoConAFND;
    destino.distintoAlfabetoSoloAFD += origen.distintoAlfabetoSoloAFD;
    destino.distintoAlfabetoConAFND += origen.distintoAlfabetoConAFND;
    destino.alfabetoIncompatibleSoloAFD += origen.alfabetoIncompatibleSoloAFD;
    destino.alfabetoIncompatibleConAFND += origen.alfabetoIncompatibleConAFND;
}

std::uint64_t totalDetalle(const DetallePares& detalle) {
    return detalle.mismoAlfabetoSoloAFD + detalle.mismoAlfabetoConAFND +
           detalle.distintoAlfabetoSoloAFD + detalle.distintoAlfabetoConAFND +
           detalle.alfabetoIncompatibleSoloAFD + detalle.alfabetoIncompatibleConAFND;
}

struct ResultadoParalelo {
    Conteo conteo;
    bool correcto = true;
    std::string error;
};

std::uint64_t pares(std::uint64_t cantidad) {
    return cantidad * (cantidad - 1) / 2;
}

std::uint64_t comparacionesTotalesParaGrupo(std::uint64_t cantidadPorGrupo) {
    // Se comparan todos contra todos dentro de los dos grupos principales:
    // C(N,2) no equivalentes + C(N,2) equivalentes = N * (N - 1).
    return cantidadPorGrupo * (cantidadPorGrupo - 1);
}

ConfiguracionStress configurarDesdeMaximo(std::uint64_t maximoComparaciones) {
    if (maximoComparaciones < 56) {
        throw std::invalid_argument("El maximo debe permitir al menos 8 automatas por grupo (56 comparaciones)");
    }

    // Los dos grupos principales tienen la misma cantidad N. N debe ser multiplo de 8:
    // no equivalentes -> 4 alfabetos -> mitad AFD/AFND; equivalentes -> 2 alfabetos -> mitad AFD/AFND.
    long double raiz = std::sqrt(1.0L + 4.0L * static_cast<long double>(maximoComparaciones));
    std::uint64_t cantidadPorGrupo = static_cast<std::uint64_t>((1.0L + raiz) / 2.0L);
    cantidadPorGrupo -= cantidadPorGrupo % 8;

    // La familia de alfabetos de 4 simbolos codifica lenguajes con 16 bits.
    // Para mantener TODOS los automatas distintos, como maximo usamos 131072 por grupo principal.
    constexpr std::uint64_t maximoPorGrupoPorUnicidad = 131072;
    if (cantidadPorGrupo > maximoPorGrupoPorUnicidad) cantidadPorGrupo = maximoPorGrupoPorUnicidad;

    while (cantidadPorGrupo >= 8 && comparacionesTotalesParaGrupo(cantidadPorGrupo) > maximoComparaciones) {
        cantidadPorGrupo -= 8;
    }

    if (cantidadPorGrupo < 8) {
        throw std::invalid_argument("No existe una configuracion pareja dentro de ese maximo");
    }

    ConfiguracionStress config;
    config.porSubgrupoNoEquivalente = static_cast<std::uint32_t>(cantidadPorGrupo / 4);
    config.porSubgrupoEquivalente = static_cast<std::uint32_t>(cantidadPorGrupo / 2);
    return config;
}

std::string formatearDuracion(double segundos) {
    if (segundos < 0.0) segundos = 0.0;
    std::uint64_t total = static_cast<std::uint64_t>(segundos);
    const std::uint64_t dias = total / 86400;
    total %= 86400;
    const std::uint64_t horas = total / 3600;
    total %= 3600;
    const std::uint64_t minutos = total / 60;
    const std::uint64_t seg = total % 60;

    std::ostringstream salida;
    if (dias > 0) salida << dias << "d ";
    if (dias > 0 || horas > 0) salida << horas << "h ";
    if (dias > 0 || horas > 0 || minutos > 0) salida << minutos << "m ";
    salida << seg << "s";
    return salida.str();
}

struct ProgresoStress {
    std::uint64_t total = 0;
    std::atomic<std::uint64_t> completadas{0};
    std::chrono::steady_clock::time_point inicio = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point ultimaImpresion = inicio;
    std::mutex mutexImpresion;

    explicit ProgresoStress(std::uint64_t totalComparaciones) : total(totalComparaciones) {}

    void agregar(std::uint64_t cantidad) {
        const std::uint64_t hechas = completadas.fetch_add(cantidad, std::memory_order_relaxed) + cantidad;

        // Cada worker publica de a bloques. Solo uno imprime y como maximo cada 5 segundos.
        std::unique_lock<std::mutex> lock(mutexImpresion, std::try_to_lock);
        if (!lock.owns_lock()) return;

        const auto ahora = std::chrono::steady_clock::now();
        if (hechas < total && ahora - ultimaImpresion < std::chrono::seconds(5)) return;
        ultimaImpresion = ahora;

        const double segundos = std::chrono::duration<double>(ahora - inicio).count();
        const double velocidad = segundos > 0.0 ? static_cast<double>(hechas) / segundos : 0.0;
        const double porcentaje = total > 0 ? (100.0 * static_cast<double>(hechas) / static_cast<double>(total)) : 100.0;
        const std::uint64_t restantes = hechas < total ? total - hechas : 0;
        const double eta = velocidad > 0.0 ? static_cast<double>(restantes) / velocidad : 0.0;

        std::cout << "[PROGRESO] Comparaciones: " << hechas << " / " << total
                  << " (" << std::fixed << std::setprecision(4) << porcentaje << "%)"
                  << " | Velocidad: " << std::setprecision(0) << velocidad << " comp/s"
                  << " | ETA: " << formatearDuracion(eta)
                  << " | Transcurrido: " << formatearDuracion(segundos) << '\n';
    }

    void mostrarFinal() {
        std::lock_guard<std::mutex> lock(mutexImpresion);
        const auto ahora = std::chrono::steady_clock::now();
        const std::uint64_t hechas = completadas.load(std::memory_order_relaxed);
        const double segundos = std::chrono::duration<double>(ahora - inicio).count();
        const double velocidad = segundos > 0.0 ? static_cast<double>(hechas) / segundos : 0.0;
        std::cout << "[FINAL] Comparaciones: " << hechas << " / " << total
                  << " | Velocidad media: " << std::fixed << std::setprecision(0) << velocidad << " comp/s"
                  << " | Tiempo total: " << formatearDuracion(segundos) << '\n';
    }
};

// Permutacion afin sobre 2^bits. Como el multiplicador es impar, no hay
// colisiones: indices distintos producen codigos distintos mientras entren
// dentro del espacio de bits elegido. Da aspecto pseudoaleatorio sin perder
// la garantia matematica de unicidad.
std::uint64_t permutarSemilla(std::uint64_t indice, unsigned bits, std::uint64_t suma) {
    const std::uint64_t mascara = (std::uint64_t{1} << bits) - 1;
    constexpr std::uint64_t multiplicador = 0x9E3779B185EBCA87ULL; // impar
    return (indice * multiplicador + suma) & mascara;
}

std::vector<std::string> simbolos(const std::string& texto) {
    std::vector<std::string> resultado;
    resultado.reserve(texto.size());
    for (char c : texto) resultado.emplace_back(1, c);
    return resultado;
}

void agregarTransicionesAFD(Automata& a, const std::string& origen,
                            const std::vector<std::string>& alfabeto,
                            const std::vector<std::string>& destinos) {
    if (alfabeto.size() != destinos.size()) throw std::logic_error("Tabla AFD invalida");
    for (std::size_t i = 0; i < alfabeto.size(); ++i) {
        a.agregarTransicion(origen, alfabeto[i], {destinos[i]});
    }
}

// Lenguaje exacto para alfabeto de 4: un subconjunto de las 16 palabras de
// longitud 2. Los 16 bits de mascara dicen exactamente que palabras acepta.
Automata generarLenguajeLongitud2(const std::string& textoAlfabeto, std::uint16_t mascara,
                                  bool hacerloAFND) {
    const auto alfabeto = simbolos(textoAlfabeto);
    if (alfabeto.size() != 4) throw std::logic_error("Se esperaba alfabeto de 4 simbolos");

    Automata a;
    a.setAlfabeto(std::set<std::string>(alfabeto.begin(), alfabeto.end()));
    a.agregarEstado("Q0");
    for (int i = 0; i < 4; ++i) a.agregarEstado("P" + std::to_string(i));
    a.agregarEstado("F", true);
    a.agregarEstado("N");
    a.setEstadoInicial("Q0");

    std::vector<std::string> desdeInicial;
    for (int i = 0; i < 4; ++i) desdeInicial.push_back("P" + std::to_string(i));
    agregarTransicionesAFD(a, "Q0", alfabeto, desdeInicial);

    for (int i = 0; i < 4; ++i) {
        std::vector<std::string> destinos;
        for (int j = 0; j < 4; ++j) {
            const unsigned bit = static_cast<unsigned>(i * 4 + j);
            destinos.push_back((mascara & (std::uint16_t{1} << bit)) ? "F" : "N");
        }
        agregarTransicionesAFD(a, "P" + std::to_string(i), alfabeto, destinos);
    }

    agregarTransicionesAFD(a, "F", alfabeto, std::vector<std::string>(4, "N"));
    agregarTransicionesAFD(a, "N", alfabeto, std::vector<std::string>(4, "N"));

    // Epsilon self-loop: fuerza AFND sin cambiar el lenguaje.
    if (hacerloAFND) a.agregarTransicion("N", "", {"N"});
    return a;
}

// Lenguaje exacto para alfabeto de 3: un subconjunto de las 27 palabras de
// longitud 3. Los 27 bits identifican de forma unica el lenguaje.
Automata generarLenguajeLongitud3(const std::string& textoAlfabeto, std::uint32_t mascara,
                                  bool hacerloAFND) {
    const auto alfabeto = simbolos(textoAlfabeto);
    if (alfabeto.size() != 3) throw std::logic_error("Se esperaba alfabeto de 3 simbolos");

    Automata a;
    a.setAlfabeto(std::set<std::string>(alfabeto.begin(), alfabeto.end()));
    a.agregarEstado("Q0");
    for (int i = 0; i < 3; ++i) a.agregarEstado("P" + std::to_string(i));
    for (int i = 0; i < 9; ++i) a.agregarEstado("D" + std::to_string(i));
    a.agregarEstado("F", true);
    a.agregarEstado("N");
    a.setEstadoInicial("Q0");

    std::vector<std::string> nivel1;
    for (int i = 0; i < 3; ++i) nivel1.push_back("P" + std::to_string(i));
    agregarTransicionesAFD(a, "Q0", alfabeto, nivel1);

    for (int i = 0; i < 3; ++i) {
        std::vector<std::string> destinos;
        for (int j = 0; j < 3; ++j) destinos.push_back("D" + std::to_string(i * 3 + j));
        agregarTransicionesAFD(a, "P" + std::to_string(i), alfabeto, destinos);
    }

    for (int prefijo = 0; prefijo < 9; ++prefijo) {
        std::vector<std::string> destinos;
        for (int k = 0; k < 3; ++k) {
            const unsigned bit = static_cast<unsigned>(prefijo * 3 + k);
            destinos.push_back((mascara & (std::uint32_t{1} << bit)) ? "F" : "N");
        }
        agregarTransicionesAFD(a, "D" + std::to_string(prefijo), alfabeto, destinos);
    }

    agregarTransicionesAFD(a, "F", alfabeto, std::vector<std::string>(3, "N"));
    agregarTransicionesAFD(a, "N", alfabeto, std::vector<std::string>(3, "N"));

    if (hacerloAFND) a.agregarTransicion("N", "", {"N"});
    return a;
}

// Todos los automatas de esta familia aceptan exactamente la palabra de un
// simbolo formada por el PRIMER simbolo del alfabeto. El codigo variante solo
// cambia que copia equivalente del estado muerto se usa en las transiciones.
// Por eso podemos fabricar muchisimas estructuras distintas con igual lenguaje.
Automata generarEquivalente(const std::string& textoAlfabeto, std::uint64_t variante,
                            bool hacerloAFND) {
    const auto alfabeto = simbolos(textoAlfabeto);
    if (alfabeto.size() != 6) throw std::logic_error("Se esperaba alfabeto de 6 simbolos");

    Automata a;
    a.setAlfabeto(std::set<std::string>(alfabeto.begin(), alfabeto.end()));
    a.agregarEstado("Q0");
    a.agregarEstado("F", true);
    for (int i = 0; i < 4; ++i) a.agregarEstado("N" + std::to_string(i));
    a.setEstadoInicial("Q0");

    // 11 digitos base 4 alcanzan para mas de cuatro millones de variantes.
    std::uint64_t codigo = variante;
    auto siguienteMuerto = [&codigo]() {
        const int destino = static_cast<int>(codigo & 3ULL);
        codigo >>= 2;
        return "N" + std::to_string(destino);
    };

    a.agregarTransicion("Q0", alfabeto[0], {"F"});
    for (std::size_t i = 1; i < alfabeto.size(); ++i) {
        a.agregarTransicion("Q0", alfabeto[i], {siguienteMuerto()});
    }

    for (const auto& s : alfabeto) a.agregarTransicion("F", s, {siguienteMuerto()});

    // Todos los N pertenecen a la misma clase de rechazo. Sus tablas pueden
    // variar con la semilla sin cambiar el lenguaje visto desde Q0.
    for (int n = 0; n < 4; ++n) {
        for (const auto& s : alfabeto) {
            // Mezcla determinista extra. No participa en la garantia de unicidad.
            codigo = codigo * 6364136223846793005ULL + 1442695040888963407ULL;
            a.agregarTransicion("N" + std::to_string(n), s,
                                {"N" + std::to_string(static_cast<int>((codigo >> 32) & 3ULL))});
        }
    }

    if (hacerloAFND) a.agregarTransicion("Q0", "", {"Q0"});
    return a;
}

struct CasoNoEquivalente {
    Automata automata;
    std::string alfabeto;
    std::size_t tamanoAlfabeto = 0;
    bool afnd = false;
};

struct CasoEquivalente {
    Automata automata;
    std::string alfabeto;
    bool afnd = false;
};

CasoNoEquivalente generarNoEquivalente(std::uint32_t indice, const ConfiguracionStress& config) {
    const std::uint32_t n = config.porSubgrupoNoEquivalente;
    if (indice >= 4 * n) throw std::out_of_range("Indice no equivalente fuera de rango");

    const std::uint32_t grupo = indice / n;
    const std::uint32_t local = indice % n;
    const bool afnd = local >= n / 2;

    if (grupo < 2) {
        // Los grupos abcd y efgh comparten la misma numeracion de lenguajes.
        // rangoLenguaje es 0..(2*n-1), por lo que ninguna mascara se repite.
        const std::uint32_t rangoLenguaje = grupo * n + local;
        const auto codigo = static_cast<std::uint16_t>(
            permutarSemilla(rangoLenguaje, 16, 0x4D35ULL));
        const std::string alfabeto = grupo == 0 ? "abcd" : "efgh";
        return {generarLenguajeLongitud2(alfabeto, codigo, afnd), alfabeto, 4, afnd};
    }

    // Mismo principio para abc/def, usando 27 bits porque hay 3^3 palabras.
    const std::uint32_t rangoLenguaje = (grupo - 2) * n + local;
    const auto codigo = static_cast<std::uint32_t>(
        permutarSemilla(rangoLenguaje, 27, 0x13579BULL));
    const std::string alfabeto = grupo == 2 ? "abc" : "def";
    return {generarLenguajeLongitud3(alfabeto, codigo, afnd), alfabeto, 3, afnd};
}

// CODIGO VIEJO:
// Automata generarCasoEquivalente(std::uint32_t indice, const ConfiguracionStress& config) {
CasoEquivalente generarCasoEquivalente(std::uint32_t indice, const ConfiguracionStress& config) {
    const std::uint32_t n = config.porSubgrupoEquivalente;
    if (indice >= 2 * n) throw std::out_of_range("Indice equivalente fuera de rango");

    const std::uint32_t grupo = indice / n;
    const std::uint32_t local = indice % n;
    const bool afnd = local >= n / 2;
    const std::string alfabeto = grupo == 0 ? "abcdef" : "ghijkl";

    // 22 bits = 11 digitos base 4. La permutacion hace que los 100000 casos
    // sigan siendo unicos aun despues de renombrar ghijkl -> abcdef.
    const auto variante = permutarSemilla(indice, 22, 0x2A55AAULL);
    return {generarEquivalente(alfabeto, variante, afnd), alfabeto, afnd};
}

void registrarError(std::atomic<bool>& detener, std::mutex& mutexError, std::string& error,
                    const std::string& mensaje) {
    bool esperado = false;
    if (detener.compare_exchange_strong(esperado, true)) {
        std::lock_guard<std::mutex> lock(mutexError);
        error = mensaje;
    }
}

// CODIGO VIEJO:
// ResultadoParalelo comprobarEquivalentes(const ConfiguracionStress& config) {
ResultadoParalelo comprobarEquivalentes(const ConfiguracionStress& config, ProgresoStress& progreso) {
    const std::uint32_t total = 2 * config.porSubgrupoEquivalente;
    std::atomic<std::uint32_t> siguienteI{0};
    std::atomic<bool> detener{false};
    std::mutex mutexError;
    std::string error;
    std::vector<Conteo> locales(config.hilos);
    std::vector<std::thread> hilos;

    for (std::uint32_t numeroHilo = 0; numeroHilo < config.hilos; ++numeroHilo) {
        hilos.emplace_back([&, numeroHilo] {
            TesterEquivalencia tester;
            auto& conteo = locales[numeroHilo];
            std::uint64_t pendientesProgreso = 0;

            while (!detener.load(std::memory_order_relaxed)) {
                const std::uint32_t i = siguienteI.fetch_add(1);
                if (i + 1 >= total) break;

                CasoEquivalente primero = generarCasoEquivalente(i, config);
                for (std::uint32_t j = i + 1; j < total; ++j) {
                    if (detener.load(std::memory_order_relaxed)) break;
                    CasoEquivalente segundo = generarCasoEquivalente(j, config);
                    const bool mismoAlfabeto = primero.alfabeto == segundo.alfabeto;
                    const bool hayAFND = primero.afnd || segundo.afnd;
                    const bool equivalente = tester.compararAFDPorIteraciones(
                        primero.automata, segundo.automata, false);
                    ++conteo.comparaciones;
                    ++pendientesProgreso;
                    if (pendientesProgreso >= 10000) {progreso.agregar(pendientesProgreso); pendientesProgreso = 0;}

                    if (!equivalente) {
                        registrarError(detener, mutexError, error,
                            "Falso negativo entre equivalentes: indices " +
                            std::to_string(i) + " y " + std::to_string(j));
                        break;
                    }
                    ++conteo.equivalentes;
                    registrarDetalle(conteo, mismoAlfabeto, true, hayAFND);
                }
            }
            if (pendientesProgreso > 0) progreso.agregar(pendientesProgreso);
        });
    }

    for (auto& hilo : hilos) hilo.join();

    ResultadoParalelo resultado;
    resultado.correcto = !detener.load();
    resultado.error = error;
    for (const auto& local : locales) {
        resultado.conteo.comparaciones += local.comparaciones;
        resultado.conteo.equivalentes += local.equivalentes;
        resultado.conteo.noEquivalentes += local.noEquivalentes;
        resultado.conteo.alfabetosIncompatibles += local.alfabetosIncompatibles;
        sumarDetalle(resultado.conteo.detalle, local.detalle);
    }
    return resultado;
}

// CODIGO VIEJO:
// ResultadoParalelo comprobarNoEquivalentes(const ConfiguracionStress& config) {
ResultadoParalelo comprobarNoEquivalentes(const ConfiguracionStress& config, ProgresoStress& progreso) {
    const std::uint32_t n = config.porSubgrupoNoEquivalente;
    const std::uint32_t total = 4 * n;
    std::atomic<std::uint32_t> siguienteI{0};
    std::atomic<bool> detener{false};
    std::mutex mutexError;
    std::string error;
    std::vector<Conteo> locales(config.hilos);
    std::vector<std::thread> hilos;

    for (std::uint32_t numeroHilo = 0; numeroHilo < config.hilos; ++numeroHilo) {
        hilos.emplace_back([&, numeroHilo] {
            TesterEquivalencia tester;
            auto& conteo = locales[numeroHilo];
            std::uint64_t pendientesProgreso = 0;

            while (!detener.load(std::memory_order_relaxed)) {
                const std::uint32_t i = siguienteI.fetch_add(1);
                if (i + 1 >= total) break;

                CasoNoEquivalente primero = generarNoEquivalente(i, config);
                for (std::uint32_t j = i + 1; j < total; ++j) {
                    if (detener.load(std::memory_order_relaxed)) break;
                    CasoNoEquivalente segundo = generarNoEquivalente(j, config);
                    const bool mismoAlfabeto = primero.alfabeto == segundo.alfabeto;
                    const bool mismoTamanoAlfabeto = primero.tamanoAlfabeto == segundo.tamanoAlfabeto;
                    const bool hayAFND = primero.afnd || segundo.afnd;
                    ++conteo.comparaciones;
                    registrarDetalle(conteo, mismoAlfabeto, mismoTamanoAlfabeto, hayAFND);
                    ++pendientesProgreso;
                    if (pendientesProgreso >= 10000) {progreso.agregar(pendientesProgreso); pendientesProgreso = 0;}

                    if (primero.tamanoAlfabeto != segundo.tamanoAlfabeto) {
                        try {
                            (void)tester.compararAFDPorIteraciones(primero.automata, segundo.automata, false);
                            registrarError(detener, mutexError, error,
                                "Se esperaba error por alfabetos de distinto tamano: indices " +
                                std::to_string(i) + " y " + std::to_string(j));
                            break;
                        } catch (const std::invalid_argument&) {
                            ++conteo.alfabetosIncompatibles;
                        }
                        continue;
                    }

                    const bool equivalente = tester.compararAFDPorIteraciones(
                        primero.automata, segundo.automata, false);
                    if (equivalente) {
                        registrarError(detener, mutexError, error,
                            "Falso positivo entre no equivalentes: indices " +
                            std::to_string(i) + " y " + std::to_string(j));
                        break;
                    }
                    ++conteo.noEquivalentes;
                }
            }
            if (pendientesProgreso > 0) progreso.agregar(pendientesProgreso);
        });
    }

    for (auto& hilo : hilos) hilo.join();

    ResultadoParalelo resultado;
    resultado.correcto = !detener.load();
    resultado.error = error;
    for (const auto& local : locales) {
        resultado.conteo.comparaciones += local.comparaciones;
        resultado.conteo.equivalentes += local.equivalentes;
        resultado.conteo.noEquivalentes += local.noEquivalentes;
        resultado.conteo.alfabetosIncompatibles += local.alfabetosIncompatibles;
        sumarDetalle(resultado.conteo.detalle, local.detalle);
    }
    return resultado;
}

void verificarGeneracion(const ConfiguracionStress& config) {
    // Solo prueba las fronteras de cada subgrupo. La unicidad completa no se
    // verifica guardando 200000 objetos: esta garantizada por las permutaciones
    // sin colisiones usadas para construir cada semilla/codigo.
    for (std::uint32_t grupo = 0; grupo < 4; ++grupo) {
        const std::uint32_t base = grupo * config.porSubgrupoNoEquivalente;
        auto afd = generarNoEquivalente(base, config);
        auto afnd = generarNoEquivalente(base + config.porSubgrupoNoEquivalente / 2, config);
        if (!afd.automata.esDeterminista()) throw std::runtime_error("Se esperaba AFD en grupo no equivalente");
        if (afnd.automata.esDeterminista()) throw std::runtime_error("Se esperaba AFND en grupo no equivalente");
    }

    for (std::uint32_t grupo = 0; grupo < 2; ++grupo) {
        const std::uint32_t base = grupo * config.porSubgrupoEquivalente;
        auto afd = generarCasoEquivalente(base, config);
        auto afnd = generarCasoEquivalente(base + config.porSubgrupoEquivalente / 2, config);
        if (!afd.automata.esDeterminista()) throw std::runtime_error("Se esperaba AFD en grupo equivalente");
        if (afnd.automata.esDeterminista()) throw std::runtime_error("Se esperaba AFND en grupo equivalente");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        ConfiguracionStress config;
        bool smoke = false;
        std::uint64_t maximoPedido = 0;

        if (argc > 1 && std::string(argv[1]) == "--smoke") {
            // Misma distribucion y mismos seis hilos, pero pocos casos para CTest.
            config.porSubgrupoNoEquivalente = 8;
            config.porSubgrupoEquivalente = 16;
            smoke = true;
        } else {
            std::cout << "Maximo de comparaciones que queres ejecutar: ";
            if (!(std::cin >> maximoPedido)) {
                throw std::invalid_argument("El maximo de comparaciones debe ser un numero entero");
            }
            config = configurarDesdeMaximo(maximoPedido);
        }

        if (config.porSubgrupoNoEquivalente % 2 != 0 ||
            config.porSubgrupoEquivalente % 2 != 0) {
            throw std::logic_error("Cada subgrupo debe tener cantidad par para dividir AFD/AFND exactamente a la mitad");
        }

        const std::uint64_t totalNoEq = 4ULL * config.porSubgrupoNoEquivalente;
        const std::uint64_t totalEq = 2ULL * config.porSubgrupoEquivalente;
        const std::uint64_t mitadNoEq = 2ULL * config.porSubgrupoNoEquivalente;

        const std::uint64_t esperadoEq = pares(totalEq);
        const std::uint64_t esperadoNoEqComparables = 2ULL * pares(mitadNoEq);
        const std::uint64_t esperadoIncompatibles = mitadNoEq * mitadNoEq;
        const std::uint64_t esperadoNoEqTotal = pares(totalNoEq);

        const std::uint64_t mitadSubgrupoNoEq = config.porSubgrupoNoEquivalente / 2ULL;
        const std::uint64_t mitadSubgrupoEq = config.porSubgrupoEquivalente / 2ULL;

        DetallePares esperadoDetalleNoEq;
        esperadoDetalleNoEq.mismoAlfabetoSoloAFD = 4ULL * pares(mitadSubgrupoNoEq);
        esperadoDetalleNoEq.mismoAlfabetoConAFND = 4ULL * (pares(mitadSubgrupoNoEq) + mitadSubgrupoNoEq * mitadSubgrupoNoEq);
        esperadoDetalleNoEq.distintoAlfabetoSoloAFD = 2ULL * mitadSubgrupoNoEq * mitadSubgrupoNoEq;
        esperadoDetalleNoEq.distintoAlfabetoConAFND = 6ULL * mitadSubgrupoNoEq * mitadSubgrupoNoEq;
        esperadoDetalleNoEq.alfabetoIncompatibleSoloAFD = 4ULL * mitadSubgrupoNoEq * mitadSubgrupoNoEq;
        esperadoDetalleNoEq.alfabetoIncompatibleConAFND = 12ULL * mitadSubgrupoNoEq * mitadSubgrupoNoEq;

        DetallePares esperadoDetalleEq;
        esperadoDetalleEq.mismoAlfabetoSoloAFD = 2ULL * pares(mitadSubgrupoEq);
        esperadoDetalleEq.mismoAlfabetoConAFND = 2ULL * (pares(mitadSubgrupoEq) + mitadSubgrupoEq * mitadSubgrupoEq);
        esperadoDetalleEq.distintoAlfabetoSoloAFD = mitadSubgrupoEq * mitadSubgrupoEq;
        esperadoDetalleEq.distintoAlfabetoConAFND = 3ULL * mitadSubgrupoEq * mitadSubgrupoEq;

        // CODIGO VIEJO:
        // std::cout << (smoke ? "STRESS SMOKE" : "STRESS COMPLETO 200000") << '\n';
        std::cout << (smoke ? "STRESS SMOKE" : "STRESS CONFIGURABLE") << '\n';
        std::cout << "Hilos: " << config.hilos << '\n';
        std::cout << "No equivalentes generados: " << totalNoEq << '\n';
        std::cout << "  abcd: " << config.porSubgrupoNoEquivalente << " ("
                  << config.porSubgrupoNoEquivalente / 2 << " AFD + "
                  << config.porSubgrupoNoEquivalente / 2 << " AFND)\n";
        std::cout << "  efgh: " << config.porSubgrupoNoEquivalente << " ("
                  << config.porSubgrupoNoEquivalente / 2 << " AFD + "
                  << config.porSubgrupoNoEquivalente / 2 << " AFND)\n";
        std::cout << "  abc:  " << config.porSubgrupoNoEquivalente << " ("
                  << config.porSubgrupoNoEquivalente / 2 << " AFD + "
                  << config.porSubgrupoNoEquivalente / 2 << " AFND)\n";
        std::cout << "  def:  " << config.porSubgrupoNoEquivalente << " ("
                  << config.porSubgrupoNoEquivalente / 2 << " AFD + "
                  << config.porSubgrupoNoEquivalente / 2 << " AFND)\n";
        std::cout << "  Esperados false (mismo tamano de alfabeto): " << esperadoNoEqComparables << '\n';
        std::cout << "  Esperados error (alfabeto 4 vs 3): " << esperadoIncompatibles << '\n';
        std::cout << "  Pares totales del grupo: " << esperadoNoEqTotal << '\n';
        std::cout << "Equivalentes generados: " << totalEq << '\n';
        std::cout << "  abcdef: " << config.porSubgrupoEquivalente << " ("
                  << config.porSubgrupoEquivalente / 2 << " AFD + "
                  << config.porSubgrupoEquivalente / 2 << " AFND)\n";
        std::cout << "  ghijkl: " << config.porSubgrupoEquivalente << " ("
                  << config.porSubgrupoEquivalente / 2 << " AFD + "
                  << config.porSubgrupoEquivalente / 2 << " AFND)\n";
        std::cout << "  Esperados true: " << esperadoEq << '\n';

        verificarGeneracion(config);

        const std::uint64_t totalComparacionesEsperadas = esperadoNoEqTotal + esperadoEq;
        ProgresoStress progreso(totalComparacionesEsperadas);
        if (!smoke) {
            std::cout << "Maximo pedido: " << maximoPedido << '\n';
            std::cout << "Maximo ajustado para reparto exacto: " << totalComparacionesEsperadas << '\n';
            std::cout << "Automatas totales generados: " << (totalNoEq + totalEq) << '\n';
        }
        std::cout << "Comparaciones totales esperadas: " << totalComparacionesEsperadas << '\n';
        std::cout << "El progreso se actualiza aproximadamente cada 5 segundos.\n";

        // CODIGO VIEJO:
        /*
        auto noEq = comprobarNoEquivalentes(config);
        */
        auto noEq = comprobarNoEquivalentes(config, progreso);
        if (!noEq.correcto) throw std::runtime_error(noEq.error);
        if (noEq.conteo.comparaciones != esperadoNoEqTotal ||
            noEq.conteo.noEquivalentes != esperadoNoEqComparables ||
            noEq.conteo.alfabetosIncompatibles != esperadoIncompatibles ||
            noEq.conteo.equivalentes != 0 ||
            noEq.conteo.detalle.mismoAlfabetoSoloAFD != esperadoDetalleNoEq.mismoAlfabetoSoloAFD ||
            noEq.conteo.detalle.mismoAlfabetoConAFND != esperadoDetalleNoEq.mismoAlfabetoConAFND ||
            noEq.conteo.detalle.distintoAlfabetoSoloAFD != esperadoDetalleNoEq.distintoAlfabetoSoloAFD ||
            noEq.conteo.detalle.distintoAlfabetoConAFND != esperadoDetalleNoEq.distintoAlfabetoConAFND ||
            noEq.conteo.detalle.alfabetoIncompatibleSoloAFD != esperadoDetalleNoEq.alfabetoIncompatibleSoloAFD ||
            noEq.conteo.detalle.alfabetoIncompatibleConAFND != esperadoDetalleNoEq.alfabetoIncompatibleConAFND ||
            totalDetalle(noEq.conteo.detalle) != esperadoNoEqTotal) {
            throw std::runtime_error("Conteos inesperados en grupo no equivalente");
        }

        // CODIGO VIEJO:
        /*
        auto eq = comprobarEquivalentes(config);
        */
        auto eq = comprobarEquivalentes(config, progreso);
        if (!eq.correcto) throw std::runtime_error(eq.error);
        if (eq.conteo.comparaciones != esperadoEq || eq.conteo.equivalentes != esperadoEq ||
            eq.conteo.noEquivalentes != 0 || eq.conteo.alfabetosIncompatibles != 0 ||
            eq.conteo.detalle.mismoAlfabetoSoloAFD != esperadoDetalleEq.mismoAlfabetoSoloAFD ||
            eq.conteo.detalle.mismoAlfabetoConAFND != esperadoDetalleEq.mismoAlfabetoConAFND ||
            eq.conteo.detalle.distintoAlfabetoSoloAFD != esperadoDetalleEq.distintoAlfabetoSoloAFD ||
            eq.conteo.detalle.distintoAlfabetoConAFND != esperadoDetalleEq.distintoAlfabetoConAFND ||
            eq.conteo.detalle.alfabetoIncompatibleSoloAFD != 0 ||
            eq.conteo.detalle.alfabetoIncompatibleConAFND != 0 ||
            totalDetalle(eq.conteo.detalle) != esperadoEq) {
            throw std::runtime_error("Conteos inesperados en grupo equivalente");
        }

        progreso.mostrarFinal();

        // CODIGO VIEJO: resumen corto sin distinguir alfabeto ni presencia de AFND.
        /*
        std::cout << "OK no equivalentes: " << noEq.conteo.noEquivalentes << " false + "
                  << noEq.conteo.alfabetosIncompatibles << " incompatibles.\n";
        std::cout << "OK equivalentes: " << eq.conteo.equivalentes << " true.\n";
        std::cout << "OK comparaciones totales: "
                  << (noEq.conteo.comparaciones + eq.conteo.comparaciones) << '\n';
        */

        std::cout << "\n============================================================\n";
        std::cout << " RESUMEN FINAL DETALLADO\n";
        std::cout << "============================================================\n\n";

        std::cout << noEq.conteo.comparaciones << " pares generados como NO EQUIVALENTES. De esos:\n";
        std::cout << "  " << noEq.conteo.detalle.mismoAlfabetoSoloAFD
                  << " -> mismo alfabeto y no habia AFND (AFD + AFD).\n";
        std::cout << "  " << noEq.conteo.detalle.mismoAlfabetoConAFND
                  << " -> mismo alfabeto y habia al menos un AFND.\n";
        std::cout << "  " << noEq.conteo.detalle.distintoAlfabetoSoloAFD
                  << " -> distinto alfabeto, mismo tamano, y no habia AFND (AFD + AFD).\n";
        std::cout << "  " << noEq.conteo.detalle.distintoAlfabetoConAFND
                  << " -> distinto alfabeto, mismo tamano, y habia al menos un AFND.\n";
        std::cout << "  " << noEq.conteo.detalle.alfabetoIncompatibleSoloAFD
                  << " -> alfabetos de distinto tamano y no habia AFND (error esperado).\n";
        std::cout << "  " << noEq.conteo.detalle.alfabetoIncompatibleConAFND
                  << " -> alfabetos de distinto tamano y habia al menos un AFND (error esperado).\n";
        std::cout << "  Resultado: " << noEq.conteo.noEquivalentes << " devolvieron false y "
                  << noEq.conteo.alfabetosIncompatibles << " dieron incompatibilidad de alfabeto.\n";

        std::cout << "\n------------------------------------------------------------\n\n";

        std::cout << eq.conteo.equivalentes << " pares EQUIVALENTES. De esos:\n";
        std::cout << "  " << eq.conteo.detalle.mismoAlfabetoSoloAFD
                  << " -> mismo alfabeto y no habia AFND (AFD + AFD).\n";
        std::cout << "  " << eq.conteo.detalle.mismoAlfabetoConAFND
                  << " -> mismo alfabeto y habia al menos un AFND.\n";
        std::cout << "  " << eq.conteo.detalle.distintoAlfabetoSoloAFD
                  << " -> distinto alfabeto y no habia AFND (AFD + AFD).\n";
        std::cout << "  " << eq.conteo.detalle.distintoAlfabetoConAFND
                  << " -> distinto alfabeto y habia al menos un AFND.\n";

        std::cout << "\nComparaciones totales verificadas: "
                  << (noEq.conteo.comparaciones + eq.conteo.comparaciones) << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "STRESS ERROR: " << e.what() << '\n';
        return 1;
    }
}
