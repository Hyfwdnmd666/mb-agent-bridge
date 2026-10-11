# publish.ps1 - push this repo to GitHub with as little ceremony as possible.
#
# Git Credential Manager is already installed (git config --system credential.helper = manager),
# so authentication happens in a browser window that opens by itself - no personal access token
# to create, copy or remember.
#
# Usage:
#     .\publish.ps1 -User your-github-username
#     .\publish.ps1 -User your-github-username -Name mb-agent-bridge -Public
#
# Create the empty repository first, at https://github.com/new
#   - Repository name: mb-agent-bridge
#   - Do NOT tick "Add a README", ".gitignore" or "license" - this repo already has commits,
#     and an initialised remote makes the first push fail with "fetch first".

param(
    [Parameter(Mandatory = $true)][string]$User,
    [string]$Name = 'mb-agent-bridge',
    [switch]$Public,
    [string]$Email
)

$ErrorActionPreference = 'Stop'
$git = 'git'
if (-not (Test-Path $git)) { $git = 'git' }

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $here
Write-Host "repo     : $here" -ForegroundColor Cyan

# 1. identity - a placeholder makes the commits look unattributed on GitHub
$mail = if ($Email) { $Email } else { "$User@users.noreply.github.com" }
& $git config user.name  $User
& $git config user.email $mail
Write-Host "identity : $User <$mail>"

# 2. remote
$url = "https://github.com/$User/$Name.git"
$existing = (& $git remote 2>$null) -join ','
if ($existing -match 'origin') {
    & $git remote set-url origin $url
    Write-Host "remote   : origin updated -> $url"
} else {
    & $git remote add origin $url
    Write-Host "remote   : origin added -> $url"
}

# 3. branch name
& $git branch -M main
Write-Host "branch   : main"

# 4. push - a browser window should open for sign-in
Write-Host ""
Write-Host "pushing... a browser window will open for GitHub sign-in." -ForegroundColor Yellow
Write-Host "(if it does not, look for a small 'Git Credential Manager' window)" -ForegroundColor Yellow
Write-Host ""
& $git push -u origin main

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "done -> https://github.com/$User/$Name" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "push failed. a common cause: the remote repository already has a commit" -ForegroundColor Red
    Write-Host "(for example a README created by the GitHub website). fix with either:" -ForegroundColor Red
    Write-Host "    git pull --rebase origin main ; then run this script again"
    Write-Host "    or delete the GitHub repo and recreate it empty"
}
