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

    typeToBytes(testTypeValue, dstBuffer, reverseEndian);
    bytesToType(dstBuffer, outType, reverseEndian);

    assert(memcmp(&testTypeValue, &outType, sizeof(T)) == 0);
}

template <typename T>
void testBytesToIntegerCheckValueForEndian(T testIntegerValue, bool reverseEndian)
{
    using namespace restools;

    T outInteger = 0;
    unsigned char dstBuffer[sizeof(T)] = { 0 };

    typeToBytes(testIntegerValue, dstBuffer, reverseEndian);
    bytesToType(dstBuffer, outInteger, reverseEndian);
    T outInteger2 = bytesToType<T>(dstBuffer, reverseEndian);

    assert(testIntegerValue == outInteger);
    assert(testIntegerValue == outInteger2);
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

void testBytesToInteger()
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
}