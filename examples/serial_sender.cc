/* Simple serial sender example.
 * Usage:
 *   serial_sender <port> <baud> <message> [--interval-ms N]
 * Example:
 *   serial_sender /dev/ttyUSB0 115200 "Hello world" --interval-ms 500
 * Sends the message followed by a newline at the given interval until Ctrl-C.
 */

#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

#include "serial/serial.h"

static bool keep_running = true;

void signal_handler(int) { keep_running = false; }

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: serial_sender <port> <baud> <message> [--interval-ms N]\n";
    return 1;
  }
  std::string port = argv[1];
  unsigned long baud = std::stoul(argv[2]);
  std::string message = argv[3];
  unsigned long interval_ms = 1000;
  for (int i = 4; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--interval-ms" && i + 1 < argc) {
      interval_ms = std::stoul(argv[++i]);
    }
  }

  serial::Serial ser(port, baud, serial::Timeout::simpleTimeout(1000));
  if (!ser.isOpen()) {
    std::cerr << "Failed to open port: " << port << "\n";
    return 2;
  }
  std::cout << "Opened " << port << " at " << baud << " baud. Sending every "
            << interval_ms << " ms. Press Ctrl-C to stop." << std::endl;

  std::signal(SIGINT, signal_handler);

  size_t counter = 0;
  while (keep_running) {
    std::string payload = message + "\n"; // newline helps receiver readline()
    size_t written = ser.write(reinterpret_cast<const uint8_t*>(payload.data()), payload.size());
    std::cout << "Sent (#" << counter++ << "): '" << message << "' (" << written << " bytes)" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
  }

  std::cout << "Stopping sender." << std::endl;
  return 0;
}
