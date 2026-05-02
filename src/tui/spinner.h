/**
 * @file spinner.h
 * @brief RAII spinner animation for terminal wait states.
 * @see docs/adr/adr-066-solid-refactoring.md
 */
#ifndef LLAMA_CLI_SPINNER_H
#define LLAMA_CLI_SPINNER_H

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

/** RAII spinner — shows animated dots while waiting for LLM response.
 * Only active when constructed with active=true (interactive TTY).
 * Destructor stops the animation and clears the line automatically. */
class Spinner {
 public:
  /** Start spinner with custom messages. No-op if active=false. */
  Spinner(std::ostream& out, bool active, std::vector<const char*> msgs = {});
  ~Spinner();
  void stop();
  Spinner(const Spinner&) = delete;
  Spinner& operator=(const Spinner&) = delete;

 private:
  void run();
  std::ostream& out_;
  std::atomic<bool> running_;
  std::vector<const char*> msgs_;
  std::thread thread_;
};

#endif  // LLAMA_CLI_SPINNER_H
