#ifndef CONVERT_H
#define CONVERT_H

#include <type_traits>

template <typename Enum>
constexpr auto toUnderlying(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

template <typename Enum>
constexpr Enum toEnum(std::underlying_type_t<Enum> value) noexcept
{
    return static_cast<Enum>(value);
}

unsigned floatToUnsigned(float value);

float unsignedToFloat(unsigned value);

#endif // CONVERT_H