#pragma once

#include <jbkvs/detail/mixins.h>
#include <memory>

namespace jbkvs::types
{

    using BlobPtr = std::shared_ptr<class Blob>;

    class Blob
        : public detail::NonCopyableMixin<Blob>
    {
        const uint8_t* _data;
        const size_t _size;

    public:
        static BlobPtr create(const uint8_t* data, size_t size);

        const uint8_t* data() const { return _data; }
        size_t size() const { return _size; }

    private:
        Blob(const uint8_t* data, size_t size);
        ~Blob();
    };

} // namespace jbkvs::types
