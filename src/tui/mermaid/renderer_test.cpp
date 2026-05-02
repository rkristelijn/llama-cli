/**
 * @file renderer_test.cpp
 * @brief Unit tests for pluggable diagram renderers.
 *
 * Tests the registry dispatch, sequence, pie, state, and flowchart renderers.
 * Each diagram type has tests for: basic rendering, edge cases, and failure modes.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "tui/mermaid/renderer.h"

#include <doctest/doctest.h>

#include <sstream>

#include "tui/markdown.h"

// --- Registry dispatch tests ---

SCENARIO ("Diagram registry dispatches to correct renderer") {
  GIVEN ("the global diagram registry") {
    auto& reg = tui::diagram_registry();

    WHEN ("input starts with 'graph TD'") {
      std::ostringstream out;
      bool ok = reg.render("graph TD\n    A --> B\n", out);
      THEN ("flowchart renderer handles it") {
        CHECK (ok)
          ;
        CHECK (!out.str().empty())
          ;
      }
    }
    WHEN ("input starts with 'flowchart LR'") {
      std::ostringstream out;
      bool ok = reg.render("flowchart LR\n    A --> B\n", out);
      THEN ("flowchart renderer handles it") {
        CHECK (ok)
          ;
      }
    }
    WHEN ("input starts with 'sequenceDiagram'") {
      std::ostringstream out;
      bool ok = reg.render("sequenceDiagram\n    A->>B: hi\n", out);
      THEN ("sequence renderer handles it") {
        CHECK (ok)
          ;
      }
    }
    WHEN ("input starts with 'pie'") {
      std::ostringstream out;
      bool ok = reg.render("pie\n    \"A\" : 50\n    \"B\" : 50\n", out);
      THEN ("pie renderer handles it") {
        CHECK (ok)
          ;
      }
    }
    WHEN ("input starts with 'stateDiagram-v2'") {
      std::ostringstream out;
      bool ok = reg.render("stateDiagram-v2\n    [*] --> Active\n    Active --> [*]\n", out);
      THEN ("state renderer handles it") {
        CHECK (ok)
          ;
      }
    }
    WHEN ("input is an unknown diagram type") {
      std::ostringstream out;
      bool ok = reg.render("gantt\n    title Plan\n", out);
      THEN ("no renderer matches, returns false") {
        CHECK (!ok)
          ;
        CHECK (out.str().empty())
          ;
      }
    }
    WHEN ("input is empty") {
      std::ostringstream out;
      bool ok = reg.render("", out);
      THEN ("returns false") {
        CHECK (!ok)
          ;
      }
    }
  }
}

// --- Flowchart renderer tests ---

SCENARIO ("Flowchart renderer produces braille art with labels") {
  GIVEN ("a simple linear flowchart") {
    std::string input = "graph TD\n    A[Start] --> B[Process] --> C[End]\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("braille output is produced") {
        CHECK (ok)
          ;
        // Braille characters are in U+2800 range (3-byte UTF-8: 0xE2 0xA0-0xBF ...)
        CHECK (out.str().find("\xe2") != std::string::npos)
          ;
      }
      THEN ("node labels appear below the diagram") {
        CHECK (out.str().find("Start") != std::string::npos)
          ;
        CHECK (out.str().find("Process") != std::string::npos)
          ;
        CHECK (out.str().find("End") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a flowchart with branching") {
    std::string input = "graph TD\n    A --> B\n    A --> C\n    B --> D\n    C --> D\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("succeeds") {
        CHECK (ok)
          ;
        CHECK (!out.str().empty())
          ;
      }
    }
  }
  GIVEN ("a left-to-right flowchart") {
    std::string input = "graph LR\n    A --> B --> C\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("succeeds") {
        CHECK (ok)
          ;
      }
    }
  }
  GIVEN ("a flowchart with no edges") {
    std::string input = "graph TD\n    A[Lonely]\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("fails — no edges means no graph") {
        CHECK (!ok)
          ;
      }
    }
  }
}

// --- Sequence diagram renderer tests ---

SCENARIO ("Sequence diagram renders participants and messages") {
  GIVEN ("a basic two-participant sequence") {
    std::string input = R"(sequenceDiagram
    participant Client
    participant Server
    Client->>Server: GET /data
    Server-->>Client: 200 OK
)";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      std::string result = out.str();
      THEN ("participants appear in header") {
        CHECK (ok)
          ;
        CHECK (result.find("Client") != std::string::npos)
          ;
        CHECK (result.find("Server") != std::string::npos)
          ;
      }
      THEN ("message labels are visible") {
        CHECK (result.find("GET /data") != std::string::npos)
          ;
        CHECK (result.find("200 OK") != std::string::npos)
          ;
      }
      THEN ("arrows are drawn") {
        CHECK (result.find(">") != std::string::npos)
          ;
        CHECK (result.find("<") != std::string::npos)
          ;
      }
      THEN ("lifelines are present") {
        CHECK (result.find("|") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("participants auto-added from messages") {
    std::string input = "sequenceDiagram\n    Alice->>Bob: Hello\n    Bob-->>Alice: Hi back\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("both participants appear") {
        CHECK (ok)
          ;
        CHECK (out.str().find("Alice") != std::string::npos)
          ;
        CHECK (out.str().find("Bob") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a sequence with notes") {
    std::string input = R"(sequenceDiagram
    participant A
    participant B
    Note over A: Thinking...
    A->>B: Request
)";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("note text is visible") {
        CHECK (ok)
          ;
        CHECK (out.str().find("Thinking") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a sequence with many participants") {
    std::string input = R"(sequenceDiagram
    participant User
    participant Frontend
    participant Backend
    participant Database
    User->>Frontend: Click button
    Frontend->>Backend: API call
    Backend->>Database: Query
    Database-->>Backend: Results
    Backend-->>Frontend: JSON response
    Frontend-->>User: Update UI
)";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      std::string result = out.str();
      THEN ("all participants visible") {
        CHECK (ok)
          ;
        CHECK (result.find("User") != std::string::npos)
          ;
        CHECK (result.find("Frontend") != std::string::npos)
          ;
        CHECK (result.find("Backend") != std::string::npos)
          ;
        CHECK (result.find("Database") != std::string::npos)
          ;
      }
      THEN ("all labels visible") {
        CHECK (result.find("Click button") != std::string::npos)
          ;
        CHECK (result.find("API call") != std::string::npos)
          ;
        CHECK (result.find("Query") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a sequence with activate/deactivate (ignored)") {
    std::string input = R"(sequenceDiagram
    A->>B: Request
    activate B
    B-->>A: Response
    deactivate B
)";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("renders without error, ignoring activate/deactivate") {
        CHECK (ok)
          ;
        CHECK (out.str().find("Request") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("an empty sequence diagram") {
    std::string input = "sequenceDiagram\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("fails — no participants") {
        CHECK (!ok)
          ;
      }
    }
  }
}

// --- Pie chart renderer tests ---

SCENARIO ("Pie chart renders as horizontal bar chart") {
  GIVEN ("a pie chart with title and slices") {
    std::string input = R"(pie title Programming Languages
    "C++" : 45
    "Python" : 30
    "Rust" : 15
    "Other" : 10
)";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      std::string result = out.str();
      THEN ("title is shown") {
        CHECK (ok)
          ;
        CHECK (result.find("Programming Languages") != std::string::npos)
          ;
      }
      THEN ("all labels are shown") {
        CHECK (result.find("C++") != std::string::npos)
          ;
        CHECK (result.find("Python") != std::string::npos)
          ;
        CHECK (result.find("Rust") != std::string::npos)
          ;
        CHECK (result.find("Other") != std::string::npos)
          ;
      }
      THEN ("percentages are shown") {
        CHECK (result.find("45%") != std::string::npos)
          ;
        CHECK (result.find("30%") != std::string::npos)
          ;
        CHECK (result.find("15%") != std::string::npos)
          ;
        CHECK (result.find("10%") != std::string::npos)
          ;
      }
      THEN ("bar characters are present") {
        // █ = U+2588 = 0xE2 0x96 0x88
        CHECK (result.find("\xe2\x96\x88") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a pie chart without title") {
    std::string input = "pie\n    \"Yes\" : 70\n    \"No\" : 30\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("renders without title") {
        CHECK (ok)
          ;
        CHECK (out.str().find("70%") != std::string::npos)
          ;
        CHECK (out.str().find("30%") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a pie chart with single slice") {
    std::string input = "pie\n    \"Everything\" : 100\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("shows 100%") {
        CHECK (ok)
          ;
        CHECK (out.str().find("100%") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a pie chart with no slices") {
    std::string input = "pie title Empty\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("fails") {
        CHECK (!ok)
          ;
      }
    }
  }
  GIVEN ("a pie chart with decimal values") {
    std::string input = "pie\n    \"A\" : 33.3\n    \"B\" : 66.7\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("renders with rounded percentages") {
        CHECK (ok)
          ;
        CHECK (out.str().find("33%") != std::string::npos)
          ;
        CHECK (out.str().find("67%") != std::string::npos)
          ;
      }
    }
  }
}

// --- State diagram renderer tests ---

SCENARIO ("State diagram renders via flowchart engine") {
  GIVEN ("a simple state machine") {
    std::string input = R"(stateDiagram-v2
    [*] --> Idle
    Idle --> Running
    Running --> Done
    Done --> [*]
)";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("produces braille output") {
        CHECK (ok)
          ;
        CHECK (out.str().find("\xe2") != std::string::npos)
          ;
      }
      THEN ("state labels appear") {
        CHECK (out.str().find("Idle") != std::string::npos)
          ;
        CHECK (out.str().find("Running") != std::string::npos)
          ;
        CHECK (out.str().find("Done") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a state diagram with labeled transitions (labels ignored)") {
    std::string input = R"(stateDiagram-v2
    Idle --> Active : start
    Active --> Idle : stop
)";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("renders the states") {
        CHECK (ok)
          ;
        CHECK (out.str().find("Idle") != std::string::npos)
          ;
        CHECK (out.str().find("Active") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a state diagram with no transitions") {
    std::string input = "stateDiagram-v2\n    direction LR\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("fails — no edges") {
        CHECK (!ok)
          ;
      }
    }
  }
}

// --- StreamRenderer integration: mermaid blocks ---

SCENARIO ("StreamRenderer renders mermaid blocks inline") {
  GIVEN ("a StreamRenderer with color enabled") {
    std::ostringstream out;
    StreamRenderer sr(out, true);

    WHEN ("streaming a supported flowchart block") {
      sr.write("Here is a diagram:\n");
      sr.write("```mermaid\n");
      sr.write("graph TD\n");
      sr.write("    A[Hello] --> B[World]\n");
      sr.write("```\n");
      sr.write("Done.\n");
      sr.finish();
      std::string result = out.str();
      THEN ("diagram is rendered (braille present)") {
        CHECK (result.find("\xe2") != std::string::npos)
          ;
      }
      THEN ("node labels appear") {
        CHECK (result.find("Hello") != std::string::npos)
          ;
        CHECK (result.find("World") != std::string::npos)
          ;
      }
      THEN ("surrounding text is preserved") {
        CHECK (result.find("Here is a diagram:") != std::string::npos)
          ;
        CHECK (result.find("Done.") != std::string::npos)
          ;
      }
      THEN ("raw mermaid source is NOT shown") {
        CHECK (result.find("graph TD") == std::string::npos)
          ;
      }
    }
    WHEN ("streaming an unsupported diagram type") {
      sr.write("```mermaid\n");
      sr.write("gantt\n");
      sr.write("    title Plan\n");
      sr.write("```\n");
      sr.finish();
      std::string result = out.str();
      THEN ("fallback shows as code block") {
        CHECK (result.find("gantt") != std::string::npos)
          ;
        CHECK (result.find("```mermaid") != std::string::npos)
          ;
        CHECK (result.find("```\n") != std::string::npos)
          ;
      }
    }
    WHEN ("streaming a sequence diagram block") {
      sr.write("```mermaid\n");
      sr.write("sequenceDiagram\n");
      sr.write("    A->>B: Hello\n");
      sr.write("```\n");
      sr.finish();
      std::string result = out.str();
      THEN ("sequence diagram is rendered as ASCII") {
        CHECK (result.find("A") != std::string::npos)
          ;
        CHECK (result.find("B") != std::string::npos)
          ;
        CHECK (result.find("Hello") != std::string::npos)
          ;
        CHECK (result.find(">") != std::string::npos)
          ;
      }
      THEN ("raw mermaid syntax is NOT shown") {
        CHECK (result.find("sequenceDiagram") == std::string::npos)
          ;
        CHECK (result.find("->>") == std::string::npos)
          ;
      }
    }
    WHEN ("streaming a pie chart block") {
      sr.write("```mermaid\n");
      sr.write("pie\n");
      sr.write("    \"Yes\" : 80\n");
      sr.write("    \"No\" : 20\n");
      sr.write("```\n");
      sr.finish();
      std::string result = out.str();
      THEN ("pie chart is rendered as bar chart") {
        CHECK (result.find("Yes") != std::string::npos)
          ;
        CHECK (result.find("80%") != std::string::npos)
          ;
        CHECK (result.find("20%") != std::string::npos)
          ;
      }
    }
  }
}

// --- Venn diagram renderer tests ---

SCENARIO ("Venn diagram renders overlapping circles with legend") {
  GIVEN ("a two-set venn diagram") {
    std::string input = "venn title Skills\n    \"Frontend\" : 60\n    \"Backend\" : 40\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("title and legend appear") {
        CHECK (ok);
        CHECK (out.str().find("Skills") != std::string::npos);
        CHECK (out.str().find("Frontend") != std::string::npos);
        CHECK (out.str().find("Backend") != std::string::npos);
      }
    }
  }
  GIVEN ("a three-set venn") {
    std::string input = "venn\n    \"A\" : 30\n    \"B\" : 30\n    \"C\" : 30\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out);
      THEN ("all three sets in legend") {
        CHECK (ok);
        CHECK (out.str().find("A (30)") != std::string::npos);
        CHECK (out.str().find("B (30)") != std::string::npos);
        CHECK (out.str().find("C (30)") != std::string::npos);
      }
    }
  }
  GIVEN ("more than 3 sets") {
    std::string input = "venn\n    \"A\":1\n    \"B\":1\n    \"C\":1\n    \"D\":1\n";
    std::ostringstream out;
    WHEN ("rendered") {
      THEN ("fails — max 3 sets") { CHECK (!tui::diagram_registry().render(input, out)); }
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
        CHECK (ok);
        CHECK (out.str().find("Sprint") != std::string::npos);
        CHECK (out.str().find("Coding") != std::string::npos);
        CHECK (out.str().find("Testing") != std::string::npos);
        CHECK (out.str().find("Review") != std::string::npos);
        CHECK (out.str().find("Dev") != std::string::npos);
        CHECK (out.str().find("QA") != std::string::npos);
      }
    }
  }
  GIVEN ("an empty gantt") {
    std::ostringstream out;
    THEN ("fails") { CHECK (!tui::diagram_registry().render("gantt\n    title Empty\n", out)); }
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
        CHECK (ok);
        CHECK (result.find("Topic") != std::string::npos);
        CHECK (result.find("A") != std::string::npos);
        CHECK (result.find("A1") != std::string::npos);
        CHECK (result.find("B") != std::string::npos);
      }
    }
  }
  GIVEN ("an empty mindmap") {
    std::ostringstream out;
    THEN ("fails") { CHECK (!tui::diagram_registry().render("mindmap\n", out)); }
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
        CHECK (ok);
        CHECK (result.find("Priority") != std::string::npos);
        CHECK (result.find("Do") != std::string::npos);
        CHECK (result.find("Plan") != std::string::npos);
        CHECK (result.find("Fix bug") != std::string::npos);
        CHECK (result.find("Rewrite") != std::string::npos);
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
        CHECK (ok);
        CHECK (result.find("History") != std::string::npos);
        CHECK (result.find("2024") != std::string::npos);
        CHECK (result.find("Jan") != std::string::npos);
        CHECK (result.find("Start") != std::string::npos);
        CHECK (result.find("v1.0") != std::string::npos);
      }
    }
  }
  GIVEN ("an empty timeline") {
    std::ostringstream out;
    THEN ("fails") { CHECK (!tui::diagram_registry().render("timeline\n", out)); }
  }
}

// --- Kanban renderer tests ---

SCENARIO ("Kanban renders multi-column board") {
  GIVEN ("a kanban with columns and items") {
    std::string input = "kanban\n    title Board\n    column Todo\n      Task A\n      Task B\n    column Done\n      Task C\n";
    std::ostringstream out;
    WHEN ("rendered") {
      bool ok = tui::diagram_registry().render(input, out, 60);
      std::string result = out.str();
      THEN ("title, columns and items appear") {
        CHECK (ok);
        CHECK (result.find("Board") != std::string::npos);
        CHECK (result.find("Todo") != std::string::npos);
        CHECK (result.find("Done") != std::string::npos);
        CHECK (result.find("Task A") != std::string::npos);
        CHECK (result.find("Task C") != std::string::npos);
      }
    }
  }
  GIVEN ("an empty kanban") {
    std::ostringstream out;
    THEN ("fails") { CHECK (!tui::diagram_registry().render("kanban\n    title X\n", out)); }
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
        CHECK (ok);
        CHECK (result.find("Metrics") != std::string::npos);
        CHECK (result.find("Alpha") != std::string::npos);
        CHECK (result.find("Beta") != std::string::npos);
        CHECK (result.find("100") != std::string::npos);
        CHECK (result.find("50") != std::string::npos);
      }
    }
  }
  GIVEN ("an empty bar chart") {
    std::ostringstream out;
    THEN ("fails") { CHECK (!tui::diagram_registry().render("barchart\n    title X\n", out)); }
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
        CHECK (ok);
        CHECK (result.find("CEO") != std::string::npos);
        CHECK (result.find("CTO") != std::string::npos);
        CHECK (result.find("Dev") != std::string::npos);
        CHECK (result.find("CFO") != std::string::npos);
      }
    }
  }
  GIVEN ("an empty org chart") {
    std::ostringstream out;
    THEN ("fails") { CHECK (!tui::diagram_registry().render("orgchart\n", out)); }
  }
}
