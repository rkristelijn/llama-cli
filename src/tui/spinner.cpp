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
/// Picks a random 1-char spinner style at start, then cycles frames.
/// Source: ~/git/hub/personal/scripts/lib/progress.sh (sindresorhus/cli-spinners + custom)
/// Rotates messages every 30 frames (~2.4s).
void Spinner::run() {
  // 1-char spinner styles (consistent width per style)
  // Each style is a vector of UTF-8 frames, randomly selected per instance.
  // Styles sourced from sindresorhus/cli-spinners + custom kiro variants.
  static const std::vector<std::vector<const char*>> styles = {
      {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"},  // braille
      {"⣾", "⣽", "⣻", "⢿", "⡿", "⣟", "⣯", "⣷"},            // dots
      {"◐", "◓", "◑", "◒"},                                // circle
      {"⠁", "⠂", "⠄", "⠂"},                                // bounce
      {"◑", "◒", "◐", "◓"},                                // moon
      {"◜", "◠", "◝", "◞", "◡", "◟"},                      // arc
      {"◢", "◣", "◤", "◥"},                                // triangle
      {"◰", "◳", "◲", "◱"},                                // square
      {"▫", "▪"},                                          // toggle
      {"▓", "▒", "░"},                                     // noise
      {"✶", "✸", "✹", "✺", "✹", "✷"},                      // star
      {"☱", "☲", "☴"},                                     // hamburger
      {"▖", "▘", "▝", "▗"},                                // box
      {"▌", "▀", "▐", "▄"},                                // box2
      {"◴", "◷", "◶", "◵"},                                // clock
      {"←", "↖", "↑", "↗", "→", "↘", "↓", "↙"},            // arrow
      {"┤", "┘", "┴", "└", "├", "┌", "┬", "┐"},            // pipe
      {"-", "=", "≡"},                                     // layer
      {"/", "-", "\\", "|"},                               // classic
      {"○", "◔", "◑", "◕", "●"},                           // kiro (fill)
      {"●", "◕", "◑", "◔", "○"},                           // kiro (drain)
      {"○", "◔", "◑", "◕", "●", "◕", "◑", "◔"},            // kiro (fill+drain)
      {"●", "◕", "◑", "◔", "○", "◔", "◑", "◕"},            // kiro (drain+fill)
  };
  // Pick a random style for this spinner instance (seeded by clock)
  auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
  auto idx = static_cast<size_t>(seed) % styles.size();
  const auto& frames = styles[idx];

  int i = 0;
  int msg_idx = 0;
  // Animate until stop() sets running_ to false
  while (running_) {
    // Dim text (\033[2m) so spinner doesn't distract from output
    out_ << "\r\033[2m" << frames[i % frames.size()] << " " << msgs_[msg_idx % msgs_.size()] << "\033[0m\033[K" << std::flush;
    i++;
    if (i % 30 == 0) {
      msg_idx++;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
  }
}
