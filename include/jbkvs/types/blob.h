#pragma once

#include <memory>

#include <jbkvs/detail/mixins.h>

namespace jbkvs::types
{

    using BlobPtr = std::shared_ptr<class Blob>;

    class Blob
        : public detail::NonCopyableMixin<Blob>
    {
        std::unique_ptr<const uint8_t[]> _data;
        const size_t _size;

    public:
        static BlobPtr create(const uint8_t* data, size_t size);
        static BlobPtr create(std::unique_ptr<const uint8_t[]>&& data, size_t size);

        const uint8_t* data() const noexcept { return _data.get(); }
        size_t size() const noexcept { return _size; }

    private:
        Blob(std::unique_ptr<const uint8_t[]>&& data, size_t size) noexcept;
        ~Blob() noexcept;
    };

} // namespace jbkvs::types
