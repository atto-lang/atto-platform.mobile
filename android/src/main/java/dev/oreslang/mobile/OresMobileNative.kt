package dev.oreslang.mobile

/**
 * JNI smoke bridge, NOT the Oreslang runtime.
 * An app must package liboresmobile_jni.so for the device ABI.
 */
object OresMobileNative {
    init { System.loadLibrary("oresmobile_jni") }

    external fun abiVersion(): Int

    /** Returns a copied byte array. Messages >64 KiB are rejected. */
    external fun copyMessage(message: ByteArray): ByteArray
}
