package dev.oreslang.mobile;

public final class OresMobileGuestNativeTest {
    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }

    public static void main(String[] args) {
        OresMobileGuestNative guest = new OresMobileGuestNative();
        long[] values = {0L, 1L, 41L, 499L, 4095L};
        for (long value : values) {
            require(guest.step(value) == value + 1L,
                    "unexpected guest step for " + value);
        }
        System.out.println("mobile real Oreslang LLVM -> JNI -> Java: PASS");
    }
}
