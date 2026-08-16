# test_overflow

A small standalone utility for exercising the buffer instrumentation
added in this fork (`overflow_count`, `high_watermark`,
`ringbuffer_capacity`, `reset_stats`, and `SOAPY_SDR_OVERFLOW`
signaling). It opens the device with a deliberately small ring buffer,
stalls without reading for 2 seconds (guaranteed to overflow a small
buffer at any real sample rate), then drains and reports what happened.

It's meant as a quick sanity check after building this fork, and as a
reference for how to read the new settings from your own code.

## Build

Build directly against the same SoapySDR install this fork's driver
uses (adjust paths if your install prefix differs from `/usr/local`):

```bash
g++ -std=c++17 test_overflow.cpp -o test_overflow \
    -I/usr/local/include -L/usr/local/lib -lSoapySDR
```

## Run

```bash
LD_LIBRARY_PATH=/usr/local/lib ./test_overflow
```

Edit the `SERIAL` and `BUFSIZE` constants at the top of `main()` in
`test_overflow.cpp` first if you want to target a specific device (or
leave `SERIAL` empty to use whichever Airspy is found first) or use a
different buffer size than the 512KiB default.

> **Note on Python bindings:** if you have a second, separately
> packaged SoapySDR install (e.g. from your distro's `python3-soapysdr`
> package), it may link against a different `libSoapySDR` than the one
> this fork was built/installed into, and silently load a different,
> unpatched `airspy` module instead. This C++ tool sidesteps that
> entirely by linking directly against the same install your driver
> build uses. If you want to test via Python instead, make sure
> `SOAPY_SDR_PLUGIN_PATH` and `LD_LIBRARY_PATH` both point at your
> from-source install explicitly.

## Example output

Real output from an Airspy R2, 6 Msps, `bufsize=524288` (512KiB):

```
Opening device...
ringbuffer_capacity: 524288
After reset -> overflow_count: 0  high_watermark: 0
Sample rate: 6 Msps
[INFO] Using format CS16.
Not reading at all for 2 seconds -- guaranteed to overflow a 512KB buffer at any real sample rate...
[INFO] SoapyAirspy::rx_callback: ringbuffer write timeout, dropped 262144 bytes (overflow #1)
[INFO] SoapyAirspy::rx_callback: ringbuffer write timeout, dropped 262144 bytes (overflow #2)
[INFO] SoapyAirspy::rx_callback: ringbuffer write timeout, dropped 262144 bytes (overflow #3)
[INFO] SoapyAirspy::rx_callback: ringbuffer write timeout, dropped 262144 bytes (overflow #4)
[INFO] SoapyAirspy::rx_callback: ringbuffer write timeout, dropped 262144 bytes (overflow #5)
[INFO] SoapyAirspy::rx_callback: ringbuffer write timeout, dropped 262144 bytes (overflow #6)
[INFO] SoapyAirspy::rx_callback: ringbuffer write timeout, dropped 262144 bytes (overflow #7)
overflow_count after the stall (before any read): 7
  [drain read 0] ret=-4  <- SOAPY_SDR_OVERFLOW
  [drain read 1] ret=65536
  [drain read 2] ret=65536
  [drain read 3] ret=65536
  [drain read 4] ret=65536
  [drain read 5] ret=65536
  [drain read 6] ret=65536
  [drain read 7] ret=65536
  [drain read 8] ret=65536
  [drain read 9] ret=65536
=== Results ===
overflow_count: 7
high_watermark: 524288 bytes (capacity: 524288)
SOAPY_SDR_OVERFLOW seen on readStream(): true
```

What to look for:

- **`overflow_count`** climbs by one for each dropped block during the
  stall (7 here, matching the 7 log lines) — confirms drops are
  actually being counted.
- **`ret=-4` on the first drain read** is `SOAPY_SDR_OVERFLOW`
  (`SoapySDR/Errors.hpp`) — confirms the flag set during the stall is
  correctly reported on the very next `readStream()` call, then
  subsequent reads return to normal (`ret=65536`, the requested sample
  count).
- **`high_watermark: 524288`** equals `ringbuffer_capacity` exactly —
  confirms the buffer genuinely filled to capacity (which is why it
  overflowed), read correctly from the producer thread.

If `overflow_count` stays 0 during the stall, or `high_watermark`
stays 0 despite overflows occurring, something is wrong -- most likely
you're linking against a different, unpatched SoapySDR/airspy module
than you think (see the Python bindings note above; the same class of
mismatch can happen with C++ builds too if you have multiple SoapySDR
installs).
