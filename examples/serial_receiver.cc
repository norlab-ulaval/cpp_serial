/* Simple serial receiver example.
 * Usage:
 *   serial_receiver <port> <baud> [--lines|--raw]
 * Example:
 *   serial_receiver /dev/ttyUSB0 115200 --lines
 * Continuously reads from the port and prints to stdout.
 */

#include <iostream>
#include <string>
#include <vector>

#include "serial/serial.h"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: serial_receiver <port> <baud> [--lines|--raw]\n";
    return 1;
  }
  std::string port = argv[1];
  unsigned long baud = std::stoul(argv[2]);
  bool as_lines = true; // default to line mode

  for (int i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--raw") as_lines = false;
    if (arg == "--lines") as_lines = true;
  }

  serial::Serial ser(port, baud, serial::Timeout::simpleTimeout(500));
  if (!ser.isOpen()) {
    std::cerr << "Failed to open port: " << port << "\n";
    return 2;
  }
  std::cerr << "Opened " << port << " at " << baud << " baud. Press Ctrl-C to stop." << std::endl;

  if (as_lines) {
    while (true) {
      std::string line = ser.readline(1024, "\n");
      if (!line.empty()) {
        std::cout << line; // already includes '\n'
        std::cout.flush();
      }
    }
  } else {
    std::vector<uint8_t> buf(4096);
    while (true) {
      size_t n = ser.read(buf, buf.size());
      if (n > 0) {
        std::cout.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(n));
        std::cout.flush();
      } else {
        // Avoid busy loop on timeouts
        ser.waitByteTimes(1);
      }
    }
  }

  return 0;
}
