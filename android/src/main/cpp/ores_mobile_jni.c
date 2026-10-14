#include <jni.h>
#include <stdint.h>
#include <stdlib.h>
#include "ores_mobile_bridge.h"

static void throw_by_name(JNIEnv *env, const char *name, const char *message) {
    jclass type = (*env)->FindClass(env, name);
    if (type != NULL) {
        (*env)->ThrowNew(env, type, message);
        (*env)->DeleteLocalRef(env, type);
    }
}

JNIEXPORT jint JNICALL
Java_dev_oreslang_mobile_OresMobileNative_abiVersion(JNIEnv *env, jobject self) {
    (void)env; (void)self;
    return (jint)ores_mobile_abi_version();
}

JNIEXPORT jbyteArray JNICALL
Java_dev_oreslang_mobile_OresMobileNative_copyMessage(
    JNIEnv *env, jobject self, jbyteArray input) {
    jsize count;
    jbyte *borrowed = NULL;
    uint8_t *owned = NULL;
    size_t written = 0;
    ores_mobile_status status;
    jbyteArray output = NULL;
    (void)self;
    if (input == NULL) {
        throw_by_name(env, "java/lang/IllegalArgumentException", "message must not be null");
        return NULL;
    }
    count = (*env)->GetArrayLength(env, input);
    if ((size_t)count > ORES_MOBILE_MAX_MESSAGE_BYTES) {
        throw_by_name(env, "java/lang/IllegalArgumentException", "message exceeds 64 KiB");
        return NULL;
    }
    if (count != 0) {
        borrowed = (*env)->GetByteArrayElements(env, input, NULL);
        if (borrowed == NULL) return NULL; /* pending JVM exception */
        owned = (uint8_t *)malloc((size_t)count);
        if (owned == NULL) {
            (*env)->ReleaseByteArrayElements(env, input, borrowed, JNI_ABORT);
            throw_by_name(env, "java/lang/OutOfMemoryError", "mobile message buffer allocation failed");
            return NULL;
        }
    }
    status = ores_mobile_copy_message(
        (const uint8_t *)borrowed, (size_t)count, owned, (size_t)count, &written);
    if (borrowed != NULL) (*env)->ReleaseByteArrayElements(env, input, borrowed, JNI_ABORT);
    if (status != ORES_MOBILE_OK) {
        free(owned);
        throw_by_name(env, "java/lang/IllegalStateException", "native ABI copy failed");
        return NULL;
    }
    output = (*env)->NewByteArray(env, (jsize)written);
    if (output != NULL && written != 0)
        (*env)->SetByteArrayRegion(env, output, 0, (jsize)written, (const jbyte *)owned);
    free(owned);
    return output;
}
