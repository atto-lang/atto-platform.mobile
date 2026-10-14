# Android JNI reference

This JNI adapter invokes the **portable C bridge** to copy bytes across the Kotlin/native boundary. It neither executes `.ores` source nor instantiates Oreslang actors.

Prerequisite: Android NDK with CMake toolchain and an installed Android SDK.

```sh
export ANDROID_NDK_HOME=/path/to/android-ndk
cmake -S android -B build-android \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=24
cmake --build build-android
```

Package `liboresmobile_jni.so` into your Android application's `jniLibs/arm64-v8a/` directory and copy `src/main/java/dev/oreslang/mobile/OresMobileNative.kt` into your Kotlin app. Call `OresMobileNative.abiVersion()` or `copyMessage(...)`.

**Not verified on Android devices yet.** Real native-AOT compiler integration, ART lifecycle, coroutine/actor bridge and instrumentation tests are tracked as future work.
