#pragma once

namespace jbkvs::detail
{

    template <typename T>
    class NonCopyableMixin
    {
    protected:
        NonCopyableMixin() = default;
        ~NonCopyableMixin() = default;

        NonCopyableMixin(const NonCopyableMixin& other) = delete;
        T& operator=(const T& other) = delete;
    };

} // namespace jbkvs::detail
