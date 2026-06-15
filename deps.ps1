[CmdletBinding()]
param(
    [ValidateSet("install", "status", "clean")]
    [string]$Command = "install",

    [switch]$Force
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$projectRoot = $PSScriptRoot

$dependencies = @(
    @{
        Name = "raylib"
        Repository = "https://github.com/raysan5/raylib.git"
        Tag = "6.0"
        Directory = "raylib-6.0"
    },
    @{
        Name = "SDL"
        Repository = "https://github.com/libsdl-org/SDL.git"
        Tag = "release-3.4.10"
        Directory = "SDL-release-3.4.10"
    }
)

function Assert-InProject {
    param(
        [Parameter(Mandatory)]
        [string]$Path
    )

    $resolvedRoot = (Resolve-Path -LiteralPath $projectRoot).Path
    $fullPath = [System.IO.Path]::GetFullPath($Path)

    if (-not $fullPath.StartsWith($resolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to operate outside the project directory: $fullPath"
    }

    return $fullPath
}

function Find-Git {
    $git = Get-Command "git.exe" -ErrorAction SilentlyContinue
    if ($null -ne $git) {
        return $git.Source
    }

    $git = Get-Command "git" -ErrorAction SilentlyContinue
    if ($null -ne $git) {
        return $git.Source
    }

    throw "Could not find git. Install Git for Windows or add git to PATH."
}

function Install-Dependency {
    param(
        [Parameter(Mandatory)]
        [hashtable]$Dependency,

        [Parameter(Mandatory)]
        [string]$Git
    )

    $target = Assert-InProject (Join-Path $projectRoot $Dependency.Directory)

    if (Test-Path -LiteralPath $target) {
        if (-not $Force) {
            Write-Host "$($Dependency.Directory) already exists. Use -Force to replace it."
            return
        }

        Remove-Item -LiteralPath $target -Recurse -Force
    }

    $tempParent = Join-Path $projectRoot ".deps-tmp"
    $tempDirectory = Join-Path $tempParent "$($Dependency.Directory)-$([System.Guid]::NewGuid().ToString('N'))"

    New-Item -ItemType Directory -Path $tempParent -Force | Out-Null

    try {
        Write-Host "Installing $($Dependency.Name) $($Dependency.Tag) into $($Dependency.Directory)..."
        & $Git clone `
            --depth 1 `
            --branch $Dependency.Tag `
            --recurse-submodules `
            --shallow-submodules `
            $Dependency.Repository `
            $tempDirectory

        if ($LASTEXITCODE -ne 0) {
            throw "git clone failed for $($Dependency.Name) with exit code $LASTEXITCODE."
        }

        Move-Item -LiteralPath $tempDirectory -Destination $target
        Write-Host "Installed $($Dependency.Directory)." -ForegroundColor Green
    }
    finally {
        if (Test-Path -LiteralPath $tempDirectory) {
            Remove-Item -LiteralPath $tempDirectory -Recurse -Force
        }

        if ((Test-Path -LiteralPath $tempParent) -and -not (Get-ChildItem -LiteralPath $tempParent -Force)) {
            Remove-Item -LiteralPath $tempParent -Force
        }
    }
}

function Show-Status {
    foreach ($dependency in $dependencies) {
        $target = Join-Path $projectRoot $dependency.Directory
        if (Test-Path -LiteralPath $target -PathType Container) {
            Write-Host "$($dependency.Directory): installed"
        }
        else {
            Write-Host "$($dependency.Directory): missing"
        }
    }
}

function Clean-Dependencies {
    foreach ($dependency in $dependencies) {
        $target = Assert-InProject (Join-Path $projectRoot $dependency.Directory)

        if (Test-Path -LiteralPath $target) {
            Remove-Item -LiteralPath $target -Recurse -Force
            Write-Host "Removed $($dependency.Directory)."
        }
    }
}

switch ($Command) {
    "install" {
        $git = Find-Git
        foreach ($dependency in $dependencies) {
            Install-Dependency -Dependency $dependency -Git $git
        }
    }
    "status" {
        Show-Status
    }
    "clean" {
        Clean-Dependencies
    }
}
