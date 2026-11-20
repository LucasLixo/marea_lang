<#
  .SYNOPSIS
  Initializes the Git submodules of the project, including the nested ones.

  .DESCRIPTION
  Checks out every submodule this repository pins (and, recursively, whatever submodules
  those pin) at exactly the commit it is pinned to: the script never moves a pin.

  .EXAMPLE
  PS> .\GetGitModules.ps1
#>

[String[]] $Arguments = $args

if (-not $Arguments) {
    [String[]]$Arguments = "--depth", "1"
}

# Ensure we are in the root for submodule operations
$RootDir = Resolve-Path "$PSScriptRoot\.."
Push-Location $RootDir

Write-Host "Initializing submodules (recursive) with: $Arguments" -ForegroundColor Cyan
git submodule update --init --recursive --force --jobs 8 @Arguments

Pop-Location
