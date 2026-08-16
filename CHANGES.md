# Changes from ast/SoapyAirspy (v2, commit 5d60583)

## Fixed: bias tee silently not taking effect (main fix)

`activateStream()` now calls `airspy_set_rf_bias(dev_, rfBias_)` and
`airspy_set_packing(dev_, bitPack_)` immediately before `airspy_start_rx()`,
every time a stream starts — matching the call ordering used by SDR++'s
native Airspy driver.

Previously these were only applied once, in the constructor, before the
caller had set a sample rate or activated the stream — a call ordering
that could result in the setting being lost by the time streaming
actually began.

Files: `src/Streaming.cpp`

## Fixed: device-args string (`biastee=`, `bitpack=`, `gains=`) not applied at construction

Re-enabled the loop that maps device-args keys to `writeSetting()` calls
at construction time, matching upstream `pothosware/SoapyAirspy`. This
loop was present in commented-out form with a note about virtual calls
in constructors; safe here since `SoapyAirspy` has no subclasses.

Files: `src/Settings.cpp`

## Fixed: uninitialized members

`serial_`, `linearityGain_`, `sensitivityGain_`, `lnaGain_`, `mixerGain_`,
`vgaGain_`, and `agcMode_` are now initialized in the constructor's
member-init list.

Files: `src/Settings.cpp`, `src/SoapyAirspy.hpp`

## Added: SOAPY_SDR_OVERFLOW signaling

`rx_callback()` now tracks a dropped-sample event; `readStream()` reports
`SOAPY_SDR_OVERFLOW` on the next call after a drop, matching upstream
`pothosware/SoapyAirspy` behavior. Previously drops were logged at INFO
level only, with no way for callers to detect them.

Files: `src/Streaming.cpp`, `src/SoapyAirspy.hpp`

## Added: configurable ring buffer size

Ring buffer capacity is now set via the `bufsize` device arg (bytes,
must be a power of two), defaulting to the previous hardcoded 4MiB
(`1 << 22`) if unset or invalid.

Files: `src/Settings.cpp`, `src/SoapyAirspy.hpp`

## Added: runtime buffer stats

New read-only settings, readable via `SoapySDR::Device::readSetting()`
or `SoapySDRUtil --probe`:

- `overflow_count` — dropped-sample-block count since stream start or last reset
- `high_watermark` — largest queue depth observed, in bytes
- `ringbuffer_capacity` — current capacity, in bytes

New writable setting `reset_stats` zeroes the two counters above.

Files: `src/Settings.cpp`, `src/Streaming.cpp`, `src/SoapyAirspy.hpp`
