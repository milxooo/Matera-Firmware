// Cliente HTTP de POST /dispositivos/activar (HTTP plano, solo para red local de desarrollo).
#pragma once

#include <string>

namespace matera {
namespace activacion {

// Un intento. Devuelve el código HTTP (o negativo si hubo error de red). Si es 200, `cuerpo`
// trae la respuesta; el llamador la interpreta con extraerToken().
int intentar(const std::string& urlBase, const std::string& mac12, const std::string& codigo, std::string& cuerpo);

}  // namespace activacion
}  // namespace matera
