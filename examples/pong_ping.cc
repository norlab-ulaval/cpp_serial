// pong_ping.cc: Continuously reads and replies with a message.
// Usage: ./pong_ping <device> <baudrate> <message> --interval-ms <interval> [--gap-ms <gap>]
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
    int gap_ms = 2;
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
        ser.setTimeout(0, 1, 0, 1, 0);
        ser.open();
    } catch (const std::exception& e) {
        std::cerr << "Error opening serial port: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "[pong_ping] Started on " << port << " at " << baud << " baud.\n";
    // Clear any stale input
    try { ser.flushInput(); } catch (...) {}

    // Track time of last send to compute delay upon next receive
    bool has_last_send = false;
    auto last_send = std::chrono::steady_clock::now();

    // Stats and graceful shutdown
    uint64_t count = 0;
    long double sum_ms = 0.0L;
    static std::atomic<bool> running{true};
    std::signal(SIGINT, [](int){ running = false; });
    // Idle-gap framing reader
    auto wait_for_frame = [&ser](int gap_ms) -> std::string {
        std::string buf;
        // Wait until at least one byte arrives
        for (;;) {
            size_t n = 0;
            try { n = ser.available(); } catch (...) { n = 0; }
            if (n > 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
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
                    std::cerr << "[pong_ping] Error reading: " << e.what() << std::endl;
                }
            } else {
                auto now = std::chrono::steady_clock::now();
                auto idle = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_rx).count();
                if (!buf.empty() && idle >= gap_ms) return buf;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    };

    while (running.load()) {
        // Read incoming data as a frame based on inter-byte gap
    std::string result = wait_for_frame(gap_ms);
        if (!result.empty()) {
            if (has_last_send) {
                auto now = std::chrono::steady_clock::now();
                auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_send).count();
                ++count;
                sum_ms += static_cast<long double>(dt_ms);
            }
            std::cout << "[pong_ping] Received: " << result << std::endl;
            // Send reply
            ser.write(message);
            std::cout << "[pong_ping] Sent: " << message << std::endl;
            has_last_send = true;
            last_send = std::chrono::steady_clock::now();
        }
        // Optional pacing to avoid flooding if desired
        if (interval_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }
    }

    if (count > 0) {
        long double avg = sum_ms / static_cast<long double>(count);
        std::cout << "[pong_ping] Summary: iterations=" << count
                  << ", average send->receive time=" << static_cast<double>(avg) << " ms" << std::endl;
    } else {
        std::cout << "[pong_ping] Summary: no iterations completed." << std::endl;
    }
    return 0;
}
