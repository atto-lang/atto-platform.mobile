package dev.oreslang.mobile;

/** JNI bootstrap contract only; does not expose the Oreslang actor runtime. */
public final class OresMobileNative {
    static { System.loadLibrary("ores_mobile_jni"); }

    public native int abiVersion();
    public native byte[] copyMessage(byte[] input);
}
