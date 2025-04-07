#include <vector>
#include <cassert>
#include <thread>

#include "restools/buffer_composer.hpp"

#define ASSERT_COMPOSED_BUFFER_COMPARE_RESULT(GENERATED_DATA_VEC, COMPOSED_BUFFER, COMPOSED_BUFFER_SIZE) \
    assert(GENERATED_DATA_VEC.size() == COMPOSED_BUFFER_SIZE && memcmp(GENERATED_DATA_VEC.data(), COMPOSED_BUFFER, COMPOSED_BUFFER_SIZE) == 0)

std::vector<unsigned char> generateRandomData(size_t size)
{
    static const unsigned char dictionary[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static constexpr size_t dictionarySize = sizeof(dictionary) - 1;

    std::vector<unsigned char> result;
    result.resize(size);
    size_t writtenTotal = 0;

    while(writtenTotal < size) {
        size_t writtenSize = std::min(dictionarySize, size - writtenTotal);
        std::memcpy(result.data() + writtenTotal, dictionary, writtenSize);
        writtenTotal += writtenSize;
    }

    return result;
}

template <typename COMPOSER>
void testBufferComposerWithDataSize(size_t testDataSize, COMPOSER& composer)
{
    using namespace restools;

    auto generatedData = generateRandomData(testDataSize);

    for (size_t shift = 1; shift < testDataSize + 1; ++shift) {
        composer.clear();
        size_t writtenTotal = 0;
        while (writtenTotal < testDataSize) {
            size_t toWriteSize = std::min(shift, testDataSize - writtenTotal);
            const unsigned char* toWrite = generatedData.data() + writtenTotal;
            writtenTotal += toWriteSize;
			
            BufferComposerSaveStatus status = composer.save(toWrite, toWriteSize);
            assert(status == BufferComposerSaveStatus::Success);
        }
		
		unsigned char* composedData = nullptr;
		size_t composedDataSize = 0;
        BufferComposerComposeStatus status = composer.compose(composedData, composedDataSize);
        assert(status == BufferComposerComposeStatus::Success);

        ASSERT_COMPOSED_BUFFER_COMPARE_RESULT(generatedData, composedData, composedDataSize);
    }
}

void testBufferComposerNegative()
{
    using namespace restools;
    buffer_composer<8, 2> composer(24, 32);

    assert(composer.save(nullptr, 0) == BufferComposerSaveStatus::NullBuffer);
    assert(composer.save(reinterpret_cast<const unsigned char*>(""), 0) == BufferComposerSaveStatus::ZeroBufferSize);

    auto generatedDataOverflow = generateRandomData(33);
    assert(composer.save(generatedDataOverflow.data(), 
        generatedDataOverflow.size()) == BufferComposerSaveStatus::MaxSavedBufferCountLimited);

    unsigned char* composedData = nullptr;
    size_t composedDataSize = 0;
    assert(composer.compose(composedData,
        composedDataSize) == BufferComposerComposeStatus::NoDataSaved);

    auto generatedDataForChunks = generateRandomData(32);
    assert(composer.save(generatedDataForChunks.data(),
        generatedDataForChunks.size() - 1) == BufferComposerSaveStatus::Success);

    assert(composer.compose(composedData,
        composedDataSize) == BufferComposerComposeStatus::Success);

    assert(composer.save(generatedDataForChunks.data() + 31, 1) == BufferComposerSaveStatus::NotClearedAfterCompose);
}

void testBufferComposerBufferIsOverlapping()
{
    using namespace restools;
    buffer_composer<16, 2> composer(16, 16);

    auto generatedDataOverflow = generateRandomData(12);
    assert(composer.save(generatedDataOverflow.data(), generatedDataOverflow.size()) == restools::BufferComposerSaveStatus::Success);

    unsigned char* composedData = nullptr;
    size_t composedDataSize = 0;

    assert(composer.compose(composedData, composedDataSize) == restools::BufferComposerComposeStatus::Success);
    composer.clear();

    for (size_t i = 0; i < composedDataSize; ++i) {
        assert(composer.save(composedData + i, composedDataSize - i) == restools::BufferComposerSaveStatus::BufferIsOverlapping);
    }
}

template <size_t STACK_BUFFER_MAX>
void testBufferComposerWithDataSizeInterval(
    size_t linearBufferMax, 
    size_t saveBufferMax, 
    size_t testDataInterval)
{
    using namespace restools;

    buffer_composer<STACK_BUFFER_MAX, 2> composer(linearBufferMax, saveBufferMax);

    for (size_t i = 1; i < testDataInterval + 1; ++i) {
        testBufferComposerWithDataSize(i, composer);
    }
}

template <size_t STACK_BUFFER_MAX>
void testBufferComposerWithDataSizeInterval(size_t interval)
{
    using namespace restools;

    for (
        size_t saveBufferMaxI = 8;
        saveBufferMaxI < interval + 1;
        ++saveBufferMaxI) {
        for (
            size_t linearBufferMaxI = 8;
            linearBufferMaxI < saveBufferMaxI + 1;
            ++linearBufferMaxI) {
            for (
                size_t testDataSizeI = 8;
                testDataSizeI < saveBufferMaxI + 1;
                ++testDataSizeI) {

                buffer_composer<STACK_BUFFER_MAX, 2> composer(linearBufferMaxI, saveBufferMaxI);
                testBufferComposerWithDataSize(testDataSizeI, composer);
            }
        }
    }
}


void testBufferComposerWithDataSizeInterval()
{
    testBufferComposerNegative();

    std::thread firstThread = std::thread([]()
    {
        testBufferComposerWithDataSizeInterval<8>(52);
        testBufferComposerWithDataSizeInterval<32>(52);
    });

    std::thread secondThread = std::thread([]()
    {
        testBufferComposerWithDataSizeInterval<56>(59);
        testBufferComposerWithDataSizeInterval<64>(64);
    });

    if (firstThread.joinable()) {
        firstThread.join();
    }

    if (secondThread.joinable()) {
        secondThread.join();
    }
}


void testBufferComposer()
{
    testBufferComposerNegative();
    testBufferComposerBufferIsOverlapping();
    testBufferComposerWithDataSizeInterval();
}
