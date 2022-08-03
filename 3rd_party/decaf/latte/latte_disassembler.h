#pragma once

#include <span>
#include <string>
#include <cstdint>

namespace latte
{

std::string
disassemble(const std::span<const uint8_t> &binary, bool isSubroutine = false);

} // namespace latte
