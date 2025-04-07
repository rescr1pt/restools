#include <limits>
#include <cassert>

#include "restools/bytes_to_type.hpp"

#undef min
#undef max

template <typename T>
void testBytesToTypeCheckValueForEndian(T testTypeValue, bool reverseEndian)
{
    using namespace restools;

    T outType;
    memset(&outType, 0, sizeof(T));
    unsigned char dstBuffer[sizeof(T)] = { 0 };

    assert(typeToBytesSafe(testTypeValue, dstBuffer, sizeof(dstBuffer), reverseEndian) == TypeToByteStatus::Success);
    assert(bytesToTypeSafe(dstBuffer, sizeof(dstBuffer), outType, reverseEndian) == BytesToTypeStatus::Success);

    assert(memcmp(&testTypeValue, &outType, sizeof(T)) == 0);
}

template <typename T>
void testBytesToIntegerCheckValueForEndian(T testIntegerValue, bool reverseEndian)
{
    using namespace restools;

    T outInteger = 0;
    unsigned char dstBuffer[sizeof(T)] = { 0 };

    assert(typeToBytesSafe(testIntegerValue, dstBuffer, sizeof(dstBuffer), reverseEndian) == TypeToByteStatus::Success);
    assert(bytesToTypeSafe(dstBuffer, sizeof(dstBuffer), outInteger, reverseEndian) == BytesToTypeStatus::Success);

    assert(testIntegerValue == outInteger);
}

template <typename T>
void testBytesToIntegerCheckValue(T testIntegerValue)
{
    testBytesToIntegerCheckValueForEndian(testIntegerValue, false);
    testBytesToIntegerCheckValueForEndian(testIntegerValue, true);
}

template <typename T>
void testBytesToIntegerForType()
{
    testBytesToIntegerCheckValue(std::numeric_limits<T>::min());

    T testValueHighMiddle = (std::numeric_limits<T>::max() / 2) + 1;
    testBytesToIntegerCheckValue(testValueHighMiddle);

    T testValueLowMiddle = (std::numeric_limits<T>::max() / 2) - 1;
    testBytesToIntegerCheckValue(testValueLowMiddle);

    T testValuePreLast = (std::numeric_limits<T>::max() - 1);
    testBytesToIntegerCheckValue(testValuePreLast);

    testBytesToIntegerCheckValue(std::numeric_limits<T>::max());
}

void testBytesToTypeIsOverlapping()
{
    static constexpr size_t typeSizeOf = sizeof(unsigned long long);
    unsigned long long typeValue = 0x0123456789ABCDEF;
    unsigned char* testTypeAsBytes = reinterpret_cast<unsigned char*>(&typeValue);
    
    using namespace restools;

    for (size_t i = 0 ; i < typeSizeOf; ++i) {
        assert(typeToBytesSafe(typeValue, testTypeAsBytes + i, typeSizeOf, true) == TypeToByteStatus::BufferIsOverlapping);
        assert(bytesToTypeSafe(testTypeAsBytes + i, typeSizeOf, typeValue, true) == BytesToTypeStatus::BufferIsOverlapping);
    }
}

void testBytesToType()
{
    testBytesToIntegerForType<unsigned char>();
    testBytesToIntegerForType<char>();
    testBytesToIntegerForType<unsigned short>();
    testBytesToIntegerForType<short>();
    testBytesToIntegerForType<unsigned int>();
    testBytesToIntegerForType<int>();
    testBytesToIntegerForType<unsigned long>();
    testBytesToIntegerForType<long>();
    testBytesToIntegerForType<unsigned long long>();
    testBytesToIntegerForType<long long>();

    struct TestStruct
    {
        char schar = -125;
        unsigned short ushort = 0xABCF;
        int sint = 0x89ABCFEF;
        long long sllong = 0x0123456789ABCFEF;
    };

    TestStruct testStruct;
    testBytesToTypeCheckValueForEndian<TestStruct>(testStruct, false);
    testBytesToTypeCheckValueForEndian<TestStruct>(testStruct, true);

    testBytesToTypeIsOverlapping();
}