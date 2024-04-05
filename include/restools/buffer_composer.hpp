#pragma once  

#include <cstring>
#include <list>

namespace restools
{
    enum class BufferComposerSaveStatus : short
    {
        Success,
        NullBuffer,
        ZeroBufferSize,
        MaxSavedBufferCountLimited,
        NotCleanedAfterComposed,
    };

    enum class BufferComposerComposeStatus : short
    {
        Success,
        NoDataSaved,
        NoChunkSaved,
        LogicErrorWhenComposingFromBuffer,
        LogicErrorWhenComposingFromChunks,
        ComposedDataFromChunksDontMatchToSavedCount,
    };

    template <size_t STACK_BUFFER_MAX = 255 /*255b*/,
        uint16_t LINEAR_BUFFER_MULTIPLIER = 2>
    class buffer_composer
    {
        static_assert(STACK_BUFFER_MAX > 0 && STACK_BUFFER_MAX % 8 == 0, "STACK_BUFFER_MAX is not divisible to 8");
        static_assert(LINEAR_BUFFER_MULTIPLIER > 1, "LINEAR_BUFFER_MULTIPLIER < 2");

    public:
        buffer_composer(size_t linearBufferMaxSize, size_t saveBufferMaxCount)
            : linearBufferMaxSize_(linearBufferMaxSize)
            , saveBufferMaxCount_(saveBufferMaxCount)
        {
            stackBuffer_[0] = 0;
        }

        buffer_composer(const buffer_composer& rv) = delete;
        buffer_composer(buffer_composer&&) = delete;
        ~buffer_composer()
        {
            cleanup();
        }

        buffer_composer& operator=(const buffer_composer& source) = delete;
        buffer_composer& operator=(buffer_composer&& source) = delete;

        BufferComposerSaveStatus save(const unsigned char* buffer, size_t bufferSize) noexcept
        {
            if (composedBufferFromChunks_) {
                return BufferComposerSaveStatus::NotCleanedAfterComposed;
            }

            if (!buffer) {
                return BufferComposerSaveStatus::NullBuffer;
            }

            if (bufferSize == 0) {
                return BufferComposerSaveStatus::ZeroBufferSize;
            }

            const size_t nextSavedCount = savedCount_ + bufferSize;

            if (nextSavedCount > saveBufferMaxCount_) {
                return BufferComposerSaveStatus::MaxSavedBufferCountLimited;
            }

            if (nextSavedCount <= STACK_BUFFER_MAX) {
                std::memcpy(stackBuffer_ + savedCount_, buffer, bufferSize);
                savedCount_ = nextSavedCount;
                inBufferSavedCount_ = nextSavedCount;
                return BufferComposerSaveStatus::Success;
            }

            if (nextSavedCount <= linearBufferMaxSize_) {
                if (!linearBuffer_) {
                    size_t allocatedSize = std::min(linearBufferMaxSize_, nextSavedCount * LINEAR_BUFFER_MULTIPLIER);
                    linearBuffer_ = new unsigned char[allocatedSize];
                    linearBufferAllocatedSize_ = allocatedSize;
                }
                else if (nextSavedCount > linearBufferAllocatedSize_) {
                    size_t allocatedSize = std::min(linearBufferMaxSize_, (linearBufferAllocatedSize_ * LINEAR_BUFFER_MULTIPLIER) + nextSavedCount);
                    unsigned char* nextLinearBuffer = new unsigned char[allocatedSize];
                    std::memcpy(nextLinearBuffer, linearBuffer_, linearBufferAllocatedSize_);
                    delete[]linearBuffer_;
                    linearBuffer_ = nextLinearBuffer;
                    linearBufferAllocatedSize_ = allocatedSize;
                }

                if (savedCount_ > 0 && savedCount_ <= STACK_BUFFER_MAX) {
                    std::memcpy(static_cast<unsigned char*>(linearBuffer_), stackBuffer_, savedCount_);
                }

                std::memcpy(static_cast<unsigned char*>(linearBuffer_ + savedCount_), buffer, bufferSize);
                savedCount_ = nextSavedCount;
                inBufferSavedCount_ = savedCount_;
                return BufferComposerSaveStatus::Success;
            }

            chunks_.emplace_back();
            Chunk& chunk = chunks_.back();
            chunk.first = new unsigned char[bufferSize];
            memcpy(chunk.first, buffer, bufferSize);
            chunk.second = bufferSize;
            savedCount_ = nextSavedCount;

            return BufferComposerSaveStatus::Success;
        }

        BufferComposerComposeStatus compose(unsigned char*& composedData, size_t& composedDataSize) noexcept
        {
            if (savedCount_ == 0) {
                return BufferComposerComposeStatus::NoDataSaved;
            }

            if (savedCount_ <= STACK_BUFFER_MAX) {
                composedData = stackBuffer_;
                composedDataSize = savedCount_;
                return BufferComposerComposeStatus::Success;
            }

            if (savedCount_ <= linearBufferMaxSize_) {
                composedData = linearBuffer_;
                composedDataSize = savedCount_;
                return BufferComposerComposeStatus::Success;
            }

            if (composedBufferFromChunks_) {
                composedData = composedBufferFromChunks_;
                composedDataSize = savedCount_;
                return BufferComposerComposeStatus::Success;
            }

            if (chunks_.empty()) {
                return BufferComposerComposeStatus::NoChunkSaved;
            }

            composedBufferFromChunks_ = new unsigned char[savedCount_];
            size_t writtenToComposedBuffer = 0;

            if (inBufferSavedCount_ > 0) {
                if (inBufferSavedCount_ <= STACK_BUFFER_MAX) {
                    std::memcpy(composedBufferFromChunks_, stackBuffer_, inBufferSavedCount_);
                    writtenToComposedBuffer = inBufferSavedCount_;
                }
                else if (inBufferSavedCount_ <= linearBufferMaxSize_) {
                    std::memcpy(composedBufferFromChunks_, linearBuffer_, inBufferSavedCount_);
                    writtenToComposedBuffer = inBufferSavedCount_;
                }
                else {
                    return BufferComposerComposeStatus::LogicErrorWhenComposingFromBuffer;
                }
            }

            for (Chunk& record : chunks_) {
                if (!record.first || writtenToComposedBuffer + record.second > savedCount_) {
                    return BufferComposerComposeStatus::LogicErrorWhenComposingFromChunks;
                }
                std::memcpy(composedBufferFromChunks_ + writtenToComposedBuffer, record.first, record.second);
                writtenToComposedBuffer += record.second;

                delete[]record.first;
                record.first = nullptr;
            }

            if (writtenToComposedBuffer != savedCount_) {
                return BufferComposerComposeStatus::ComposedDataFromChunksDontMatchToSavedCount;
            }

            chunks_.clear();

            composedData = composedBufferFromChunks_;
            composedDataSize = savedCount_;

            return BufferComposerComposeStatus::Success;
        }

        void clear() noexcept
        {
            savedCount_ = 0;
            inBufferSavedCount_ = 0;

            for (Chunk& chunk : chunks_) {
                delete[]chunk.first;
            }

            chunks_.clear();

            delete[]composedBufferFromChunks_;
            composedBufferFromChunks_ = nullptr;
        }

        void cleanup() noexcept
        {
            clear();

            delete[]linearBuffer_;
            linearBuffer_ = nullptr;
            linearBufferAllocatedSize_ = 0;
        }

    private:
        using Chunk = std::pair<unsigned char*, size_t>;

        size_t linearBufferMaxSize_;
        size_t saveBufferMaxCount_;
        size_t savedCount_ = 0;
        unsigned char* linearBuffer_ = nullptr;
        size_t linearBufferAllocatedSize_ = 0;
        size_t inBufferSavedCount_ = 0;
        unsigned char* composedBufferFromChunks_ = nullptr;
        std::list<Chunk> chunks_;
        unsigned char stackBuffer_[STACK_BUFFER_MAX];
    };
}