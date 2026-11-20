#!/usr/bin/env bash

# .SYNOPSIS
# Moves the submodules that follow a branch to the tip of that branch, walking a limited number of .gitmodules
# layers down. Commits nothing.
#
# .DESCRIPTION
# GetGitModules.sh checks out the commits this repository pins and never moves one. This script is the
# explicit counterpart: for each .gitmodules entry whose `branch =` names a branch that exists on its remote
# (or, with no `branch =`, the remote's default branch), it runs `git submodule update --remote` and reports
# the old and new commit. An entry whose `branch =` is a tag, or does not exist on the remote, is skipped and
# named: a tag never moves. Only the submodule pointers change, so the result is a pointer bump to commit on
# its own, never together with unrelated work.
#
# Only --levels layers are walked. Below that, a submodule belongs to a third-party project that pins its own
# dependencies, so they are left at exactly the commits the project pins (synced after it moves), never
# advanced on their own.
#
# Always runs against this repository's own root (one level up from this script) - this project is never
# consumed as a submodule of another repository.
#
# .PARAMETER --levels <n>
# How many layers of .gitmodules to walk below this repository's own submodules, default 2.
#
# .PARAMETER --dry-run
# Only report what would move: compares each pinned commit with the remote tip and changes nothing.
#
# .EXAMPLE
# tools/UpdateGitModules.sh --dry-run
# tools/UpdateGitModules.sh
# tools/UpdateGitModules.sh --levels 3

set -u

BLUE='\033[0;34m'
YELLOW='\033[0;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m'

# macOS ships without GNU coreutils realpath; provide a portable fallback.
if ! command -v realpath >/dev/null 2>&1; then
    realpath() {
        local p="$1"
        [ -d "$p" ] && { (cd "$p" && pwd -P); return; }
        (cd "$(dirname "$p")" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$p")")
    }
fi

ROOT_DIR=$(realpath "$(dirname "$0")/..")
DRY_RUN=0
LEVELS=2

while [ $# -gt 0 ]; do
    case "$1" in
        --levels)
            [ $# -ge 2 ] && [ "$2" -ge 1 ] 2>/dev/null || { echo "--levels needs a number of at least 1" >&2; exit 2; }
            LEVELS="$2"
            shift 2
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        *)
            echo "Usage: $(basename "$0") [--levels <n>] [--dry-run]" >&2
            exit 2
            ;;
    esac
done

UPDATED=0
CURRENT=0
SKIPPED=0

short() {
    printf '%.9s' "$1"
}

# @param $1 repo: absolute path of a repository whose .gitmodules to walk.
# @param $2 level: 1 for the root repository, one more for each layer down.
update_repo() {
    local repo="$1"
    local level="$2"
    local gitmodules="$repo/.gitmodules"
    [ -f "$gitmodules" ] || return 0

    local key name sub_path branch sub_dir remote_ref old new tip
    while read -r key sub_path; do
        name="${key#submodule.}"
        name="${name%.path}"
        sub_dir="$repo/$sub_path"
        branch=$(git config -f "$gitmodules" --get "submodule.$name.branch" || true)

        if [ ! -e "$sub_dir/.git" ]; then
            echo -e "${YELLOW}skip  ${sub_dir#"$ROOT_DIR"/} - not initialized, run setup:git-modules first${NC}"
            SKIPPED=$((SKIPPED + 1))
            continue
        fi

        # No branch named: the remote's default branch, exactly what `git submodule update --remote` follows
        if [ -n "$branch" ]; then
            remote_ref="refs/heads/$branch"
            if ! git -C "$sub_dir" ls-remote --exit-code --heads origin "$branch" >/dev/null 2>&1; then
                echo -e "${YELLOW}skip  ${sub_dir#"$ROOT_DIR"/} - '$branch' is not a branch on its remote (a tag never moves)${NC}"
                SKIPPED=$((SKIPPED + 1))
                continue
            fi
        else
            remote_ref="HEAD"
        fi

        old=$(git -C "$repo" rev-parse "HEAD:$sub_path" 2>/dev/null || git -C "$sub_dir" rev-parse HEAD)
        tip=$(git -C "$sub_dir" ls-remote origin "$remote_ref" | awk 'NR==1{print $1}')
        if [ -z "$tip" ]; then
            echo -e "${RED}fail  ${sub_dir#"$ROOT_DIR"/} - no answer from its remote${NC}"
            SKIPPED=$((SKIPPED + 1))
            continue
        fi

        if [ "$old" = "$tip" ]; then
            echo -e "${GREEN}ok    ${sub_dir#"$ROOT_DIR"/} - already at $(short "$tip") (${branch:-HEAD})${NC}"
            CURRENT=$((CURRENT + 1))
        elif [ "$DRY_RUN" -eq 1 ]; then
            echo -e "${CYAN}would ${sub_dir#"$ROOT_DIR"/} - $(short "$old") -> $(short "$tip") (${branch:-HEAD})${NC}"
            UPDATED=$((UPDATED + 1))
        else
            echo -e "${BLUE}move  ${sub_dir#"$ROOT_DIR"/} - $(short "$old") -> $(short "$tip") (${branch:-HEAD})${NC}"
            if git -C "$repo" submodule update --init --remote -- "$sub_path" >/dev/null 2>&1; then
                new=$(git -C "$sub_dir" rev-parse HEAD)
                UPDATED=$((UPDATED + 1))
                [ "$new" = "$tip" ] || echo -e "${YELLOW}      now at $(short "$new"), the remote moved meanwhile${NC}"
            else
                echo -e "${RED}fail  ${sub_dir#"$ROOT_DIR"/} - git submodule update --remote failed${NC}"
                SKIPPED=$((SKIPPED + 1))
                continue
            fi
        fi

        if [ "$level" -lt "$LEVELS" ]; then
            update_repo "$sub_dir" $((level + 1))
        elif [ "$DRY_RUN" -eq 0 ] && [ -f "$sub_dir/.gitmodules" ]; then
            # Past the limit: its own submodules follow what the project pins now that it may have moved
            git -C "$sub_dir" submodule update --init --recursive --force -q >/dev/null 2>&1 || true
        fi
    done < <(git config -f "$gitmodules" --get-regexp '^submodule\..*\.path$')
}

cd "$ROOT_DIR" || { echo "Failed to enter $ROOT_DIR"; exit 1; }
update_repo "$ROOT_DIR" 1

if [ "$DRY_RUN" -eq 1 ]; then
    echo "$UPDATED would move, $CURRENT already current, $SKIPPED skipped (dry run, nothing changed)"
else
    echo "$UPDATED moved, $CURRENT already current, $SKIPPED skipped"
    [ "$UPDATED" -eq 0 ] || echo "Commit the pointer bump on its own: git status shows the moved submodules."
fi
