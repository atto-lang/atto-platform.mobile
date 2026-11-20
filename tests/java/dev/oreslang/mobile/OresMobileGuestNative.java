package dev.oreslang.mobile;

/** Test-only JNI bridge to an LLVM-compiled Oreslang scalar function. */
public final class OresMobileGuestNative {
    static { System.loadLibrary("ores_mobile_guest_jni"); }

    public native long step(long value);
}
