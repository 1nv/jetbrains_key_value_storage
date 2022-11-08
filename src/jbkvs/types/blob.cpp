#include <jbkvs/types/blob.h>

namespace jbkvs::types
{

    BlobPtr Blob::create(const uint8_t* data, size_t size)
    {
        uint8_t* dataCopy = new (std::nothrow) uint8_t[size];
        if (!dataCopy)
        {
            return BlobPtr();
        }

        memcpy(dataCopy, data, size * sizeof(uint8_t));

        struct MakeSharedEnabledBlob : public Blob
        {
            MakeSharedEnabledBlob(const uint8_t* data, size_t size)
                : Blob(data, size)
            {
            }
        };

        BlobPtr blob = std::make_shared<MakeSharedEnabledBlob>(dataCopy, size);
        return blob;
    }

    Blob::Blob(const uint8_t* data, size_t size)
        : _data(data)
        , _size(size)
    {
    }

    Blob::~Blob()
    {
        delete[] _data;
    }

} // namespace jbkvs::types
