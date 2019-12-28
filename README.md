# promiscuous

This is a WiFi side channel protocol based on some [reverse engineering of the ESP-Touch protocol](https://blog.csdn.net/flyingcys/article/details/54670688).

The ESP-Touch protocol consists of a `GuideCode` (synchronisation), some `DatumData` (checksums) and the `Data` (payload).
This implementation omits `DatumData` for simplicity reasons and is not meant to be compatible with the original protocol.
