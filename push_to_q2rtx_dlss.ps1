param(
    [string]$RepoPath = "O:\Claude2\Q2RTX-1.8.1-GPT\github\Q2RTX-1.8.1-GPT-source",
    [string]$RemoteUrl = "https://github.com/Quake-Journey/Q2RTX_DLSS.git",
    [string]$Branch = "main",
    [string]$CommitMessage = "Initial import of Q2RTX-1.8.1-GPT source"
)

$ErrorActionPreference = "Stop"

function Assert-PathInside {
    param(
        [string]$BasePath,
        [string]$CandidatePath
    )

    $base = [System.IO.Path]::GetFullPath($BasePath)
    $candidate = [System.IO.Path]::GetFullPath($CandidatePath)

    if (-not $candidate.StartsWith($base, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to touch path outside repo root: $candidate"
    }
}

Write-Host "Repo path: $RepoPath"

if (-not (Test-Path $RepoPath)) {
    throw "Repo path does not exist: $RepoPath"
}

$gitExe = Get-Command git -ErrorAction SilentlyContinue
if (-not $gitExe) {
    throw "git is not installed or not available in PATH."
}

Set-Location $RepoPath

$externPath = Join-Path $RepoPath "extern"
if (Test-Path $externPath) {
    Get-ChildItem $externPath -Force -Directory | ForEach-Object {
        $nestedGit = Join-Path $_.FullName ".git"
        if (Test-Path $nestedGit) {
            Assert-PathInside -BasePath $RepoPath -CandidatePath $nestedGit
            Write-Host "Removing nested git metadata: $nestedGit"
            Remove-Item $nestedGit -Recurse -Force
        }
    }
}

$gitmodulesPath = Join-Path $RepoPath ".gitmodules"
if (Test-Path $gitmodulesPath) {
    Assert-PathInside -BasePath $RepoPath -CandidatePath $gitmodulesPath
    Write-Host "Removing .gitmodules"
    Remove-Item $gitmodulesPath -Force
}

$topLevelGit = Join-Path $RepoPath ".git"
if (-not (Test-Path $topLevelGit)) {
    Write-Host "Initializing git repository"
    git init -b $Branch
}

$remoteExists = $false
try {
    $existingRemote = git remote get-url origin 2>$null
    if ($LASTEXITCODE -eq 0 -and $existingRemote) {
        $remoteExists = $true
    }
} catch {
    $remoteExists = $false
}

if ($remoteExists) {
    Write-Host "Updating origin remote"
    git remote set-url origin $RemoteUrl
} else {
    Write-Host "Adding origin remote"
    git remote add origin $RemoteUrl
}

Write-Host "Staging files"
git add .

$status = git status --porcelain
if (-not $status) {
    Write-Host "Nothing to commit."
} else {
    Write-Host "Creating commit"
    git commit -m $CommitMessage
}

Write-Host "Pushing to $RemoteUrl ($Branch)"
git push -u origin $Branch

Write-Host "Done."
