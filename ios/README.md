# iOS target

AttoLang guest: LLVM AOT -> signed bundled arm64 machine code -> C ABI -> Swift/UIKit host adapter. Apple APIs are used through audited Objective-C/Swift wrappers, not a JVM. Actor scheduling is distinct from UIKit's main actor. No guest-owned UIKit objects, no downloaded executable feature code, no general JIT. 

Current AttoXP host and simulator evidence: https://github.com/atto-xp/atto-xp-ios . No physical-device or signing claim. 
