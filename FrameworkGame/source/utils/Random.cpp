#include "utils/Random.h"

namespace sfmx {

std::mt19937 Random::m_mt;
bool Random::m_init = false;

} // namespace sfmx
