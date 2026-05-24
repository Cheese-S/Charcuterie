#include "core/IMacro.h"
namespace mk::asset
{
class Uuid
{
public:
    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Uuid);

    static Uuid generate();

    bool operator==(const Uuid& other);

private:
};
} // namespace mk::asset
