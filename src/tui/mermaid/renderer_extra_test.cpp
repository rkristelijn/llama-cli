/**
 * @file renderer_extra_test.cpp
 * @brief Unit tests for new diagram renderers: venn, gantt, mindmap,
 *        quadrant, timeline, kanban, barchart, orgchart.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

#include "tui/mermaid/renderer.h"

// --- Venn diagram renderer tests ---

SCENARIO ("Venn diagram renders overlapping circles with legend") {
  GIVEN ("a two-set venn diagram") {
    std::string input = "venn title Skills\n    \"Frontend\" : 60\n    \"Backend\" : 40\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("title and legend appear") {
        CHECK (ok)
          ;
        CHECK (out.str().find("Skills") != std::string::npos)
          ;
        CHECK (out.str().find("Frontend") != std::string::npos)
          ;
        CHECK (out.str().find("Backend") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a three-set venn") {
    std::string input = "venn\n    \"A\" : 30\n    \"B\" : 30\n    \"C\" : 30\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("all three sets in legend") {
        CHECK (ok)
          ;
        CHECK (out.str().find("A (30)") != std::string::npos)
          ;
        CHECK (out.str().find("B (30)") != std::string::npos)
          ;
        CHECK (out.str().find("C (30)") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("more than 3 sets") {
    std::string input = "venn\n    \"A\":1\n    \"B\":1\n    \"C\":1\n    \"D\":1\n";
    std::ostringstream out;
    WHEN ("rendered") {
      THEN ("fails — max 3 sets") {
        CHECK (!tui::diagram_registry().render(input, out))
          ;
      }
    }
  }
}

// --- Gantt chart renderer tests ---

SCENARIO ("Gantt chart renders horizontal time bars") {
  GIVEN ("a gantt with sections and tasks") {
    std::string input = R"(gantt
    title Sprint
    section Dev
    Coding :a1, 2024-01-01, 10d
    Testing :a2, after a1, 5d
    section QA
    Review :b1, after a2, 3d
)";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out, 60);
      THEN ("title and labels appear") {
        CHECK (ok)
          ;
        CHECK (out.str().find("Sprint") != std::string::npos)
          ;
        CHECK (out.str().find("Coding") != std::string::npos)
          ;
        CHECK (out.str().find("Testing") != std::string::npos)
          ;
        CHECK (out.str().find("Dev") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("an empty gantt") {
    std::ostringstream out;
    WHEN ("rendered") {
      THEN ("fails") {
        CHECK (!tui::diagram_registry().render("gantt\n    title Empty\n", out))
          ;
      }
    }
  }
}

// --- Mindmap renderer tests ---

SCENARIO ("Mindmap renders as tree with connectors") {
  GIVEN ("a mindmap with branches") {
    std::string input = "mindmap\n  root((Topic))\n    A\n      A1\n      A2\n    B\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      std::string result = out.str();
      THEN ("root and branches appear") {
        CHECK (ok)
          ;
        CHECK (result.find("Topic") != std::string::npos)
          ;
        CHECK (result.find("A1") != std::string::npos)
          ;
        CHECK (result.find("B") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("an empty mindmap") {
    std::ostringstream out;
    WHEN ("rendered") {
      THEN ("fails") {
        CHECK (!tui::diagram_registry().render("mindmap\n", out))
          ;
      }
    }
  }
}

// --- Quadrant chart renderer tests ---

SCENARIO ("Quadrant chart renders 2x2 grid with items") {
  GIVEN ("a quadrant with items") {
    std::string input = R"(quadrantChart
    title Priority
    x-axis Low --> High
    y-axis Low --> High
    quadrant-1 Do
    quadrant-2 Plan
    quadrant-3 Delegate
    quadrant-4 Drop
    Fix bug: [0.2, 0.8]
    Rewrite: [0.9, 0.2]
)";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out, 60);
      std::string result = out.str();
      THEN ("title, quadrants and items appear") {
        CHECK (ok)
          ;
        CHECK (result.find("Priority") != std::string::npos)
          ;
        CHECK (result.find("Do") != std::string::npos)
          ;
        CHECK (result.find("Fix bug") != std::string::npos)
          ;
        CHECK (result.find("Rewrite") != std::string::npos)
          ;
      }
    }
  }
}

// --- Timeline renderer tests ---

SCENARIO ("Timeline renders vertical events with markers") {
  GIVEN ("a timeline with sections") {
    std::string input = "timeline\n    title History\n    section 2024\n      Jan : Start\n      Jun : v1.0\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      std::string result = out.str();
      THEN ("title, sections and events appear") {
        CHECK (ok)
          ;
        CHECK (result.find("History") != std::string::npos)
          ;
        CHECK (result.find("2024") != std::string::npos)
          ;
        CHECK (result.find("Jan") != std::string::npos)
          ;
        CHECK (result.find("Start") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("an empty timeline") {
    std::ostringstream out;
    WHEN ("rendered") {
      THEN ("fails") {
        CHECK (!tui::diagram_registry().render("timeline\n", out))
          ;
      }
    }
  }
}

// --- Kanban renderer tests ---

SCENARIO ("Kanban renders multi-column board") {
  GIVEN ("a kanban with columns and items") {
    std::string input = "kanban\n    title Board\n    column Todo\n      Task A\n    column Done\n      Task C\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out, 60);
      std::string result = out.str();
      THEN ("title, columns and items appear") {
        CHECK (ok)
          ;
        CHECK (result.find("Board") != std::string::npos)
          ;
        CHECK (result.find("Todo") != std::string::npos)
          ;
        CHECK (result.find("Done") != std::string::npos)
          ;
        CHECK (result.find("Task A") != std::string::npos)
          ;
        CHECK (result.find("Task C") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("an empty kanban") {
    std::ostringstream out;
    WHEN ("rendered") {
      THEN ("fails") {
        CHECK (!tui::diagram_registry().render("kanban\n    title X\n", out))
          ;
      }
    }
  }
}

// --- Bar chart renderer tests ---

SCENARIO ("Bar chart renders horizontal bars with absolute values") {
  GIVEN ("a bar chart with items") {
    std::string input = "barchart\n    title Metrics\n    \"Alpha\" : 100\n    \"Beta\" : 50\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out, 60);
      std::string result = out.str();
      THEN ("title, labels and values appear") {
        CHECK (ok)
          ;
        CHECK (result.find("Metrics") != std::string::npos)
          ;
        CHECK (result.find("Alpha") != std::string::npos)
          ;
        CHECK (result.find("100") != std::string::npos)
          ;
        CHECK (result.find("50") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("an empty bar chart") {
    std::ostringstream out;
    WHEN ("rendered") {
      THEN ("fails") {
        CHECK (!tui::diagram_registry().render("barchart\n    title X\n", out))
          ;
      }
    }
  }
}

// --- Org chart renderer tests ---

SCENARIO ("Org chart renders hierarchy as flowchart") {
  GIVEN ("an org chart with hierarchy") {
    std::string input = "orgchart\n    CEO\n      CTO\n        Dev\n      CFO\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out, 60);
      std::string result = out.str();
      THEN ("all nodes appear in boxes") {
        CHECK (ok)
          ;
        CHECK (result.find("CEO") != std::string::npos)
          ;
        CHECK (result.find("CTO") != std::string::npos)
          ;
        CHECK (result.find("CFO") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("an empty org chart") {
    std::ostringstream out;
    WHEN ("rendered") {
      THEN ("fails") {
        CHECK (!tui::diagram_registry().render("orgchart\n", out))
          ;
      }
    }
  }
}
