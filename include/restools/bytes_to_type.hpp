#pragma once 

#include <algorithm> // reverse_copy
#include <bit> // endian
#include <cstring> // memcpy

namespace restools
{
    enum class BytesToTypeStatus : short
    {
        Success,
        BufferIsOverflow,
        BufferIsOverlapping
    };

    enum class TypeToByteStatus : short
    {
        Success,
        BufferIsOverflow,
        BufferIsOverlapping
    };


    template <typename T>
    inline void bytesToTypeFast(const unsigned char* srcBytes, T& dstValue, bool isBigEndian)
    {
        bool needsReverse = ((std::endian::native == std::endian::little && isBigEndian) ||
            (std::endian::native == std::endian::big && !isBigEndian));

        if (needsReverse) {
            std::reverse_copy(srcBytes,
                srcBytes + sizeof(T),
                reinterpret_cast<unsigned char*>(&dstValue));
        }
        else {
            std::memcpy(reinterpret_cast<unsigned char*>(&dstValue),
                reinterpret_cast<const void*>(srcBytes),
                sizeof(T));
        }
    }

    template <typename T>
    BytesToTypeStatus bytesToTypeSafe(const unsigned char* srcBytes, size_t srcBytesSize, T& dstValue, bool reverseEndian)
    {
        constexpr size_t dstValueCapacity = sizeof(T);

        if (srcBytesSize < dstValueCapacity) {
            return BytesToTypeStatus::BufferIsOverflow;
        }

        unsigned char* dstAsBytes = reinterpret_cast<unsigned char*>(&dstValue);

        if ((srcBytes < dstAsBytes + dstValueCapacity) && (dstAsBytes < srcBytes + srcBytesSize)) {
            return BytesToTypeStatus::BufferIsOverlapping;
        }

        bytesToTypeFast(srcBytes, dstValue, reverseEndian);

        return BytesToTypeStatus::Success;
    }

    template <typename T>
    TypeToByteStatus typeToBytesSafe(const T& srcValue, unsigned char* dstBytes, size_t dstBytesCapacity, bool isBigEndian)
    {
        constexpr size_t srcValueSize = sizeof(T);

        if (dstBytesCapacity < srcValueSize) {
            return TypeToByteStatus::BufferIsOverflow;
        }

        const unsigned char* srcAsBytes = reinterpret_cast<const unsigned char*>(&srcValue);

        if ((srcAsBytes < dstBytes + dstBytesCapacity) && (dstBytes < srcAsBytes + srcValueSize)) {
            return TypeToByteStatus::BufferIsOverlapping;
        }

        bool needsReverse = (
            (std::endian::native == std::endian::little && isBigEndian) ||
            (std::endian::native == std::endian::big && !isBigEndian)
            );

        if (needsReverse) {
            std::reverse_copy(srcAsBytes, srcAsBytes + srcValueSize, dstBytes);
        }
        else {
            std::memcpy(dstBytes, srcAsBytes, srcValueSize);
        }

        return TypeToByteStatus::Success;
    }

}