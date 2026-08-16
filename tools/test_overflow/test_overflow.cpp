// Test program for the SoapyAirspy buffer instrumentation fork.
// Build against the SAME /usr/local SoapySDR install SoapySDRUtil uses,
// avoiding the apt-package ABI mismatch that breaks Python here.
//
// Build:
//   g++ -std=c++17 test_overflow.cpp -o test_overflow \
//       -I/usr/local/include -L/usr/local/lib -lSoapySDR
//
// Run:
//   LD_LIBRARY_PATH=/usr/local/lib ./test_overflow

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Types.hpp>
#include <SoapySDR/Formats.hpp>
#include <iostream>
#include <vector>
#include <complex>
#include <chrono>
#include <thread>
#include <algorithm>

int main() {
    // Edit these for your setup:
    const std::string SERIAL = "744c60c8218e3e4f"; // "" = first device found
    const std::string BUFSIZE = "524288";           // 512KiB, deliberately small

    SoapySDR::Kwargs args;
    args["driver"] = "airspy";
    if (!SERIAL.empty()) args["serial"] = SERIAL;
    args["bufsize"] = BUFSIZE;

    std::cout << "Opening device...\n";
    SoapySDR::Device *dev = SoapySDR::Device::make(args);

    std::cout << "ringbuffer_capacity: " << dev->readSetting("ringbuffer_capacity") << "\n";

    dev->writeSetting("reset_stats", "1");
    std::cout << "After reset -> overflow_count: " << dev->readSetting("overflow_count")
              << "  high_watermark: " << dev->readSetting("high_watermark") << "\n";

    auto rates = dev->listSampleRates(SOAPY_SDR_RX, 0);
    double rate = *std::max_element(rates.begin(), rates.end());
    dev->setSampleRate(SOAPY_SDR_RX, 0, rate);
    dev->setFrequency(SOAPY_SDR_RX, 0, 100e6);
    std::cout << "Sample rate: " << rate / 1e6 << " Msps\n";

    SoapySDR::Stream *stream = dev->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CS16);
    dev->activateStream(stream);

    std::vector<std::complex<int16_t>> buf(65536);
    void *buffs[] = { buf.data() };
    bool overflowSeen = false;
    int reads = 0;

    std::cout << "Not reading at all for 2 seconds -- guaranteed to overflow "
                 "a 512KB buffer at any real sample rate...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "overflow_count after the stall (before any read): "
              << dev->readSetting("overflow_count") << "\n";

    // Now drain with reads, checking each one for SOAPY_SDR_OVERFLOW.
    // Multiple drops likely queued up during the stall, so we expect to
    // see the flag on one of the first several reads.
    for (int i = 0; i < 10; i++) {
        int flags = 0;
        long long timeNs = 0;
        int ret = dev->readStream(stream, buffs, buf.size(), flags, timeNs, 200000);
        reads++;
        std::cout << "  [drain read " << i << "] ret=" << ret
                  << (ret == SOAPY_SDR_OVERFLOW ? "  <- SOAPY_SDR_OVERFLOW" : "") << "\n";
        if (ret == SOAPY_SDR_OVERFLOW) {
            overflowSeen = true;
        }
    }

    dev->deactivateStream(stream);
    dev->closeStream(stream);

    std::cout << "\n=== Results ===\n";
    std::cout << "overflow_count: " << dev->readSetting("overflow_count") << "\n";
    std::cout << "high_watermark: " << dev->readSetting("high_watermark")
              << " bytes (capacity: " << dev->readSetting("ringbuffer_capacity") << ")\n";
    std::cout << "SOAPY_SDR_OVERFLOW seen on readStream(): " << (overflowSeen ? "true" : "false") << "\n";

    SoapySDR::Device::unmake(dev);
    return 0;
}
