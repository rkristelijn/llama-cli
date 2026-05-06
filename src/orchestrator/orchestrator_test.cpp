// test_orchestrator.cpp — Unit tests for metrics + prompt_template (ADR-096)

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cstdio>
#include <fstream>

#include "orchestrator/metrics.h"
#include "orchestrator/prompt_template.h"

SCENARIO ("prompt template rendering") {
  GIVEN ("a template with variables") {
    std::string tmpl = "Hello {{name}}, your role is {{role}}.";
    std::unordered_map<std::string, std::string> vars = {{"name", "plan"}, {"role", "planner"}};
    WHEN ("rendered") {
      std::string result = render_template(tmpl, vars);
      THEN ("variables are substituted") {
        CHECK (result == "Hello plan, your role is planner.")
          ;
      }
    }
  }
  GIVEN ("a template with unknown variables") {
    std::string tmpl = "{{known}} and {{unknown}}";
    std::unordered_map<std::string, std::string> vars = {{"known", "yes"}};
    WHEN ("rendered") {
      std::string result = render_template(tmpl, vars);
      THEN ("unknown keys are left as-is") {
        CHECK (result == "yes and {{unknown}}")
          ;
      }
    }
  }
  GIVEN ("a template with no variables") {
    std::string tmpl = "plain text";
    std::unordered_map<std::string, std::string> vars;
    WHEN ("rendered") {
      std::string result = render_template(tmpl, vars);
      THEN ("text is unchanged") {
        CHECK (result == "plain text")
          ;
      }
    }
  }
  GIVEN ("a template with adjacent variables") {
    std::string tmpl = "{{a}}{{b}}";
    std::unordered_map<std::string, std::string> vars = {{"a", "X"}, {"b", "Y"}};
    WHEN ("rendered") {
      CHECK (render_template(tmpl, vars) == "XY")
        ;
    }
  }
}

SCENARIO ("metrics scoring") {
  GIVEN ("no recorded metrics") {
    WHEN ("score is requested") {
      double s = metrics_score("nonexistent-model", "test");
      THEN ("score is zero") {
        CHECK (s == 0.0)
          ;
      }
    }
  }
  GIVEN ("a recorded successful call") {
    metrics_record("test-model", "localhost:11434", "test-agent", 1000, 50, 100, true);
    WHEN ("score is calculated") {
      double s = metrics_score("test-model", "test-agent");
      THEN ("score is positive") {
        CHECK (s > 0.0)
          ;
      }
    }
  }
  GIVEN ("a recorded failed call") {
    metrics_record("fail-model", "localhost:11434", "test-agent", 5000, 10, 0, false);
    WHEN ("score is calculated") {
      double s = metrics_score("fail-model", "test-agent");
      THEN ("score is zero (0% success rate)") {
        CHECK (s == 0.0)
          ;
      }
    }
  }
}

SCENARIO ("load_prompt from file") {
  GIVEN ("an existing prompt file") {
    // plan.txt was created earlier
    std::string prompt = load_prompt("plan");
    WHEN ("loaded") {
      THEN ("it contains content") {
        CHECK (prompt.find("plan") != std::string::npos)
          ;
      }
    }
  }
  GIVEN ("a non-existent prompt") {
    std::string prompt = load_prompt("nonexistent_agent_xyz");
    WHEN ("loaded") {
      THEN ("empty string returned") {
        CHECK (prompt.empty())
          ;
      }
    }
  }
}
