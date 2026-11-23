<#
  .SYNOPSIS
  Moves the submodules that follow a branch to the tip of that branch, walking a limited number of
  .gitmodules layers down. Commits nothing.

  .DESCRIPTION
  GetGitModules.ps1 checks out the commits this repository pins and never moves one. This script is the
  explicit counterpart: for each .gitmodules entry whose `branch =` names a branch that exists on its remote
  (or, with no `branch =`, the remote's default branch), it runs `git submodule update --remote` and reports
  the old and new commit. An entry whose `branch =` is a tag, or does not exist on the remote, is skipped and
  named: a tag never moves. Only the submodule pointers change, so the result is a pointer bump to commit on
  its own, never together with unrelated work.

  Only -Levels layers are walked. Below that, a submodule belongs to a third-party project that pins its own
  dependencies, so they are left at exactly the commits the project pins (synced after it moves), never
  advanced on their own.

  Always runs against this repository's own root (one level up from this script) - this project is never
  consumed as a submodule of another repository.

  .PARAMETER Levels
  How many layers of .gitmodules to walk below this repository's own submodules, default 2.

  .PARAMETER DryRun
  Only report what would move: compares each pinned commit with the remote tip and changes nothing.

  .EXAMPLE
  PS> .\UpdateGitModules.ps1 -DryRun
  PS> .\UpdateGitModules.ps1
  PS> .\UpdateGitModules.ps1 -Levels 3
#>

param(
    [ValidateRange(1, 10)][int]$Levels = 2,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$RootDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

$script:Updated = 0
$script:Current = 0
$script:Skipped = 0

function Get-ShortSha([string]$Sha) {
    return $Sha.Substring(0, [Math]::Min(9, $Sha.Length))
}

function Get-Relative([string]$Path) {
    return $Path.Substring([Math]::Min($RootDir.Length + 1, $Path.Length))
}

# Walks one repository's .gitmodules, then recurses into each submodule it moved or kept, down to -Levels.
function Update-Repo([string]$Repo, [int]$Level) {
    $GitModules = Join-Path $Repo ".gitmodules"
    if (-not (Test-Path $GitModules)) {
        return
    }

    $Entries = git config -f $GitModules --get-regexp '^submodule\..*\.path$'
    foreach ($Entry in $Entries) {
        if (-not $Entry) { continue }
        $Key, $SubPath = $Entry -split ' ', 2
        $Name = $Key -replace '^submodule\.', '' -replace '\.path$', ''
        $SubDir = Join-Path $Repo $SubPath
        $Rel = Get-Relative $SubDir
        $Branch = git config -f $GitModules --get "submodule.$Name.branch" 2>$null
        if (-not $Branch) { $Branch = "" }

        if (-not (Test-Path (Join-Path $SubDir ".git"))) {
            Write-Host "skip  $Rel - not initialized, run setup:git-modules first" -ForegroundColor Yellow
            $script:Skipped += 1
            continue
        }

        # No branch named: the remote's default branch, exactly what `git submodule update --remote` follows
        if ($Branch) {
            $RemoteRef = "refs/heads/$Branch"
            git -C $SubDir ls-remote --exit-code --heads origin $Branch *> $null
            if ($LASTEXITCODE -ne 0) {
                Write-Host "skip  $Rel - '$Branch' is not a branch on its remote (a tag never moves)" -ForegroundColor Yellow
                $script:Skipped += 1
                continue
            }
        } else {
            $RemoteRef = "HEAD"
        }

        $Old = git -C $Repo rev-parse "HEAD:$SubPath" 2>$null
        if ($LASTEXITCODE -ne 0) { $Old = git -C $SubDir rev-parse HEAD }
        $TipLine = git -C $SubDir ls-remote origin $RemoteRef | Select-Object -First 1
        $Tip = if ($TipLine) { ($TipLine -split '\s+')[0] } else { "" }
        if (-not $Tip) {
            Write-Host "fail  $Rel - no answer from its remote" -ForegroundColor Red
            $script:Skipped += 1
            continue
        }

        $Label = if ($Branch) { $Branch } else { "HEAD" }
        if ($Old -eq $Tip) {
            Write-Host "ok    $Rel - already at $(Get-ShortSha $Tip) ($Label)" -ForegroundColor Green
            $script:Current += 1
        } elseif ($DryRun) {
            Write-Host "would $Rel - $(Get-ShortSha $Old) -> $(Get-ShortSha $Tip) ($Label)" -ForegroundColor Cyan
            $script:Updated += 1
        } else {
            Write-Host "move  $Rel - $(Get-ShortSha $Old) -> $(Get-ShortSha $Tip) ($Label)" -ForegroundColor Blue
            git -C $Repo submodule update --init --remote -- $SubPath *> $null
            if ($LASTEXITCODE -ne 0) {
                Write-Host "fail  $Rel - git submodule update --remote failed" -ForegroundColor Red
                $script:Skipped += 1
                continue
            }
            $New = git -C $SubDir rev-parse HEAD
            $script:Updated += 1
            if ($New -ne $Tip) {
                Write-Host "      now at $(Get-ShortSha $New), the remote moved meanwhile" -ForegroundColor Yellow
            }
        }

        if ($Level -lt $Levels) {
            Update-Repo $SubDir ($Level + 1)
        } elseif (-not $DryRun -and (Test-Path (Join-Path $SubDir ".gitmodules"))) {
            # Past the limit: its own submodules follow what the project pins now that it may have moved
            git -C $SubDir submodule update --init --recursive --force -q *> $null
        }
    }
}

Push-Location $RootDir
try {
    Update-Repo $RootDir 1
} finally {
    Pop-Location
}

if ($DryRun) {
    Write-Host "$($script:Updated) would move, $($script:Current) already current, $($script:Skipped) skipped (dry run, nothing changed)"
} else {
    Write-Host "$($script:Updated) moved, $($script:Current) already current, $($script:Skipped) skipped"
    if ($script:Updated -gt 0) {
        Write-Host "Commit the pointer bump on its own: git status shows the moved submodules."
    }
}
