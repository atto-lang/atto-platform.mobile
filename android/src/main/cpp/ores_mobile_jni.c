#include <jni.h>
#include <stdint.h>
#include <stdlib.h>
#include "ores_mobile_bridge.h"

/* JNI example only, not the Oreslang actor runtime.
 * Keep the small synchronous native copy out of malloc; still copy into
 * caller-owned Java result and zero the temporary stack or heap bytes. */
#define ORES_MOBILE_JNI_STACK_COPY_LIMIT 1024u
static void throw_by_name(JNIEnv *env, const char *name, const char *message) {
    jclass type = (*env)->FindClass(env, name);
    if (type != NULL) {
        (*env)->ThrowNew(env, type, message);
        (*env)->DeleteLocalRef(env, type);
    }
}
/* Own native bytes only for the synchronous JNI call. All owned copies,
 * including failed JNI region reads, are wiped before returning to malloc.
 * The JVM input/output arrays remain owned by Java and are not scrubbed. */
static void clear_and_release(uint8_t *owned, size_t length, uint8_t *stack_copy) {
    if (owned != NULL) {
        (void)ores_mobile_clear_message(owned, length);
        if (owned != stack_copy) free(owned);
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
    uint8_t stack_copy[ORES_MOBILE_JNI_STACK_COPY_LIMIT];
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
    if ((*env)->ExceptionCheck(env)) return NULL;
    if (count < 0 || (size_t)count > ORES_MOBILE_MAX_MESSAGE_BYTES) {
        throw_by_name(env, "java/lang/IllegalArgumentException", "message exceeds 64 KiB");
        return NULL;
    }

    /* Region copying avoids pinning a JVM array across allocation/processing. */
    if (count != 0) {
        if ((size_t)count <= sizeof stack_copy)
            owned = stack_copy;
        else
            owned = (uint8_t *)malloc((size_t)count);
        if (owned == NULL) {
            throw_by_name(env, "java/lang/OutOfMemoryError", "native buffer allocation failed");
            return NULL;
        }
        (*env)->GetByteArrayRegion(env, input, 0, count, (jbyte *)owned);
        if ((*env)->ExceptionCheck(env)) {
            clear_and_release(owned, (size_t)count, stack_copy);
            return NULL;
        }
    }
    status = ores_mobile_copy_message(
        owned, (size_t)count, owned, (size_t)count, &written);
    if (status != ORES_MOBILE_OK) {
        clear_and_release(owned, (size_t)count, stack_copy);
        throw_by_name(env, "java/lang/IllegalStateException", "native ABI copy failed");
        return NULL;
    }
    output = (*env)->NewByteArray(env, (jsize)written);
    if (output != NULL && written != 0)
        (*env)->SetByteArrayRegion(env, output, 0, (jsize)written, (const jbyte *)owned);
    clear_and_release(owned, (size_t)count, stack_copy);
    return output; /* JVM exceptions from allocation or copying propagate. */
}
