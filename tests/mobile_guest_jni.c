#include <jni.h>
#include <stdint.h>

extern int64_t mobile_guest_step(int64_t value);

JNIEXPORT jlong JNICALL
Java_dev_oreslang_mobile_OresMobileGuestNative_step(
    JNIEnv *env, jobject self, jlong value) {
    (void)env;
    (void)self;
    return (jlong)mobile_guest_step((int64_t)value);
}
