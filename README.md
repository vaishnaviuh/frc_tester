# FRC (Flat Ribbon Cable) Pinout Tester

ESP32-based tester for 10-conductor flat ribbon cables with 2x5 IDC
connectors. It checks each cable against a configurable expected pinout
(straight-through, crossed, fan-out, or intentionally open pins) and
reports opens and shorts over USB serial. Built on a solderless breadboard.

- [Project README: how it works, BOM, pinout, serial commands](frc_test/FRC_TESTS/README.md)
- [Firmware (Arduino sketch)](frc_test/FRC_TESTS/firmware/frc_continuity_tester/frc_continuity_tester.ino)
