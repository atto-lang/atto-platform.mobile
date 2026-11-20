package dev.oreslang.mobile;

import java.util.Arrays;

/** Executes the real JNI boundary, not a mocked Java implementation. */
public final class OresMobileNativeTest {
    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }

    private static void roundTrip(OresMobileNative nativeBridge, int length) {
        byte[] original = new byte[length];
        for (int i = 0; i < length; i++) original[i] = (byte) (i * 37 + 11);
        byte[] result = nativeBridge.copyMessage(original);
        require(result != null, "null JNI result for length " + length);
        require(result != original, "native bridge returned original array for " + length);
        require(Arrays.equals(original, result), "byte mismatch for length " + length);
        if (length > 0) {
            byte previous = result[0];
            original[0] ^= 0x5a;
            require(result[0] == previous, "native result aliases input for " + length);
            result[length - 1] ^= 0x33;
            require(original[length - 1] != result[length - 1] || length == 1,
                    "result mutation unexpectedly changed input for " + length);
        }
    }

    private static void rejected(OresMobileNative nativeBridge, byte[] payload) {
        try {
            nativeBridge.copyMessage(payload);
            throw new AssertionError("invalid payload accepted");
        } catch (IllegalArgumentException expected) {
            // JNI input admission must reject before allocating native buffers.
        }
    }

    public static void main(String[] args) {
        OresMobileNative nativeBridge = new OresMobileNative();
        require(nativeBridge.abiVersion() == 1, "wrong native ABI version");
        for (int n : new int[]{0, 1, 2, 7, 8, 255, 256, 1023, 1024, 1025, 4096, 65535, 65536}) {
            roundTrip(nativeBridge, n);
        }
        for (int n = 0; n < 128; n++) roundTrip(nativeBridge, n * 31 % 4097);
        rejected(nativeBridge, null);
        rejected(nativeBridge, new byte[65537]);
        roundTrip(nativeBridge, 65536); // rejected calls must not poison later JNI calls
        System.out.println("mobile real JNI boundary: PASS");
    }
}
