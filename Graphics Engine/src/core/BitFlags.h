#pragma once

template <typename EnumType>
    requires std::is_enum_v<EnumType>
constexpr EnumType operator|(EnumType l, EnumType r)
{
    using Underlying = std::underlying_type_t<EnumType>;
    return static_cast<EnumType>(static_cast<Underlying>(l) |
                                 static_cast<Underlying>(r));
}

template <typename EnumType>
    requires std::is_enum_v<EnumType>
constexpr EnumType operator&(EnumType l, EnumType r)
{
    using Underlying = std::underlying_type_t<EnumType>;
    return static_cast<EnumType>(static_cast<Underlying>(l) &
                                 static_cast<Underlying>(r));
}

template <typename EnumType>
    requires std::is_enum_v<EnumType>
constexpr EnumType operator~(EnumType l)
{
    using Underlying = std::underlying_type_t<EnumType>;
    return static_cast<EnumType>(~static_cast<Underlying>(l));
}