#!/usr/bin/env bash
#
# next-pr.sh
#
# Usage:
#   ./next-pr.sh [owner/repo]
#
# Example:
#   ./next-pr.sh omnigres/omnigres
#   --> Outputs the next available issue/PR number
#

# Default to omnigres/omnigres if no argument is supplied
REPO="${1:-omnigres/omnigres}"


# 1. Check if `gh` is installed
if ! command -v gh &> /dev/null; then
  echo "Error: GitHub CLI (gh) is not installed or not in your PATH."
  echo "Visit https://cli.github.com/ for installation instructions."
  exit 1
fi

# 2. Check if `gh` is authenticated. If not, suggest login.
if ! gh auth status &> /dev/null; then
  echo "Error: GitHub CLI is not authenticated."
  echo "Please run: gh auth login"
  exit 1
fi

# 3. Get the highest-numbered issue or PR from the repo
#    * GitHub issues and PRs share the same numbering sequence

CURRENT_NUMBER="$(
  gh api "repos/${REPO}/issues?per_page=1&sort=created&direction=desc" \
    --jq '.[0].number' \
    2>/dev/null || echo ""
)"

# 4. Compute the next number (default to 1 if no issues/PRs exist)
if [ -z "$CURRENT_NUMBER" ] || [ "$CURRENT_NUMBER" = "null" ]; then
  NEXT_NUMBER=1
else
  NEXT_NUMBER=$((CURRENT_NUMBER + 1))
fi

# 5. Print the result
echo "$NEXT_NUMBER"