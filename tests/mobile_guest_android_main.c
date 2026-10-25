#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

extern int64_t mobile_guest_step(int64_t value);

int main(void) {
    static const int64_t values[] = {0, 1, 41, 499, 4095};
    size_t i;
    for (i = 0; i < sizeof values / sizeof values[0]; ++i) {
        const int64_t actual = mobile_guest_step(values[i]);
        if (actual != values[i] + 1) {
            fprintf(stderr, "mobile_guest_step failed for %" PRId64 "\n", values[i]);
            return 1;
        }
    }
    puts("mobile real Oreslang LLVM -> Android ARM64 native: PASS");
    return 0;
}
