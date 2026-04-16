# REPL end-to-end testing

## CLI-Based REPL Testing

For standalone command-line tools, you need a framework that can handle interactive stdin/stdout streams.

- **BATS** (Bash Automated Testing System): Excellent for testing CLI outputs and exit codes.
- **Expect / Auto-Expect**: A classic tool specifically designed for interactive applications. It "expects" a certain string (like a prompt `>`) and then "sends" a command in response.
- **Mire**: A "record and replay" tool for CLI E2E tests that allows you to manually perform actions and then automate the replay for future tests.
- **Python's pexpect**: If you prefer Python, pexpect is the standard library for controlling interactive child applications, letting you wait for specific output patterns before sending new input.
