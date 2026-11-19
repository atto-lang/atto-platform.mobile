# iOS runtime

Own bundled AOT guest loading, startup/shutdown and C ABI linking; retain/release native resources only on their documented threads. Current AttoXP MVP guest updates synchronously on the UI thread: moving them to background actors is a future implementation milestone, NOT accomplished by these contracts.
