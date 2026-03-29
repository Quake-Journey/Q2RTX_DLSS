param(
    [string]$RepoPath = "O:\Claude2\Q2RTX-1.8.1-GPT\github\Q2RTX-1.8.1-GPT-source",
    [string]$Branch = "",
    [string]$CommitMessage = "Repository housekeeping: README, gitattributes, gitignore, publish script"
)

$ErrorActionPreference = "Stop"

Write-Host "Repo path: $RepoPath"

if (-not (Test-Path $RepoPath)) {
    throw "Repo path does not exist: $RepoPath"
}

$gitExe = Get-Command git -ErrorAction SilentlyContinue
if (-not $gitExe) {
    throw "git is not installed or not available in PATH."
}

Set-Location $RepoPath

if (-not (Test-Path (Join-Path $RepoPath ".git"))) {
    throw "This path is not a git repository: $RepoPath"
}

if (-not $Branch) {
    $Branch = (git branch --show-current).Trim()
}

if (-not $Branch) {
    throw "Could not determine the current branch. Specify -Branch explicitly."
}

$filesToStage = @(
    ".gitignore",
    ".gitattributes",
    "readme.md",
    "publish_changes.ps1"
)

Write-Host "Staging only the repository setup files:"
foreach ($file in $filesToStage) {
    if (-not (Test-Path (Join-Path $RepoPath $file))) {
        throw "Expected file is missing: $file"
    }
    Write-Host "  $file"
    git add -- $file
}

$staged = git diff --cached --name-only
if (-not $staged) {
    Write-Host "No staged changes found for the target files."
    exit 0
}

Write-Host ""
Write-Host "Commit message: $CommitMessage"
git commit -m $CommitMessage

Write-Host ""
Write-Host "Pushing to origin/$Branch"
git push -u origin $Branch

Write-Host ""
Write-Host "Done."
