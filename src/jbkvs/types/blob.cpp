#include <jbkvs/types/blob.h>

#include <string.h>

namespace jbkvs::types
{

    BlobPtr Blob::create(const uint8_t* data, size_t size)
    {
        std::unique_ptr<uint8_t[]> dataCopy(new uint8_t[size]); // TODO: C++20: replace with std::make_unique_for_overwrite().
        memcpy(dataCopy.get(), data, size * sizeof(uint8_t));

        BlobPtr blob = create(std::move(dataCopy), size);
        return blob;
    }

    BlobPtr Blob::create(std::unique_ptr<const uint8_t[]>&& data, size_t size)
    {
        struct MakeSharedEnabledBlob : public Blob
        {
            MakeSharedEnabledBlob(std::unique_ptr<const uint8_t[]>&& data, size_t size)
                : Blob(std::move(data), size)
            {
            }
        };

        BlobPtr blob = std::make_shared<MakeSharedEnabledBlob>(std::move(data), size);
        return blob;
    }

    Blob::Blob(std::unique_ptr<const uint8_t[]>&& data, size_t size) noexcept
        : _data(std::move(data))
        , _size(size)
    {
    }

    Blob::~Blob() noexcept
    {
    }

} // namespace jbkvs::types
