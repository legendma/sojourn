#pragma once

typedef uint32_t Entity;

template <typename>
struct EntityTraits;

template <>
struct EntityTraits<uint32_t>
{
    using EntityIdType = uint32_t;
    using VersionType = uint16_t;
    static constexpr EntityIdType entity_mask = 0xfffff;
    static constexpr EntityIdType version_mask = 0xfff;
    static constexpr size_t entity_shift = 20;
};

