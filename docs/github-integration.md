Work seamlessly with GitHub from the command line.

USAGE
  gh <command> <subcommand> [flags]

CORE COMMANDS
  auth:          Authenticate gh and git with GitHub
  browse:        Open repositories, issues, pull requests, and more in the browser
  codespace:     Connect to and manage codespaces
  gist:          Manage gists
  issue:         Manage issues
  org:           Manage organizations
  pr:            Manage pull requests
  project:       Work with GitHub Projects.
  release:       Manage releases
  repo:          Manage repositories

GITHUB ACTIONS COMMANDS
  cache:         Manage GitHub Actions caches
  run:           View details about workflow runs
  workflow:      View details about GitHub Actions workflows

ALIAS COMMANDS
  co:            Alias for "pr checkout"

ADDITIONAL COMMANDS
  agent-task:    Work with agent tasks (preview)
  alias:         Create command shortcuts
  api:           Make an authenticated GitHub API request
  attestation:   Work with artifact attestations
  completion:    Generate shell completion scripts
  config:        Manage configuration for gh
  extension:     Manage gh extensions
  gpg-key:       Manage GPG keys
  label:         Manage labels
  preview:       Execute previews for gh features
  ruleset:       View info about repo rulesets
  search:        Search for repositories, issues, and pull requests
  secret:        Manage GitHub secrets
  ssh-key:       Manage SSH keys
  status:        Print information about relevant issues, pull requests, and notifications across repositories
  variable:      Manage GitHub Actions variables

HELP TOPICS
  accessibility: Learn about GitHub CLI's accessibility experiences
  actions:       Learn about working with GitHub Actions
  environment:   Environment variables that can be used with gh
  exit-codes:    Exit codes used by gh
  formatting:    Formatting options for JSON data exported from gh
  mintty:        Information about using gh with MinTTY
  reference:     A comprehensive reference of all gh commands

FLAGS
  --help      Show help for command
  --version   Show gh version

EXAMPLES
  $ gh issue create
  $ gh repo clone cli/cli
  $ gh pr checkout 321

LEARN MORE
  Use `gh <command> <subcommand> --help` for more information about a command.
  Read the manual at https://cli.github.com/manual
  Learn about exit codes using `gh help exit-codes`
  Learn about accessibility experiences using `gh help accessibility`

gh search issues
gh search issues [<query>] [flags]
Search for issues on GitHub.

The command supports constructing queries using the GitHub search syntax, using the parameter and qualifier flags, or a combination of the two.

GitHub search syntax is documented at: https://docs.github.com/search-github/searching-on-github/searching-issues-and-pull-requests

On supported GitHub hosts, advanced issue search syntax can be used in the --search query. For more information about advanced issue search, see: https://docs.github.com/en/issues/tracking-your-work-with-issues/using-issues/filtering-and-searching-issues-and-pull-requests#building-advanced-filters-for-issues

For more information on handling search queries containing a hyphen, run gh search --help.

Options
--app <string>
Filter by GitHub App author
--archived
Filter based on the repository archived state {true|false}
--assignee <string>
Filter by assignee
--author <string>
Filter by author
--closed <date>
Filter on closed at date
--commenter <user>
Filter based on comments by user
--comments <number>
Filter on number of comments
--created <date>
Filter based on created at date
--include-prs
Include pull requests in results
--interactions <number>
Filter on number of reactions and comments
--involves <user>
Filter based on involvement of user
-q, --jq <expression>
Filter JSON output using a jq expression
--json <fields>
Output JSON with the specified fields
--label <strings>
Filter on label
--language <string>
Filter based on the coding language
-L, --limit <int> (default 30)
Maximum number of results to fetch
--locked
Filter on locked conversation status
--match <strings>
Restrict search to specific field of issue: {title|body|comments}
--mentions <user>
Filter based on user mentions
--milestone <title>
Filter by milestone title
--no-assignee
Filter on missing assignee
--no-label
Filter on missing label
--no-milestone
Filter on missing milestone
--no-project
Filter on missing project
--order <string> (default "desc")
Order of results returned, ignored unless '--sort' flag is specified: {asc|desc}
--owner <strings>
Filter on repository owner
--project <owner/number>
Filter on project board owner/number
--reactions <number>
Filter on number of reactions
-R, --repo <strings>
Filter on repository
--sort <string> (default "best-match")
Sort fetched results: {comments|created|interactions|reactions|reactions-+1|reactions--1|reactions-heart|reactions-smile|reactions-tada|reactions-thinking_face|updated}
--state <string>
Filter based on state: {open|closed}
--team-mentions <string>
Filter based on team mentions
-t, --template <string>
Format JSON output using a Go template; see "gh help formatting"
--updated <date>
Filter on last updated at date
--visibility <strings>
Filter based on repository visibility: {public|private|internal}
-w, --web
Open the search query in the web browser
JSON Fields
assignees, author, authorAssociation, body, closedAt, commentsCount, createdAt, id, isLocked, isPullRequest, labels, number, repository, state, title, updatedAt, url

Examples
# Search issues matching set of keywords "readme" and "typo"
$ gh search issues readme typo

# Search issues matching phrase "broken feature"
$ gh search issues "broken feature"

# Search issues and pull requests in cli organization
$ gh search issues --include-prs --owner=cli

# Search open issues assigned to yourself
$ gh search issues --assignee=@me --state=open

# Search issues with numerous comments
$ gh search issues --comments=">100"

# Search issues without label "bug"
$ gh search issues -- -label:bug

# Search issues only from un-archived repositories (default is all repositories)
$ gh search issues --owner github --archived=false
See also
gh search
