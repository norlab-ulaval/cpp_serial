// ping_pong.cc: Sends a message, then waits for an incoming message before sending again.
// The interval is applied as a delay BEFORE each send (including after responses).
// Usage: ./ping_pong <device> <baudrate> <message> --interval-ms <interval> [--gap-ms <gap>]
#include <serial/serial.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <device> <baudrate> <message> --interval-ms <interval> [--gap-ms <gap>]\n";
        return 1;
    }
    std::string port = argv[1];
    unsigned long baud = std::stoul(argv[2]);
    std::string message = argv[3];
    int interval_ms = 1000;
    int gap_ms = 2; // idle gap to frame incoming message
    for (int i = 4; i < argc; ++i) {
        if (std::string(argv[i]) == "--interval-ms" && i + 1 < argc) {
            interval_ms = std::stoi(argv[i + 1]);
        } else if (std::string(argv[i]) == "--gap-ms" && i + 1 < argc) {
            gap_ms = std::stoi(argv[i + 1]);
        }
    }
    serial::Serial ser;
    try {
        ser.setPort(port);
        ser.setBaudrate(baud);
        // Minimize IO timeouts for responsiveness
        ser.setTimeout(0, 1, 0, 1, 0);
        ser.open();
    } catch (const std::exception& e) {
        std::cerr << "Error opening serial port: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "[ping_pong] Started on " << port << " at " << baud << " baud." << std::endl;
    // Clear any stale data before starting the ping/pong
    try { ser.flushInput(); } catch (...) {}

    // Helper: wait for a frame based on an inter-byte idle gap (e.g., 10ms)
    auto wait_for_frame = [&ser](int gap_ms) -> std::string {
        std::string buf;
        // Wait until at least one byte arrives
        for (;;) {
            size_t n = 0;
            try { n = ser.available(); } catch (...) { n = 0; }
            if (n > 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // Read burst and wait for quiet period
        auto last_rx = std::chrono::steady_clock::now();
        for (;;) {
            size_t n = 0;
            try { n = ser.available(); } catch (...) { n = 0; }
            if (n > 0) {
                try {
                    std::string chunk = ser.read(n);
                    buf += chunk;
                    last_rx = std::chrono::steady_clock::now();
                } catch (const std::exception& e) {
                    std::cerr << "[ping_pong] Error reading: " << e.what() << std::endl;
                }
            } else {
                auto now = std::chrono::steady_clock::now();
                auto idle = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_rx).count();
                if (!buf.empty() && idle >= gap_ms) {
                    return buf;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    };

    // Send the first message after the configured delay
    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    auto last_send = std::chrono::steady_clock::now();
    try {
    ser.write(message);
        std::cout << "[ping_pong] Sent: " << message << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ping_pong] Error writing: " << e.what() << std::endl;
    }

    // Stats
    uint64_t count = 0;
    long double sum_ms = 0.0L;

    // Graceful shutdown to print average at the end
    static std::atomic<bool> running{true};
    std::signal(SIGINT, [](int){ running = false; });

    // Thereafter, wait for any incoming message before sending again
    while (running.load()) {
        std::string incoming = wait_for_frame(gap_ms);
        auto now = std::chrono::steady_clock::now();
        auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_send).count();
        // accumulate only; no per-iteration delay print
        ++count;
        sum_ms += static_cast<long double>(dt_ms);
        std::cout << "[ping_pong] Received: " << incoming << std::endl;

        // Delay before replying again
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        try {
            ser.write(message);
            std::cout << "[ping_pong] Sent: " << message << std::endl;
            last_send = std::chrono::steady_clock::now();
        } catch (const std::exception& e) {
            std::cerr << "[ping_pong] Error writing: " << e.what() << std::endl;
        }
    }

    // Summary
    if (count > 0) {
        long double avg = sum_ms / static_cast<long double>(count);
        std::cout << "[ping_pong] Summary: iterations=" << count
                  << ", average send->receive time=" << static_cast<double>(avg) << " ms" << std::endl;
    } else {
        std::cout << "[ping_pong] Summary: no iterations completed." << std::endl;
    }
    return 0;
}
