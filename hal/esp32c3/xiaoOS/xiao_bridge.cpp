extern "C" {
#include "../../../kernel/core/xiao_core.c"
#include "../../../boot/common/image.c"

#if __has_include("../../../apps/hello.c")
#include "../../../apps/hello.c"
#endif

#if __has_include("../../../apps/serial_hello.c")
#include "../../../apps/serial_hello.c"
#endif
}
