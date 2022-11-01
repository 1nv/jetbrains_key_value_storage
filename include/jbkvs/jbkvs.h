#pragma once

namespace jbkvs
{

    class Storage
    {
        int _xxx;
    public:
        Storage(int xxx)
            : _xxx(xxx)
        {
        }

        int getXxx() const
        {
            return _xxx;
        }

        int test();
    };

} // namespace jbkvs
