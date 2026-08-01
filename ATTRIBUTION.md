# Attribution

## Protocol

The Omron "legacy" vendor GATT protocol — the four 16-byte transmit and receive
channels, the packet framing with its XOR checksum, the 16-byte unlock key
exchange, and the EEPROM memory map and record layout of the HEM-7600T — was
reconstructed from community reverse-engineering work:

* [userx14/omblepy](https://github.com/userx14/omblepy) — a Python tool for
  reading Omron monitors over BLE. Its device-specific description of the
  HEM-7600T (memory addresses, 14-byte record layout, clock block) is what this
  implementation follows.
* [LazyT/ubpm](https://codeberg.org/LazyT/ubpm) — an independent Qt/C++
  implementation covering the same family of devices. Used to cross-check every
  constant.

No source code from either project was copied; both are independent
implementations of the same observed wire protocol.

Every UUID and constant used here was additionally verified against a physical
EVOLV (HEM-7600T) by reading its GATT database.

## Icon

Original artwork (a heart with a lightning-bolt cut-out), shared with the
sibling app that reads a wired monitor.

## Trademarks

"Omron" and "EVOLV" are trademarks of their respective owner. This project is
not affiliated with, endorsed by, or supported by Omron.
