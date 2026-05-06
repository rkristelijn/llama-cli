// test_task.cpp — Unit tests for task schema (ADR-096 Phase 1)
// Uses Given/When/Then style per ADR-008

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "orchestrator/task.h"

#include <doctest/doctest.h>

SCENARIO ("task plan parsing") {
  GIVEN ("a valid JSON task plan") {
    std::string json = R"({
      "goal": "Fix the bug in parser.cpp",
      "steps": [
        { "id": 1, "tool": "read", "action": "Read the file", "input": { "path": "src/parser.cpp" } },
        { "id": 2, "tool": "str_replace", "action": "Fix the bug", "input": { "path": "src/parser.cpp", "old": "foo", "new": "bar" } },
        { "id": 3, "tool": "exec", "action": "Run tests", "input": { "cmd": "make test-unit" } }
      ]
    })";
    WHEN ("it is parsed") {
      TaskPlan plan = parse_task_plan(json);
      THEN ("the goal is extracted") {
        CHECK (plan.goal == "Fix the bug in parser.cpp")
          ;
      }
      THEN ("all steps are parsed") {
        CHECK (plan.steps.size() == 3)
          ;
      }
      THEN ("step tools are correct") {
        CHECK (plan.steps[0].tool == Tool::Read)
          ;
        CHECK (plan.steps[1].tool == Tool::StrReplace)
          ;
        CHECK (plan.steps[2].tool == Tool::Exec)
          ;
      }
      THEN ("step actions are extracted") {
        CHECK (plan.steps[0].action == "Read the file")
          ;
      }
      THEN ("step inputs are parsed") {
        CHECK (plan.steps[0].input.size() == 1)
          ;
        CHECK (plan.steps[0].input[0].first == "path")
          ;
        CHECK (plan.steps[0].input[0].second == "src/parser.cpp")
          ;
        CHECK (plan.steps[2].input[0].first == "cmd")
          ;
        CHECK (plan.steps[2].input[0].second == "make test-unit")
          ;
      }
      THEN ("str_replace has old and new") {
        CHECK (plan.steps[1].input.size() == 3)
          ;  // path, old, new
      }
    }
  }

  GIVEN ("invalid JSON") {
    std::string json = "not json at all";
    WHEN ("it is parsed") {
      TaskPlan plan = parse_task_plan(json);
      THEN ("an empty plan is returned") {
        CHECK (plan.goal.empty())
          ;
        CHECK (plan.steps.empty())
          ;
      }
    }
  }

  GIVEN ("JSON with no steps array") {
    std::string json = R"({"goal": "something"})";
    WHEN ("it is parsed") {
      TaskPlan plan = parse_task_plan(json);
      THEN ("goal is extracted but steps are empty") {
        CHECK (plan.goal == "something")
          ;
        CHECK (plan.steps.empty())
          ;
      }
    }
  }
}

SCENARIO ("task plan serialization") {
  GIVEN ("a task plan with steps") {
    TaskPlan plan;
    plan.goal = "Deploy the app";
    plan.status = Status::Running;
    TaskStep s1;
    s1.id = 1;
    s1.tool = Tool::Exec;
    s1.action = "Build";
    s1.status = Status::Completed;
    s1.input = {{"cmd", "make build"}};
    plan.steps.push_back(s1);

    WHEN ("it is serialized") {
      std::string json = serialize_task_plan(plan);
      THEN ("it contains the goal") {
        CHECK (json.find("Deploy the app") != std::string::npos)
          ;
      }
      THEN ("it contains the status") {
        CHECK (json.find("running") != std::string::npos)
          ;
      }
      THEN ("it contains the step") {
        CHECK (json.find("\"tool\":\"exec\"") != std::string::npos)
          ;
      }
      THEN ("it contains step status") {
        CHECK (json.find("\"status\":\"completed\"") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("tool and status conversion") {
  GIVEN ("tool enum values") {
    THEN ("they convert to strings") {
      CHECK (tool_to_string(Tool::Read) == "read")
        ;
      CHECK (tool_to_string(Tool::Write) == "write")
        ;
      CHECK (tool_to_string(Tool::StrReplace) == "str_replace")
        ;
      CHECK (tool_to_string(Tool::Exec) == "exec")
        ;
      CHECK (tool_to_string(Tool::Git) == "git")
        ;
    }
    THEN ("strings convert back") {
      CHECK (string_to_tool("read") == Tool::Read)
        ;
      CHECK (string_to_tool("write") == Tool::Write)
        ;
      CHECK (string_to_tool("str_replace") == Tool::StrReplace)
        ;
      CHECK (string_to_tool("exec") == Tool::Exec)
        ;
      CHECK (string_to_tool("git") == Tool::Git)
        ;
      CHECK (string_to_tool("unknown") == Tool::Read)
        ;  // fallback
    }
  }
  GIVEN ("status enum values") {
    THEN ("they convert to strings") {
      CHECK (status_to_string(Status::Pending) == "pending")
        ;
      CHECK (status_to_string(Status::Running) == "running")
        ;
      CHECK (status_to_string(Status::Completed) == "completed")
        ;
      CHECK (status_to_string(Status::Failed) == "failed")
        ;
      CHECK (status_to_string(Status::Skipped) == "skipped")
        ;
    }
  }
}
