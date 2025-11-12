// pong_ping.cc: Continuously reads and replies with a message.
// Usage: ./pong_ping <device> <baudrate> <message> --interval-ms <interval>
#include <serial/serial.h>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <device> <baudrate> <message> --interval-ms <interval>\n";
        return 1;
    }
    std::string port = argv[1];
    unsigned long baud = std::stoul(argv[2]);
    std::string message = argv[3];
    int interval_ms = 1000;
    for (int i = 4; i < argc; ++i) {
        if (std::string(argv[i]) == "--interval-ms" && i + 1 < argc) {
            interval_ms = std::stoi(argv[i + 1]);
        }
    }
    serial::Serial ser;
    try {
        ser.setPort(port);
        ser.setBaudrate(baud);
        ser.open();
    } catch (const std::exception& e) {
        std::cerr << "Error opening serial port: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "[pong_ping] Started on " << port << " at " << baud << " baud.\n";
    while (true) {
        // Read incoming data
        std::string result = ser.read(ser.available());
        if (!result.empty()) {
            std::cout << "[pong_ping] Received: " << result << std::endl;
            // Send reply
            ser.write(message);
            std::cout << "[pong_ping] Sent: " << message << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
    return 0;
}
