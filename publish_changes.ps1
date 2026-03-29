param(
    [string]$RepoPath = "O:\Claude2\Q2RTX-1.8.1-GPT\github\Q2RTX-1.8.1-GPT-source",
    [string]$CommitMessage = "Update repository",
    [string]$Branch = ""
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

Write-Host "Current branch: $Branch"
Write-Host ""
Write-Host "Working tree before staging:"
git status --short

git add -A

$status = git status --porcelain
if (-not $status) {
    Write-Host ""
    Write-Host "No changes to publish."
    exit 0
}

Write-Host ""
Write-Host "Creating commit: $CommitMessage"
git commit -m $CommitMessage

Write-Host ""
Write-Host "Pushing to origin/$Branch"
git push -u origin $Branch

Write-Host ""
Write-Host "Done."
