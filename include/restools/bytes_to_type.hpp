#pragma once 

#include <cstring> // memcpy
#include <algorithm> // reverse_copy

namespace restools
{
    template <typename T>
    inline T const& bytesToType(const unsigned char* srcBytes, T& dstValue, bool reverseEndian)
    {
        if (reverseEndian) {
            std::reverse_copy(srcBytes, srcBytes + sizeof(T), reinterpret_cast<unsigned char*>(&dstValue));
        }
        else {
            std::memcpy(reinterpret_cast<void*>(&dstValue), reinterpret_cast<const void*>(srcBytes), sizeof(T));
        }

        return dstValue;

    }

    template <typename T>
    inline T bytesToType(const unsigned char* srcBytes, bool reverseEndian)
    {
        T dstValue = 0;
        return bytesToType<T>(srcBytes, dstValue, reverseEndian);
    }

    template <typename T>
    inline unsigned char* typeToBytes(const T srcValue, unsigned char* dstBytes, bool reverseEndian)
    {
        if (reverseEndian) {
            const unsigned char* srcAsBytes = reinterpret_cast<const unsigned char*>(&srcValue);
            std::reverse_copy(srcAsBytes, srcAsBytes + sizeof(T), dstBytes);
        }
        else {
            memcpy(reinterpret_cast<void*>(dstBytes), reinterpret_cast<const void*>(&srcValue), sizeof(T));
        }

        return dstBytes;
    }
}