/**
 * @file markdown_it.cpp
 * @brief Integration tests: markdown renderer, on/off toggle
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

#include "test_helpers.h"
#include "tui/tui.h"

SCENARIO ("Markdown rendering on vs off in REPL") {
  GIVEN ("the LLM responds with markdown") {
    auto md_chat = [](const std::vector<Message>&) { return "# Title\n**bold** and *italic*\n- item one"; };

    WHEN ("markdown is off via /set") {
      std::istringstream in("/set markdown\nrender\nexit\n");
      std::ostringstream out;
      run_repl(md_chat, test_cfg(), in, out);
      THEN ("raw markdown is preserved") {
        CHECK (out.str().find("# Title") != std::string::npos)
          ;
        CHECK (out.str().find("**bold**") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("Markdown renderer") {
  GIVEN ("color is enabled") {
    WHEN ("rendering a heading") {
      auto r = tui::render_markdown("# Hello", true);
      THEN ("# is stripped") {
        CHECK (r.find("Hello") != std::string::npos)
          ;
        CHECK (r.find("# ") == std::string::npos)
          ;
      }
    }
    WHEN ("rendering bold") {
      auto r = tui::render_markdown("**bold**", true);
      THEN ("** is stripped") {
        CHECK (r.find("bold") != std::string::npos)
          ;
        CHECK (r.find("**") == std::string::npos)
          ;
      }
    }
    WHEN ("rendering italic") {
      auto r = tui::render_markdown("*italic*", true);
      THEN ("* is stripped") {
        CHECK (r.find("italic") != std::string::npos)
          ;
        CHECK (r.find("*italic*") == std::string::npos)
          ;
      }
    }
    WHEN ("rendering inline code") {
      auto r = tui::render_markdown("`code`", true);
      THEN ("backticks are stripped") {
        CHECK (r.find("code") != std::string::npos)
          ;
        CHECK (r.find("`code`") == std::string::npos)
          ;
      }
    }
    WHEN ("rendering a bullet list") {
      auto r = tui::render_markdown("- item one\n- item two", true);
      THEN ("bullets are converted") {
        CHECK (r.find("•") != std::string::npos)
          ;
        CHECK (r.find("item one") != std::string::npos)
          ;
      }
    }
    WHEN ("rendering a numbered list") {
      auto r = tui::render_markdown("1. first\n2. second", true);
      THEN ("numbers preserved") {
        CHECK (r.find("1.") != std::string::npos)
          ;
      }
    }
    WHEN ("rendering a code block") {
      auto r = tui::render_markdown("```\nint x = 1;\n```", true);
      THEN ("code preserved") {
        CHECK (r.find("int x = 1;") != std::string::npos)
          ;
      }
    }
    WHEN ("rendering a horizontal rule with dashes") {
      auto r = tui::render_markdown("---", true);
      THEN ("rule is rendered as dim line") {
        CHECK (r.find("────") != std::string::npos)
          ;
        CHECK (r.find("---") == std::string::npos)
          ;
      }
    }
    WHEN ("rendering a horizontal rule with asterisks") {
      auto r = tui::render_markdown("***", true);
      THEN ("rule is rendered as dim line") {
        CHECK (r.find("────") != std::string::npos)
          ;
      }
    }
    WHEN ("rendering a horizontal rule with underscores") {
      auto r = tui::render_markdown("___", true);
      THEN ("rule is rendered as dim line") {
        CHECK (r.find("────") != std::string::npos)
          ;
      }
    }
    WHEN ("rendering a blockquote") {
      auto r = tui::render_markdown("> This is a quote", true);
      THEN ("quote has bar prefix and content") {
        CHECK (r.find("│") != std::string::npos)
          ;
        CHECK (r.find("This is a quote") != std::string::npos)
          ;
      }
    }
    WHEN ("rendering a blockquote without space after >") {
      auto r = tui::render_markdown(">no space", true);
      THEN ("still renders as blockquote") {
        CHECK (r.find("│") != std::string::npos)
          ;
        CHECK (r.find("no space") != std::string::npos)
          ;
      }
    }
    WHEN ("rendering an indented sub-list") {
      auto r = tui::render_markdown("- parent\n  - child", true);
      THEN ("both items have bullets") {
        // Both should have bullet markers
        auto first = r.find("•");
        auto second = r.find("•", first + 1);
        CHECK (first != std::string::npos)
          ;
        CHECK (second != std::string::npos)
          ;
        CHECK (r.find("parent") != std::string::npos)
          ;
        CHECK (r.find("child") != std::string::npos)
          ;
      }
    }
    WHEN ("rendering strikethrough") {
      auto r = tui::render_markdown("~~deleted~~", true);
      THEN ("~~ is stripped") {
        CHECK (r.find("deleted") != std::string::npos)
          ;
        CHECK (r.find("~~") == std::string::npos)
          ;
      }
    }
    WHEN ("rendering bold+italic") {
      auto r = tui::render_markdown("***important***", true);
      THEN ("*** is stripped") {
        CHECK (r.find("important") != std::string::npos)
          ;
        CHECK (r.find("***") == std::string::npos)
          ;
      }
    }
    WHEN ("rendering a link") {
      auto r = tui::render_markdown("[click here](https://example.com)", true);
      THEN ("text and url are both shown") {
        CHECK (r.find("click here") != std::string::npos)
          ;
        CHECK (r.find("https://example.com") != std::string::npos)
          ;
        // Raw markdown syntax should be gone
        CHECK (r.find("](") == std::string::npos)
          ;
      }
    }
    WHEN ("rendering mixed inline formatting") {
      auto r = tui::render_markdown("Use **bold** and ~~old~~ with `code`", true);
      THEN ("all formatting is applied") {
        CHECK (r.find("bold") != std::string::npos)
          ;
        CHECK (r.find("old") != std::string::npos)
          ;
        CHECK (r.find("code") != std::string::npos)
          ;
        CHECK (r.find("**") == std::string::npos)
          ;
        CHECK (r.find("~~") == std::string::npos)
          ;
        CHECK (r.find("`code`") == std::string::npos)
          ;
      }
    }
  }
  GIVEN ("color is disabled") {
    WHEN ("rendering markdown") {
      auto r = tui::render_markdown("# Hello\n**bold**", false);
      THEN ("text unchanged") {
        CHECK (r == "# Hello\n**bold**")
          ;
      }
    }
  }
}

SCENARIO ("Table rendering") {
  GIVEN ("a simple markdown table") {
    std::string table = "| Name | Age |\n| --- | --- |\n| Alice | 30 |";
    WHEN ("rendered with color") {
      auto r = tui::render_markdown(table, true);
      THEN ("cell content is preserved") {
        CHECK (r.find("Name") != std::string::npos)
          ;
        CHECK (r.find("Alice") != std::string::npos)
          ;
        CHECK (r.find("30") != std::string::npos)
          ;
      }
      THEN ("separator row is dimmed") {
        // Separator becomes dashes, wrapped in dim ANSI
        CHECK (r.find("---") != std::string::npos)
          ;
      }
    }
    WHEN ("rendered without color") {
      auto r = tui::render_markdown(table, false);
      THEN ("raw markdown is preserved") {
        CHECK (r == table)
          ;
      }
    }
  }
  GIVEN ("a table with varying column widths") {
    std::string table = "| Short | A much longer column |\n| --- | --- |\n| x | y |";
    WHEN ("rendered with color") {
      auto r = tui::render_markdown(table, true);
      THEN ("all content is present") {
        CHECK (r.find("Short") != std::string::npos)
          ;
        CHECK (r.find("A much longer column") != std::string::npos)
          ;
        CHECK (r.find("x") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("table helper functions") {
    WHEN ("checking is_table_row") {
      CHECK (tui::is_table_row("| a | b |"))
        ;
      CHECK_FALSE (tui::is_table_row("not a table"))
        ;
      CHECK_FALSE (tui::is_table_row(""))
        ;
    }
    WHEN ("checking is_table_separator") {
      CHECK (tui::is_table_separator("| --- | --- |"))
        ;
      CHECK (tui::is_table_separator("|---|---|"))
        ;
      CHECK (tui::is_table_separator("| :---: | ---: |"))
        ;
      CHECK_FALSE (tui::is_table_separator("| Name | Age |"))
        ;
    }
    WHEN ("parsing table cells") {
      auto cells = tui::parse_table_cells("| Alice | 30 | Engineer |");
      CHECK (cells.size() == 3)
        ;
      CHECK (cells[0] == "Alice")
        ;
      CHECK (cells[1] == "30")
        ;
      CHECK (cells[2] == "Engineer")
        ;
    }
  }
}

SCENARIO ("StreamRenderer handles tables") {
  GIVEN ("a StreamRenderer with color enabled") {
    std::ostringstream out;
    StreamRenderer sr(out, true);
    WHEN ("streaming a complete table") {
      sr.write("| A | B |\n");
      sr.write("| --- | --- |\n");
      sr.write("| 1 | 2 |\n");
      sr.write("done\n");
      sr.finish();
      THEN ("table content is rendered before the next line") {
        auto s = out.str();
        CHECK (s.find("A") != std::string::npos)
          ;
        CHECK (s.find("B") != std::string::npos)
          ;
        CHECK (s.find("1") != std::string::npos)
          ;
        CHECK (s.find("done") != std::string::npos)
          ;
      }
    }
    WHEN ("streaming a table at end of output") {
      sr.write("| X | Y |\n");
      sr.finish();
      THEN ("table is flushed on finish") {
        auto s = out.str();
        CHECK (s.find("X") != std::string::npos)
          ;
        CHECK (s.find("Y") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("StreamRenderer handles new block elements") {
  GIVEN ("a StreamRenderer with color enabled") {
    std::ostringstream out;
    StreamRenderer sr(out, true);

    WHEN ("streaming a horizontal rule") {
      sr.write("---\n");
      sr.finish();
      THEN ("rule is rendered") {
        CHECK (out.str().find("────") != std::string::npos)
          ;
      }
    }
    WHEN ("streaming a blockquote") {
      sr.write("> quoted text\n");
      sr.finish();
      THEN ("blockquote has bar prefix") {
        CHECK (out.str().find("│") != std::string::npos)
          ;
        CHECK (out.str().find("quoted text") != std::string::npos)
          ;
      }
    }
    WHEN ("streaming strikethrough inline") {
      sr.write("~~removed~~\n");
      sr.finish();
      THEN ("strikethrough is rendered") {
        CHECK (out.str().find("removed") != std::string::npos)
          ;
        CHECK (out.str().find("~~") == std::string::npos)
          ;
      }
    }
    WHEN ("streaming bold+italic inline") {
      sr.write("***emphasis***\n");
      sr.finish();
      THEN ("bold+italic is rendered") {
        CHECK (out.str().find("emphasis") != std::string::npos)
          ;
        CHECK (out.str().find("***") == std::string::npos)
          ;
      }
    }
    WHEN ("streaming a link") {
      sr.write("[docs](https://docs.rs)\n");
      sr.finish();
      THEN ("link text and url are shown") {
        CHECK (out.str().find("docs") != std::string::npos)
          ;
        CHECK (out.str().find("https://docs.rs") != std::string::npos)
          ;
      }
    }
  }
}
