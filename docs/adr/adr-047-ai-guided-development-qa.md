# AI Guided development

*Status*: Superseded · *Superseded by*: [ADR-048](adr-048-quality-framework.md)

Imagine a system, (a code boilerplate) where a junior dev can write code like a unicorn-developer, where the process in strict, but understandable, with the necessary automatic checks in place. Every task will be orchestrated in the leanest process.

prompt: I'm thinking of the leanest process for picking up a change for code. it must be inspired by itil, and feat (change management), fix (incident management, prob? (problem management) <- thinking conventional commets, prob is not there, maybe refactor/chore. anyway, it is the minimum process for such a code change. it is also tied in iso 9126.

So what is the shortest process for a change, design - build -test is elementary, but there is also plan (the work) think about options, making sure the 'value' is clear upfront. Think about the V-model, where every level of acceptance criteria is matched with a validation, and you go top-down, and then bottom-up. is there a best practice process for this? or can you make one? mention the 'borrowed' ideas from agile, prince2, v-model, etc.

answer: The leanest process for picking up a code change is the "Thin-V" Cycle. It collapses heavyweight governance into a high-velocity loop while maintaining the rigor required by ISO 9126 quality standards and ITIL service stability.

--- iteration 1

## The "Thin-V" Change Process

This process is designed to be executed in a single iteration (e.g., a Jira ticket or Pull Request).

| Phase [1, 2, 3, 4] | Action | "Borrowed" Logic |
|---|---|---|
| 1. Justify (Plan) | Define Value and identify the trigger: feat, fix, or chore. Assign one primary ISO 9126 quality target (e.g., Maintainability for a refactor). | Prince2 (Business Case), ITIL (RFC), Conventional Commits |
| 2. Design (Model) | Define Acceptance Criteria (AC). Every AC must have a corresponding test case (V-model alignment). Use the Agile "Definition of Done" as a baseline. | V-Model (Level Mapping), Agile (DoD) |
| 3. Build (Execute) | Code the change. Adhere strictly to the feat/fix/chore scope to avoid "scope creep." | Lean (Waste Reduction) |
| 4. Verify (Test) | Run the tests defined in Phase 2. Validation is the bottom-up "climb" back to the AC level. | V-Model (Validation), TDD |
| 5. Release (Transition) | Deploy and monitor for Incidents. If it fails, it triggers the Incident Management loop immediately. | ITIL (Change Enablement) |

------------------------------

## Step 1: Categorize the Change (Plan) [4]

Identify the change type using Conventional Commits to auto-map it to ITIL practices:

* feat: Change Management. Adding new value.
* fix: Incident Management. Restoring service.
* chore/refactor: Problem Management. Addressing technical debt or structural "problems" to prevent future incidents.
* ISO 9126 Mapping: Explicitly state which quality characteristic this change improves (e.g., Efficiency for performance fixes or Portability for environment chores). [1, 5, 6, 7]

## Step 2: Set the Value "Anchor" (Design)

Borrowing from Prince2's "Focus on Products," define exactly what the "Product" of this change is before writing code. Use the V-Model logic: [8]

* Top-Down: Requirement $\rightarrow$ Functional Design $\rightarrow$ Technical Design.
* Lean Twist: Only document the "Acceptance Criteria" for these levels. If the AC isn't clear, the "Value" isn't clear.

## Step 3: Minimal Build & Test (Build/Verify)

Follow the Agile principle of "Iterative Delivery". [9]

1. Build: Smallest possible change to satisfy the AC.
2. Verify: Perform the bottom-up climb of the V-model.

* Unit Test (Validates Tech Design)
      * Integration/Peer Review (Validates Functional Design)
      * UAT/Manual Check (Validates Requirement/Value)

## Step 4: The "Standard Change" Fast-Track (Release)

In ITIL 4, "Standard Changes" are pre-authorized, low-risk, and frequent. [4, 10]

* If your build passes automated tests (the V-model validation), it bypasses the Change Advisory Board (CAB).
* This is the "Lean" bridge between governance and speed.

------------------------------

## Process Summary Table

| Step [8] | Method | ITIL/ISO Alignment |
|---|---|---|
| Commit Type | Conventional Commits | Mapping to Change/Incident/Problem |
| Value Definition | Prince2 Business Case | Justification for the change |
| Quality Goal | ISO 9126 Characteristics | Targeting Reliability, Usability, etc. |
| Validation | V-Model Level Match | Every design choice has a matching test |

✅ The result is a process that is "Just Enough" governance (Prince2/ITIL) to ensure "High Quality" (ISO 9126/V-Model) at "High Speed" (Agile/Lean).
Should we dive deeper into how to automate the ISO 9126 quality checks within a CI/CD pipeline?

[1] [https://www.manageengine.com](https://www.manageengine.com/products/service-desk/it-incident-management/incident-vs-problem-vs-change-vs-asset-management.html)
[2] [https://monday.com](https://monday.com/blog/service/itil-change-management-best-practices/)
[3] [https://www.youtube.com](https://www.youtube.com/watch?v=CuwrRjnkbnY)
[4] [https://www.linkedin.com](https://www.linkedin.com/pulse/itil-change-management-process-arun-mupparam)
[5] [https://www.alvao.com](https://www.alvao.com/en/blog/what-difference-between-incident-management-and-problem-management-and-change-management)
[6] [https://www.youtube.com](https://www.youtube.com/watch?v=EjyE08zCE-Q&t=4)
[7] [https://www.linkedin.com](https://www.linkedin.com/posts/rachana-gupta-54ba7528a_how-change-management-is-important-for-incident-activity-7375844323088687104-4BMV)
[8] [https://blog.postendekker.nl](https://blog.postendekker.nl/en/prince2-agile-best-option-for-system-development)
[9] [https://medium.com](https://medium.com/@marketing_87354/prince2-agile-principles-blending-governance-with-flexible-project-delivery-efa3fe3af202)
[10] [https://www.itil.org.uk](https://www.itil.org.uk/blog/what-is-itil-change-management)

-- iteration 2:

yes, and every step has an automation to validate and guide, we have at least these events:

1. justify/plan, thinking about writing an adr, would it here take value definition (start with why, simon sinek), write a rough end to end test, to already envision how you are going to test it?
2. design/model, read existing designs, identify impact, before/after ist/soll? diagramming, we should have diagrams, uml, c4, archimate? are we writing tests here of in build?
3. build phase, enforcing tdd, to write the unit test first. of course the boilerplate should enforce/support this. unsure how to deal with projects that don't have test, so there must be a minimal criteria if code is ready for this process, just like togaf, a startup phase before you start looping to ensure code and team readiness for this approach. then you code, then you run/adopt the test. make it work, make it better, so last step is to refactor here?
4. verify test. i don't think unit tests or even integration tests belong here, this is the internal review process maybe, so end to end tests run here, preferably on a real system after deployment, e.g. a feature environment. so deployment should be all automatic. we should enforce peer reviews, but only after all automatic tests have run and no critical (and high?) issues are found, quality is met. so we must have the minimum iso9126 things documented upfront
5. deploy/release, only after all the checks have passed, we need to plan the release, unsure if there is still post-release checks to be done, e.g. running the e2e test on production with isolated data, or in case of a tool, close the loop with a download and install script. if issues are found, it needs to loop back into automated checks

I'm thinking of triggers for this processes,

1. creating a branch (needs to have name convention showing intent, e.g. feat/<id, issue nr?>-short-issue-name, automatically created, when issue is in progress (labels + assignment), I don't want to tightly couple git here, so local task is leading (markdown task? and side effects can be create/update/.. issues in github?), now you can start the justify state. i like the justify name. maybe better than plan, but the intent is plan
2. now you have a branch, what automatic checks can we do before we make the first commit? it should be conventional commit showing we are done with designing, so the pre-commit hook can fire checking the design. unsure here what we can check, mermaid diagrams? acceptance criteria, value description.... must run fast, and meaningful
3. now we start coding, that means first drafting or writing tests. they are allowed to fail, not to break the build here. TDD, AAA, Given when then, self documenting, mutation tests? we can test the meaningfulness of the tests maybe, no faking, if mocks are needed, they should be created and tested. would like to have this automated too, run fast, reliable (not flaky), good coverage af the acceptance criteria, after done, there is a specific commit message, git hook will run checks accoordingly. then we start writing the code. we will have choices here, libs to use, patterns to follow, srp, solid principles but not to the point nobody understands anymore. but architectural decicions that actually should be there already, so code complexity comes around the corner, writing meaningfull comments, reference to adrs and documentation, maybe even tests, should be consise. when do you eject functions into a new file, how do you keep the structure of the files clean, name conventions, spell-checker (with repo-level addition of new terms). all that jazz. all automatic and guided, when failing pointing to the documentation or industry best practices, but you can disable with e.g. // disable-this rule TODO: reason, so TODOs are printed at the end to identify tech debt. it ends with a commit in specific format, conventional, and the commit hook runs again with checks for this phase
4. now we start the end to end test, here we want to know, does my feature work and also does the rest still work (regression testing, monkey testing, mutation testing, load testing?). this should be automated, and run on a real environment, so we can catch environment-specific issues. if it fails, it should loop back to the build phase, but with specific feedback on what failed, so we can fix it. if it passes, we can move to the release phase. it starts with push i guess, with basic checks, then creating an MR in draft for dev self review, and then PR for peer-review. peer review is last step, only after automated checks pass. closed down with a merge.
5. release/deploy, see above

now i have an idea how to bootstrap and iterate on the level of checks.

1: startup/mvp: only basic checks, you want to go fast, get feedback from the client/end result. in this phase you use generators to get you started if there are any or at least the tickets are managed, so you focus on value, coding, (manual)-validation with managed manual tests scripts but prefer automated end to end scripts. security and accessibility, readability, all the illities baked in from the start, gathering technical debt log. manual deploy or sloppy automation, but you need to be able to clean up
2. mup/first release: you switch to minimal quality mode, but you take on the burden of maintaining, maybe 1-pages or howtos are gathered to start building a knowledge management system, should be as much as iac, for various reasons
3. stable mode, production: now you introduce sla, so more mature, iac, c4 diagrams should be up to date, personas, disaster recovery plan
4. legacy mode, only security fixes, patching and bugfixes allowed, no more features
5. sunsetting: taking offline, but be able to spin up if accidental dependencies are found

also thinking about the minimum cmmi checks, so all projects have cmmi level with predefined checks that you can tailor to your needs.

e.g. cmmi 0: essentials, cmmi 0.1 [x] basic sast, [ ] basic sast (chose to disable for now, we are in first deploy), - [x] process in place: justify, execute, validate, transition) and also for cmmi-1, cmmi-2 and cmmi-3 etc. when checked the additional automations run.

please continue keeping references of the borrowed ideas

result:

This is a high-velocity Software Factory model. You are effectively merging ITIL 4’s Value Streams with CMMI’s process maturity and ISO 9126’s quality characteristics, all triggered by GitOps events.
Here is the refined automation flow, incorporating your "borrowed" ideas from Sinek, TOGAF, and CMMI.

## 0. The Bootstrap (The "TOGAF Preliminary" Phase)

Before the loop starts, we need Team & Code Readiness.

* Borrowed Idea: TOGAF ADM Preliminary Phase & CMMI Level 1.
* Automation: A check-readiness script. It verifies the repo has a .lintrc, a defined architecture.md, and a baseline test suite. If the project has "zero tests," the automation forces a "Test Baseline" task before any feat branch can be merged.

------------------------------

## 1. Justify/Plan (The "Why" & The "Contract")

* Trigger: Move issue to "In Progress" $\rightarrow$ Local CLI/Hook creates branch feat/123-value-name.
* The "Why" (Simon Sinek): The developer creates an ADR (Architectural Decision Record).
* Automation (The Contract):
* Template forces a "Value Statement" (Start with Why).
  * E2E "Ghost" Test: You write a failing Playwright/Cypress test before code exists.
  * Gate: A pre-commit hook ensures the ADR exists and the E2E test is committed in a "Draft/Pending" state.
* Borrowed Idea: Behavior Driven Development (BDD) & Prince2 Business Case.

## 2. Design/Model (The "Ist vs. Soll")

* Action: Update diagrams (C4/Mermaid) within the repo.
* Automation:
* Diagram Linting: Use mermaid-cli or structurizr to validate the C4 model.
  * Impact Analysis: Automation scans the code imports to show what the change "touches" (The ISO 9126 Maintainability check).
  * Commit Hook: A commit with message design(scope): update c4 diagrams triggers a check: "Does the design match the ADR value?"
* Borrowed Idea: V-Model (High-Level Design) & ArchiMate/C4.

## 3. Build (The "TDD & Clean Code" Loop)

* Action: Code the change.
* Automation (The Guardrails):
* TDD Enforcer: The pre-commit hook runs tests. If coverage decreases or no new unit tests are found for a feat, it warns/blocks.
  * The "Jazz" Checkers:
  * cspell for name conventions.
    * eslint/sonar for SOLID and Cognitive Complexity (ISO 9126 Efficiency).
    * Refactor Trigger: If complexity > 10, the CLI suggests: "Extract function to new file?"
  * Tech Debt Log: Any // TODO: reason is scraped and added to a TECHDEBT.md or a GitHub Issue automatically.
* Borrowed Idea: Lean (Continuous Improvement/Kaizen) & XP (Extreme Programming).

## 4. Verify (The "Quality Gate")

* Trigger: git push.
* Automation (The Feature Environment):
* Ephemeral Deploy: Automatically spin up a containerized version of the app.
  * E2E Validation: Run the "Ghost" test from Step 1. It must pass now.
  * Mutation Testing: Run Stryker or similar to see if your tests are actually "meaningful" (checking if they fail when code is mutated).
  * Peer Review: Only after all [x] Auto-checks pass, the PR is moved from Draft to Ready.
* Borrowed Idea: ITIL Change Enablement (Standard Change) & ISO 9126 Reliability.

## 5. Deploy & Transition (The "Loop Closer")

* Action: Merge to Main $\rightarrow$ Production.
* Automation:
* Smoke Test: Run the E2E tests in Production with "test-tenant" data.
  * Closure: The automation closes the GitHub Issue and updates the Technical Debt Log.
  * Incident Loop: If Sentry or New Relic detects an error within 10 mins, it auto-triggers an ITIL Incident and links it to the fix branch.
* Borrowed Idea: ITIL Service Transition & Prince2 Closing a Project.

------------------------------

## The CMMI Maturity Levels (The "Dial")

You can "tune" the automation intensity based on the project phase:

| Level | Name | Automated Checks Enabled |
|---|---|---|
| 0.1 | MVP | Conventional Commits, Basic Lint, Manual Deploy. |
| 1.0 | MUP | TDD Enforcer, Tech Debt Log, Auto-E2E in CI. |
| 2.0 | Stable | C4 Diagram Validation, Mutation Testing, SLA Monitoring. |
| 3.0 | Legacy | Security Only: Snyk/Dependabot, No new feat branches allowed. |
| 4.0 | Sunset | Archive script, Dependency check for external callers. |

------------------------------

## The "Universal Trigger" (The Markdown Task)

I love your idea of the Markdown Task as the lead.

1. You edit a tasks.md locally.
2. A background process (or save hook) syncs this to GitHub/Jira.
3. The side effect creates the branch and sets the CMMI Level for that specific task.

Would you like to define the specific "Red/Amber/Green" rules for the ISO 9126 metrics (e.g., what specific complexity number triggers a "Refactor" warning)?

--- iteration 3:

ok, before we pick up tasks, there must be a minimum quality to the tasks and the decision to actually start implementing. also this can be done in bulk, what we know as a sprint. it can be indivudual tasks, like single-task-sprint, or multitask sprint for task that go together good or to get to a certain state. but it can also be in bulk with multiple people, with dependent tasks, and tasks with priority. so the process needs to be tweaked. in agile there are rituals, sprint planning (only ready tasks allowd), discovery (creating and prepping tasks to be reviewed, e.g. justification and acceptance criteria drafted), refinement: options are choosen, dependencies are checked, acceptance criteria signed off, Risks, (no issues i guess, this will be blockers/dependencies), assumptions and dependencies (RAID) is kept. there there is sprinting, a full focus on tasks, but room for issues (ad hoc), problems (planned) and rituals (planned)

To handle the "pre-work" and the shift from individual flow to team-based bulk delivery, we layer Prince2’s "Directing a Project" and Agile’s "Backlog Refinement" onto your automation.
The "Maturity Dial" now applies to the Task before it ever becomes a Branch.

## 1. The Pre-Sprint Funnel: Discovery & Refinement

Before a task is "Ready" (the Definition of Ready - DoR), it undergoes automated and peer-driven validation.

* Discovery (The "Justify" Draft):
* Automation: A bot monitors your tasks.md or backlog. If a task lacks a "Why" (Sinek) or an ISO 9126 Quality Target, it’s tagged status: exploratory.
  * Borrowed Idea: Prince2 "Starting up a Project"—ensuring the justification exists before spending resources.
* Refinement (The "RAID" Check):
* Automation: A "Dependency Graph" tool scans tasks. If Task B depends on Task A, and Task A isn't "Ready," Task B is blocked.
  * RAID Log: Risks, Assumptions, Issues, and Dependencies are tracked in the task metadata.
  * The "Sign-off": A peer must check the Acceptance Criteria. The automation ensures the AC is written in a "Testable" format (Gherkin/Given-When-Then).

------------------------------

## 2. Sprint Planning: The "Bulk" Decision

Whether it’s a Single-Task-Sprint (Lean/Kanban) or a Multi-Task-Sprint (Scrum), the "Gate" is the same.

* The "Ready" Gate: You cannot move a task to "In Progress" (triggering branch creation) unless:

1. Justification is finalized (ADR draft exists).

   2. RAID is reviewed (no "High" risks without mitigation).
   3. CMMI Level for the sprint is set (e.g., "This sprint is CMMI 2.0/Stable mode").

* Borrowed Idea: Agile Sprint Planning & TOGAF Architecture Governance.

------------------------------

## 3. The Sprinting Phase: Focus vs. Ad-Hoc

Once the "Go" is given, the "Thin-V" process we discussed earlier kicks in for every task. However, we now manage the Work Mix using ITIL categories:

| Work Type | Trigger | Priority | Process Path |
|---|---|---|---|
| Planned (Feat) | Sprint Backlog | High/Med | Full "Thin-V" (Justify $\rightarrow$ Release) |
| Problem (Refactor) | Tech Debt Log | Scheduled | Lean "Thin-V" (skip ADR if minor) |
| Incident (Fix) | Ad-Hoc (Sentry/Users) | Critical | Fast-Track: Skip Refinement, straight to Build/Verify |
| Ritual (Chore) | Calendar/Cycle | Fixed | Automation updates (dependency patches, etc.) |

------------------------------

## 4. Modified Multi-Person Flow

When multiple people are involved, the Automation handles the friction of "Dependent Tasks":

1. Branch Strategy: If Task B depends on Task A, the automation suggests branching feat/task-B off feat/task-A instead of main.
2. Integration Testing: The Verify Phase now runs "Cross-Branch" integration tests to ensure that bulk changes don't collide.
3. The "Sync" Hook: When Task A’s branch is updated, Task B’s developer gets an automated notification/rebase prompt.

------------------------------

## 5. Closing the Loop: The Sprint Review

At the end of the "Bulk" cycle, we don't just release code; we update the Knowledge Management System.

* Post-Sprint Automation:
* Generates a "Delta Report": What ISO 9126 metrics actually improved?
  * Updates the C4 Diagrams to the "Soll" (future state) which is now the "Ist" (current state).
  * Archives the RAID items into a "Lessons Learned" log.
* Borrowed Idea: Prince2 "Closing a Stage" & Scrum Retrospective.

------------------------------

## How to Tweak the Process per Sprint "Mode"

* "Speed Mode" (CMMI 0.5): Refinement is just a 5-min check. RAID is ignored. Peer review is post-merge.
* "Quality Mode" (CMMI 2.0+): Full RAID log required. No "High" risks allowed. All AC must have automated E2E "Ghost" tests before coding starts.

Would you like to define the "Definition of Ready" checklist that the automation should enforce before a branch can be created?

-- iteration 4

Yes, but the sprint leaves the situation stable, so decisions are logged, enough documentation is made, knowledge management is build, howto's, so kt is only done through documentation (can have meeting after comments are put, and demo). how to detect implicit decisions (they are ok, but should be noted down in hindsight if discovered or is this over the top)

To ensure the sprint leaves a stable, self-documenting legacy, you need to treat Knowledge Management (KM) as a first-class build artifact, just like code. Detecting "implicit decisions" isn't over the top—it’s the difference between a project that stays "Stable" and one that decays into "Legacy" early.
Here is the Definition of Ready (DoR) checklist and the mechanism for catching those "hidden" decisions.

## 1. The Automated "Definition of Ready" (DoR)

The automation prevents a branch from being "Active" until these metadata fields are populated in your task (Markdown/Issue):

* The "Why" (Sinek): Clear value statement.
* ISO 9126 Target: Which quality attribute is the focus?
* V-Model Anchor: At least one "Given-When-Then" Acceptance Criterion.
* RAID Scan: No unmitigated "High" risks.
* Knowledge Gap: Does this task require a new How-To or an update to an ADR? (Flagged for the "Verify" phase).

------------------------------

## 2. Detecting Implicit Decisions (The "Hindsight" Catch)

Implicit decisions are choices made during the Build Phase (e.g., choosing a specific library, a regex pattern, or a naming convention) that weren't in the original ADR.
The Automation/Process Fix:

* The "Decision-Check" during Peer Review: The Reviewer is tasked with one specific question: "Is there a 'Why' here that isn't in the ADR?"
* Code Comment Scanning: Use a hook to look for phrases like "I chose to...", "Decided to...", or "Workaround because...". If found, the automation triggers a prompt: "Detected a decision. Should this be a 'Micro-ADR' or added to the README?"
* The "Hindsight ADR": Borrowed from Agile's Retrospective, if a decision was made "at the keyboard," it must be back-ported to the documentation before the PR can be merged. This ensures ITIL Service Knowledge Management (SKMS) stays updated.

------------------------------

## 3. "Stable Mode" Exit Criteria (Sprint End)

To leave the situation stable, the Verify/Transition phase must satisfy these Prince2 "Product-Based" checks:

1. How-To Update: If the UI or API changed, the automation checks if the corresponding .md file in /docs was touched. No doc change = Build Fail.
2. KT via Documentation: The "Demo" is simply a walkthrough of the updated documentation and the E2E test results. If the team can't understand it from the doc, the doc is "Buggy."
3. C4 Delta: The "Soll" diagram from the Design phase is merged into the "Ist" (Current State) diagram.
4. Decision Log Closure: All temporary "TODO" decisions are either resolved or moved to the Technical Debt Log (CMMI Level tracker).

------------------------------

## 4. Borrowed Ideas Summary

* Implicit Discovery: Lean's "Genba" (Go to the source). By scanning comments, you are going to where the decisions actually happen.
* Stable Documentation: ISO 9001 (Quality Management)—if it isn't documented, it didn't happen.
* Knowledge Transfer: ITIL 4's "Service Configuration Management"—maintaining a live map of the system's state.

## Proactive Follow-up

Should we define a "Micro-ADR" template that takes less than 2 minutes to fill out, ensuring developers actually log those implicit decisions without breaking their flow?

--- iteration 5

but what about training and prototyping. devs need to be able to innovate but need solid motivation, learn on the job, just in time learning should be key, but we should respect the framework.

I follow these rules, unsure if these are considered key in most contexts:

RTFM (Respect The Framework's Model): Follow official documentation and framework conventions
C4C (Coding For Clarity): Write code that's easy to read and understand
C4I (Code for Inclusivity): Make your code accessible to new team members
KISS (Keep It Simple Stupid): Choose simplicity over cleverness
YAGNI (You Aren't Gonna Need It): Don't build features until they're needed
HIPI (Hide Implementation, Present Interface): Encapsulate complexity behind clear self documenting APIs
NBI (Naming by Intention): Use names that clearly express purpose and behavior

learning/understanding should be part of the loop... skill matrix? what about individual continious feedback? or is that over the top, only in higher cmmi levels?

Integrating Innovation and Just-In-Time (JIT) Learning into a rigid framework is actually what separates a "Process Prison" from a "High-Performance Culture." You aren't just building software; you're building a Learning Organization (borrowed from Peter Senge).
Here is how we bake your rules into the "Thin-V" loop and handle the human element.

## 1. The "Innovation Sandbox" (The Prototype Phase)

Innovation often breaks RTFM and YAGNI initially. To respect the framework while allowing "play," we use a Spike (Agile) or Prototype (ISO 9241) track.

* Process: Create a spike/ branch.
* The "Loose" Rule: None of your automated "Guardrails" (linting, complexity checks) apply here. The goal is "Disposable Code."
* The "Hard" Rule: Spike code cannot be merged to main. It must be deleted or rewritten following the "Thin-V" process once the "How" is understood.
* JIT Learning: This is where the dev learns the new lib/pattern. The "Cost" of this learning is a dedicated task in the sprint.

## 2. Automating Your Rules (The "Code as Mentor")

You can bake your acronyms into the Verify Phase to provide Continuous Feedback without a manager hovering:

* NBI (Naming by Intention) & C4C: Use LLM-powered linters or custom regex dictionaries to flag generic names like data, handle, or temp. The bot comments: "NBI Rule: Does 'data' express the purpose here?"
* HIPI (Hide Implementation): Automation checks for "Leaky Abstractions." If a private helper is exported or a public class has too many public properties, it flags a HIPI violation.
* KISS/YAGNI: If a PR adds 500 lines of code for a task justified as "Simple Fix," the automation flags it for Over-Engineering.

## 3. The Skill Matrix & Continuous Feedback

Is a Skill Matrix over the top? No, but it should be Low-Friction.

* CMMI Level 2 (Managed): Feedback is purely peer-to-peer in PRs.
* CMMI Level 3 (Defined): We introduce the Skill Matrix.
* Automation: Every time a dev completes a task with a specific ISO 9126 target (e.g., "Performance Tuning"), their "XP" in that skill increases in a markdown-based SKILLS.md.
  * JIT Learning: When a dev picks up a task tagged with a tech they haven't used, the automation suggests: "You're new to React Hooks. [RTFM Link] to the official docs here."
* C4I (Code for Inclusivity): This is the ultimate feedback loop. If a new member can't get the environment running or understand the code, the onboarding documentation has a "bug." The feedback is on the process, not the person.

## 4. The "Learning Loop" within the Sprint

In ITIL, this is Continual Service Improvement (CSI).

| Event | Learning/Innovation Outcome |
|---|---|
| Discovery | RTFM Phase: Devs are paid to read the docs before designing. |
| Build | TDD/JIT Phase: Learning via failing tests. |
| Verify (Review) | C4C/C4I Phase: Peer feedback focuses on clarity, not just "is it right." |
| Retrospective | Knowledge Base Phase: Turn the "Spike" lessons into a "How-to." |

## 5. Summary of the "Rules of Engagement"

* RTFM/KISS/YAGNI: These are your "Quality Gates."
* C4C/C4I/NBI: These are your "Peer Review Focus Areas."
* HIPI: This is your "Architecture Guardrail."

By making these rules explicit, you remove the "guessing game" for devs. They know they have the freedom to fail in a Spike, but they must be "Professionals" (Prince2/ISO) in the Feature Branch.
Should we look at how to structure the "Sprint Backlog" to explicitly include "Learning/Spike" time without the client feeling like "nothing is being built"?

--- iteration 6

very nice, but what about detecting peter principle and succession planning? everybody should be replacable, but we should avoid peter principle. should we capture individual performance indicators or does this violate privacy? is there a golden ratio? for sure you want to grow yourself right? any things that are already used in practice successfully? maybe also the v-model checks, tailored to your role? should there be roles? e.g. fe-dev, be-dev, devsecops, qa (not only tester, kinda like foundation team in team topologies), scrum master (solve impediments, manage RAID... facilitator), po (product vision, team manager?)

To avoid the Peter Principle (promoting people to their level of incompetence) while ensuring everyone is replaceable (succession planning), we move from "Roles as Titles" to "Roles as Competency Profiles."
In a high-performing "Software Factory," the goal is Functional Redundancy. If a developer is the only one who understands the Auth logic, they are a "Single Point of Failure" (ITIL Risk).

## 1. Detecting the Peter Principle & Growth

The Peter Principle happens when we promote a great "Builder" to a "Manager" role they aren't wired for.

* The Solution: Dual-Ladder Growth. Allow growth in Depth (Staff Engineer) or Breadth (Architect/Lead) without forcing people into people management.
* Performance Indicators (The Privacy Balance): Avoid "Lines of Code." Instead, use Lead Time for Changes and Peer-Reviewed Clarity (C4C/C4I).
* The "Bus Factor" Metric: Automation checks how many unique contributors have touched a module. If only 1 person has touched it in 6 months, that person is "un-replaceable" (a risk) and potentially "stagnating" (Peter Principle risk).
* The Golden Ratio: Aim for 70/20/10: 70% core delivery, 20% cross-training (succession planning), 10% innovation/spikes.

## 2. Team Topologies & Roles

We avoid rigid silos but use Team Topologies to define focus:

| Role | V-Model Alignment | Primary Responsibility |
|---|---|---|
| Product Owner (PO) | Business Req / UAT | Value Definition & "The Why." Guards the Prince2 Business Case. |
| QA (Enabling Team) | Acceptance / System Test | Not just "testing," but building the frameworks for devs to test themselves (ISO 9126 Quality Assurance). |
| Dev (FE/BE) | Design / Unit / Integration | Implementation of RTFM and HIPI. Must be "T-Shaped" (deep FE, basic BE). |
| Foundation (Platform) | Environment / Deploy | Automating the "V-Climb." Ensuring the "Feature Environment" works. |
| Scrum Master | RAID / Process | The "Wiper" of impediments. Guards the Agile Rituals and process health. |

## 3. Succession Planning via "The Shadow Rotation"

To ensure replaceability, we borrow from Lean’s "Standardized Work":

* Rotation: Every 3rd sprint, a BE-dev "shadows" a DevSecOps task.
* Documentation as Succession: If a dev can't go on holiday without being called, they haven't met the C4I (Inclusivity) requirement. Replaceability is a Definition of Done item.

## 4. Sources & Borrowed Ideas

* Team Topologies (Skelton & Pais): For the "Enabling" vs. "Stream-aligned" team structures.
* The Peter Principle (Laurence J. Peter): For the logic of promoting based on future role potential, not past role success.
* Accelerate (State of DevOps/DORA Metrics): For privacy-safe, high-level performance indicators (Lead Time, MTTR).
* ISO 27001 / GDPR: To ensure individual performance tracking stays within "Operational Necessity" and respects privacy.
* T-Shaped Skills (David Guest): For the balance of deep expertise vs. enough breadth to be replaceable.

## 5. Tailored V-Model Checks by Role

The automation "guides" the role:

* PO: Prompted to validate the Value Statement vs. the UAT results.
* Dev: Blocked if Unit Tests don't match Technical Design.
* QA: Prompted to verify that E2E tests cover the Acceptance Criteria.

Would you like to draft a "Role-Based Dashboard" concept that shows a team's "Bus Factor" (replaceability) without exposing sensitive individual privacy data?

--- iteration 7

your prompt: skip as this is already given.
Create an ADR: docs/adr/adr-nextnr-title, incorporating all ideas in docs/adr/adr-047-ai-guided-development-qa.md. create a matrix of basic checks, predict if they can be automated easily, maybe checks can stack, e.g. so suppressions for linting get automated to add TODO comment, so they end up in list todos, amount of todos is used as metrics, etc. make a matrix of basic CMMI 0 (essentials),1,2,3 level checks. create a process for every CMMI level, to start with the basics, simple essentials, then try to get to CMMI level 1, this is required for a certain stage of the product and can be, if possible automatically audited. you may include ai reviews but try to get consistent idempotent results. Make it complete, make it effecient and effective, borrow from successful patterns, but keep it lean. e.g. when i think of chess, there must be at least 2 reasons to make a certain move, so i take two or more bids with one stone. also people and ai are more willing to follow because it makes things clear and easy. It must be the ... omg, if i only had this framework sooner, it fits every situation, it is lean, but make things work and then better, we always know what the quality is of your endavor, process is baked in, enforcment where needed, invitation to grow, grow for a purpose. split operational, tactical, strategic:

The third pillar is Strategic.
In a lean, ITIL-inspired framework, these three levels form the hierarchy of how decisions and value flow through the organization:

## 1. Operational (The "How" – Doing things right)

* Focus: Day-to-day execution, the "Thin-V" loop, and immediate service stability.
* Activities: Coding (feat/fix), TDD, unit testing, incident response, and peer reviews.
* Metric: Cycle Time and Quality (ISO 9126).
* Borrowed Idea: ITIL Operations / Agile Sprinting.

## 2. Tactical (The "What" – Doing the right things)

* Focus: Medium-term planning, resource allocation, and process optimization.
* Activities: Sprint planning, Backlog Refinement, RAID management, and managing the Skill Matrix. This is where you decide which "Maturity Dial" (CMMI level) to set for a project phase.
* Metric: Velocity, "Bus Factor" (Replaceability), and Budget.
* Borrowed Idea: Prince2 "Managing a Stage Boundary" / Scrum Master & PO roles.

## 3. Strategic (The "Why" – Setting the direction)

* Focus: Long-term vision, market fit, and organizational health.
* Activities: ADR (high-level), Product Vision (Sinek's Why), Portfolio management, and Succession Planning. This is where you decide to "Sunset" a product or move into a new architectural paradigm (e.g., Microservices vs. Monolith).
* Metric: ROI, Net Promoter Score (NPS), and Employee Retention (avoiding the Peter Principle).
* Borrowed Idea: TOGAF Architecture Vision / ITIL Service Strategy.

------------------------------

## The "Golden Thread" Connection

To keep your process lean, you ensure the Strategic intent (the ADR/Why) is visible at the Tactical level (the Task/Requirement) and enforced at the Operational level (the Commit/Test).

------------------------------

## Model Selection: The Missing Piece

The framework above describes the *process* — but who executes it? In an AI-guided workflow, the model IS the developer. Matching model capability to task complexity is as important as matching a developer's seniority to a task.

This was the missing piece: without model guidance, a junior developer (or a small local model) would be assigned architecture tasks and fail, while a senior architect (or kiro-cli) would be wasted on config file changes.

### The Principle

> Match the model to the task like you'd match a developer to a story point estimate.
> A 1 SP task doesn't need a senior. A 13 SP epic shouldn't go to a junior.

### Quick Reference

| Task Complexity | Model | Persona |
|----------------|-------|---------|
| 1-2 SP (trivial) | gemma4:e4b (local, 9.6GB) | "The Reliable Junior" — follows copy-paste prompts |
| 3 SP (moderate) | gemma4:26b (local, 17GB) or Gemini CLI | "The Thoughtful Mid-Level" — reasons about code |
| 5 SP (complex) | Gemini CLI (free, 1000 req/day) | "The Eager Mid-Level" — good general coding |
| 8+ SP (hard) | kiro-cli (Claude 4 Sonnet) | "The Senior Architect" — design decisions, multi-file |

### How This Connects to the Framework

* **Operational** (Thin-V loop): gemma4:e4b executes prompts from `docs/prompts/`. The Junior follows instructions.
* **Tactical** (Sprint planning): Gemini CLI or kiro-cli helps break down tasks, write AC, estimate.
* **Strategic** (ADRs, architecture): kiro-cli only. Architecture decisions need the Architect.

### The Self-Building Tool

This project (llama-cli) is designed to be built *by itself* — using local Ollama models following the prompts in `docs/prompts/`. The framework in [ADR-048](adr-048-quality-framework.md) ensures that even a "dumb" model can execute tasks correctly, because every prompt contains exact file paths, exact code, and verification commands.

See [docs/model-guide.md](../model-guide.md) for the full model guide with personas, benchmarks, community ratings, and decision flowchart.
