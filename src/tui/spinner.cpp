/**
 * @file spinner.cpp
 * @brief RAII spinner animation (extracted from tui.h).
 * @see docs/adr/adr-066-solid-refactoring.md
 */

#include "tui/spinner.h"

#include <chrono>

#include "tui/tui.h"

Spinner::Spinner(std::ostream& out, bool active, std::vector<const char*> msgs) : out_(out), running_(active), msgs_(std::move(msgs)) {
  // Use default messages if none provided
  if (msgs_.empty()) {
    msgs_ = tui::default_messages();
  }
  if (running_) {
    thread_ = std::thread([this] { run(); });
  }
}

Spinner::~Spinner() { stop(); }

/// Stop the spinner, join the thread, and clear the line.
/// Safe to call multiple times (atomic exchange prevents double-join).
void Spinner::stop() {
  bool was_running = running_.exchange(false);
  if (thread_.joinable()) {
    thread_.join();
  }
  if (was_running) {
    out_ << "\r\033[K" << std::flush;
  }
}

/// Animation loop — runs on a background thread.
/// Cycles through braille frames and rotates messages every 30 frames (~2.4s).
/// Uses \r + \033[K to overwrite the line without scrolling.
void Spinner::run() {
  const char* frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
  int i = 0;
  int msg_idx = 0;
  while (running_) {
    out_ << "\r\033[2m" << frames[i % 10] << " " << msgs_[msg_idx % msgs_.size()] << "\033[0m\033[K" << std::flush;
    i++;
    if (i % 30 == 0) {
      msg_idx++;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
  }
}
