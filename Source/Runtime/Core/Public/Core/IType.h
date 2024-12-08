#pragma once

namespace BD
{
using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using int8 = signed char;
using int16 = short;
using int32 = int;
using int64 = long long;

using float32 = float;
using float64 = double;

constexpr uint64 BD_KB = 1024u;
constexpr uint64 BD_MB = 1024u * BD_KB;
constexpr uint64 BD_GB = 1024u * BD_MB;

} // namespace BD