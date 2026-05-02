/**
 * @file highlight.cpp
 * @brief Pluggable syntax highlighting implementation.
 *
 * Built-in regex highlighter for common languages + external tool adapter.
 * The regex approach is intentionally simple: it handles keywords, strings,
 * single-line comments, and numbers. No multi-line state (block comments
 * are not tracked across lines) — good enough for terminal display.
 *
 * @see highlight.h
 */

#include "tui/highlight.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace tui {

// --- ANSI color codes for syntax elements ---
// Chosen for readability on dark terminals (most common for CLI users).
static const char* kKeyword = "\033[1;33m";  // bold yellow
static const char* kString = "\033[32m";     // green
static const char* kComment = "\033[2;37m";  // dim gray
static const char* kNumber = "\033[35m";     // magenta
static const char* kType = "\033[36m";       // cyan
static const char* kPreproc = "\033[1;35m";  // bold magenta
static const char* kReset = "\033[0m";
static const char* kCodeBase = "\033[36m";  // cyan (base code color, matches current)

// --- Language keyword tables ---

// Check if character is a word boundary (not alphanumeric or underscore)
static bool is_boundary(char c) { return !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != '_'; }

// Check if a keyword appears at position i with word boundaries
static bool match_keyword(const std::string& line, size_t i, const std::string& kw) {
  if (i + kw.size() > line.size()) {
    return false;
  }
  if (line.compare(i, kw.size(), kw) != 0) {
    return false;
  }
  // Check left boundary
  if (i > 0 && !is_boundary(line[i - 1])) {
    return false;
  }
  // Check right boundary
  if (i + kw.size() < line.size() && !is_boundary(line[i + kw.size()])) {
    return false;
  }
  return true;
}

// Language-specific keyword sets
static const std::vector<std::string> cpp_keywords = {
    "auto",    "break",     "case",   "catch",    "class",   "const",    "constexpr", "continue", "default",   "delete",
    "do",      "else",      "enum",   "explicit", "extern",  "false",    "for",       "friend",   "goto",      "if",
    "inline",  "namespace", "new",    "noexcept", "nullptr", "operator", "override",  "private",  "protected", "public",
    "return",  "sizeof",    "static", "struct",   "switch",  "template", "this",      "throw",    "true",      "try",
    "typedef", "typename",  "using",  "virtual",  "void",    "volatile", "while",
};

static const std::vector<std::string> cpp_types = {
    "int",    "char",   "bool",   "float", "double", "long",       "short",      "unsigned",
    "size_t", "string", "vector", "map",   "set",    "unique_ptr", "shared_ptr",
};

static const std::vector<std::string> python_keywords = {
    "and",      "as",    "assert",  "async", "await", "break",  "class", "continue", "def",   "del",  "elif",   "else",
    "except",   "False", "finally", "for",   "from",  "global", "if",    "import",   "in",    "is",   "lambda", "None",
    "nonlocal", "not",   "or",      "pass",  "raise", "return", "True",  "try",      "while", "with", "yield",
};

static const std::vector<std::string> bash_keywords = {
    "if",   "then", "else",     "elif",   "fi",    "for",    "while",  "do",       "done",    "case",
    "esac", "in",   "function", "return", "local", "export", "source", "readonly", "declare", "unset",
};

static const std::vector<std::string> js_keywords = {
    "async",   "await", "break",   "case", "catch",    "class", "const",  "continue", "default",    "delete", "do",   "else",  "export",
    "extends", "false", "finally", "for",  "function", "if",    "import", "in",       "instanceof", "let",    "new",  "null",  "of",
    "return",  "super", "switch",  "this", "throw",    "true",  "try",    "typeof",   "undefined",  "var",    "void", "while", "yield",
};

static const std::vector<std::string> java_keywords = {
    "abstract",   "assert",    "boolean",    "break", "byte",      "case",     "catch",  "char",    "class",        "const",   "continue",
    "default",    "do",        "double",     "else",  "enum",      "extends",  "final",  "finally", "float",        "for",     "if",
    "implements", "import",    "instanceof", "int",   "interface", "long",     "native", "new",     "null",         "package", "private",
    "protected",  "public",    "return",     "short", "static",    "strictfp", "super",  "switch",  "synchronized", "this",    "throw",
    "throws",     "transient", "try",        "void",  "volatile",  "while",    "true",   "false",
};

static const std::vector<std::string> php_keywords = {
    "abstract", "and",        "array",     "as",        "break",    "callable",   "case",    "catch",      "class",     "clone",
    "const",    "continue",   "declare",   "default",   "do",       "echo",       "else",    "elseif",     "empty",     "enddeclare",
    "endfor",   "endforeach", "endif",     "endswitch", "endwhile", "extends",    "final",   "finally",    "fn",        "for",
    "foreach",  "function",   "global",    "goto",      "if",       "implements", "include", "instanceof", "interface", "isset",
    "list",     "match",      "namespace", "new",       "null",     "or",         "print",   "private",    "protected", "public",
    "readonly", "require",    "return",    "static",    "switch",   "throw",      "trait",   "true",       "false",     "try",
    "unset",    "use",        "var",       "while",     "yield",
};

static const std::vector<std::string> swift_keywords = {
    "associatedtype", "break",    "case",      "catch",       "class",    "continue",    "default", "defer",     "deinit",   "do",
    "else",           "enum",     "extension", "fallthrough", "false",    "fileprivate", "for",     "func",      "guard",    "if",
    "import",         "in",       "init",      "inout",       "internal", "is",          "let",     "nil",       "operator", "override",
    "private",        "protocol", "public",    "repeat",      "rethrows", "return",      "self",    "Self",      "static",   "struct",
    "subscript",      "super",    "switch",    "throw",       "throws",   "true",        "try",     "typealias", "var",      "where",
    "while",
};

static const std::vector<std::string> rust_keywords = {
    "as",   "async", "await",  "break",  "const", "continue", "crate", "dyn",  "else",   "enum", "extern", "false", "fn",
    "for",  "if",    "impl",   "in",     "let",   "loop",     "match", "mod",  "move",   "mut",  "pub",    "ref",   "return",
    "self", "Self",  "static", "struct", "super", "trait",    "true",  "type", "unsafe", "use",  "where",  "while",
};

static const std::vector<std::string> go_keywords = {
    "break", "case",   "chan",      "const", "continue", "default", "defer",  "else",   "fallthrough", "for",    "func", "go",  "goto",
    "if",    "import", "interface", "map",   "package",  "range",   "return", "select", "struct",      "switch", "type", "var",
};

// --- Determine language family from fence tag ---

/// Language families for syntax highlighting keyword selection.
enum class LangFamily { Unknown, Cpp, Python, Bash, Js, Java, Php, Swift, Rust, Go };

static LangFamily classify_lang(const std::string& lang) {
  if (lang == "cpp" || lang == "c++" || lang == "c" || lang == "h" || lang == "hpp") {
    return LangFamily::Cpp;
  }
  if (lang == "python" || lang == "py") {
    return LangFamily::Python;
  }
  if (lang == "bash" || lang == "sh" || lang == "shell" || lang == "zsh") {
    return LangFamily::Bash;
  }
  if (lang == "javascript" || lang == "js" || lang == "typescript" || lang == "ts" || lang == "jsx" || lang == "tsx") {
    return LangFamily::Js;
  }
  if (lang == "java") {
    return LangFamily::Java;
  }
  if (lang == "php") {
    return LangFamily::Php;
  }
  if (lang == "swift") {
    return LangFamily::Swift;
  }
  if (lang == "rust" || lang == "rs") {
    return LangFamily::Rust;
  }
  if (lang == "go" || lang == "golang") {
    return LangFamily::Go;
  }
  return LangFamily::Unknown;
}

static const std::vector<std::string>& keywords_for(LangFamily fam) {
  switch (fam) {
    case LangFamily::Cpp:
      return cpp_keywords;
    case LangFamily::Python:
      return python_keywords;
    case LangFamily::Bash:
      return bash_keywords;
    case LangFamily::Js:
      return js_keywords;
    case LangFamily::Java:
      return java_keywords;
    case LangFamily::Php:
      return php_keywords;
    case LangFamily::Swift:
      return swift_keywords;
    case LangFamily::Rust:
      return rust_keywords;
    case LangFamily::Go:
      return go_keywords;
    default: {
      static const std::vector<std::string> empty;
      return empty;
    }
  }
}

// Comment prefix for each language family
static std::string comment_prefix(LangFamily fam) {
  switch (fam) {
    case LangFamily::Python:
      return "#";
    case LangFamily::Bash:
      return "#";
    case LangFamily::Php:
      return "//";
    default:
      return "//";
  }
}

// --- RegexHighlighter implementation ---

std::string RegexHighlighter::highlight_line(const std::string& line, const std::string& lang) const {
  LangFamily fam = classify_lang(lang);
  if (fam == LangFamily::Unknown) {
    // No highlighting for unknown languages — just cyan like before
    return std::string(kCodeBase) + line + kReset;
  }

  const auto& kws = keywords_for(fam);
  std::string cmt = comment_prefix(fam);
  std::string out;
  out.reserve(line.size() * 2);
  // After each highlight, return to base code color (not full reset)
  const char* back = kCodeBase;

  bool in_string = false;
  char string_char = 0;
  size_t i = 0;

  while (i < line.size()) {
    // Check for single-line comment
    if (!in_string && line.compare(i, cmt.size(), cmt) == 0) {
      out += kComment;
      out += line.substr(i);
      out += kReset;
      return std::string(kCodeBase) + out;
    }

    // Check for preprocessor directives (C/C++)
    if (!in_string && i == 0 && fam == LangFamily::Cpp && line[0] == '#') {
      out += kPreproc;
      out += line;
      out += kReset;
      return std::string(kCodeBase) + out;
    }

    // String handling
    if (!in_string && (line[i] == '"' || line[i] == '\'')) {
      in_string = true;
      string_char = line[i];
      out += kString;
      out += line[i];
      ++i;
      // Consume until closing quote
      while (i < line.size()) {
        out += line[i];
        if (line[i] == string_char && (i == 0 || line[i - 1] != '\\')) {
          in_string = false;
          ++i;
          break;
        }
        ++i;
      }
      out += back;
      continue;
    }

    // Number literals
    if (!in_string && (line[i] >= '0' && line[i] <= '9') && (i == 0 || is_boundary(line[i - 1]))) {
      out += kNumber;
      while (i < line.size() && ((line[i] >= '0' && line[i] <= '9') || line[i] == '.' || line[i] == 'x' || line[i] == 'f')) {
        out += line[i];
        ++i;
      }
      out += back;
      continue;
    }

    // Keyword check
    if (!in_string && (i == 0 || is_boundary(line[i - 1]))) {
      bool found = false;
      for (const auto& kw : kws) {
        if (match_keyword(line, i, kw)) {
          out += kKeyword;
          out += kw;
          out += back;
          i += kw.size();
          found = true;
          break;
        }
      }
      if (found) {
        continue;
      }
      // Type check (C++ only)
      if (fam == LangFamily::Cpp) {
        for (const auto& tp : cpp_types) {
          if (match_keyword(line, i, tp)) {
            out += kType;
            out += tp;
            out += back;
            i += tp.size();
            found = true;
            break;
          }
        }
        if (found) {
          continue;
        }
      }
    }

    // Default: emit character (base color applied at wrapper level)
    out += line[i];
    ++i;
  }

  // Wrap in base code color — embedded highlights override then reset back
  return std::string(kCodeBase) + out + kReset;
}

// --- ExternalHighlighter implementation ---

ExternalHighlighter::ExternalHighlighter(std::string tool_path, std::string tool_name)
    : tool_path_(std::move(tool_path)), tool_name_(std::move(tool_name)) {}

std::string ExternalHighlighter::highlight_line(const std::string& line, const std::string& lang) const {
  // External tools work best on full blocks, not single lines.
  // For per-line usage, fall back to builtin regex highlighter.
  return fallback_.highlight_line(line, lang);
}

// --- Detection and singleton ---

/// Check if a command exists in PATH
static bool command_exists(const std::string& cmd) {
  std::string check = "command -v " + cmd + " >/dev/null 2>&1";
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  return system(check.c_str()) == 0;
}

std::unique_ptr<SyntaxHighlighter> detect_highlighter() {
  // Prefer bat (modern, ANSI-aware), then pygmentize, then builtin
  if (command_exists("bat")) {
    return std::make_unique<ExternalHighlighter>("bat", "bat");
  }
  if (command_exists("pygmentize")) {
    return std::make_unique<ExternalHighlighter>("pygmentize", "pygmentize");
  }
  return std::make_unique<RegexHighlighter>();
}

const SyntaxHighlighter& active_highlighter() {
  static auto instance = detect_highlighter();
  return *instance;
}

}  // namespace tui
