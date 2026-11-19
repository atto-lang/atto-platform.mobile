# Android target

Primary current path: LLVM AOT -> native C runtime -> JNI -> Android Views/Kotlin. The optional Java/ART path is a separate feasibility and compatibility project inside atto-source.java; desktop GraalVM/Truffle runtime cannot simply be dropped into ART. Main Looper-only mutations, lifecycle revocation, and native ownership checks must match iOS contract. See https://github.com/atto-xp/atto-xp-android .
