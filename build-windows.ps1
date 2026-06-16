[CmdletBinding()]
param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$projectRoot = $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build-windows-release"
$outputDirectory = Join-Path $projectRoot "dist\windows"
$clionBin = Join-Path $env:LOCALAPPDATA "Programs\CLion\bin"

function Find-Tool {
    param(
        [Parameter(Mandatory)]
        [string]$Name,

        [Parameter(Mandatory)]
        [string[]]$FallbackPaths
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }

    foreach ($path in $FallbackPaths) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            return $path
        }
    }

    throw "Could not find $Name. Install it or make sure CLion is installed in its default location."
}

$cmake = Find-Tool -Name "cmake.exe" -FallbackPaths @(
    (Join-Path $clionBin "cmake\win\x64\bin\cmake.exe")
)

$ninja = Find-Tool -Name "ninja.exe" -FallbackPaths @(
    (Join-Path $clionBin "ninja\win\x64\ninja.exe")
)

$gcc = Find-Tool -Name "gcc.exe" -FallbackPaths @(
    (Join-Path $clionBin "mingw\bin\gcc.exe")
)

$compilerBin = Split-Path -Parent $gcc
$env:PATH = "$compilerBin;$env:PATH"

if ($Clean -and (Test-Path -LiteralPath $buildDirectory)) {
    $resolvedBuild = (Resolve-Path -LiteralPath $buildDirectory).Path
    $resolvedRoot = (Resolve-Path -LiteralPath $projectRoot).Path

    if (-not $resolvedBuild.StartsWith($resolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a build directory outside the project: $resolvedBuild"
    }

    Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
}

Write-Host "Configuring Windows release build..."
$configureArguments = @(
    "-S", $projectRoot,
    "-B", $buildDirectory,
    "-G", "Ninja",
    "-DCMAKE_MAKE_PROGRAM=$ninja",
    "-DCMAKE_C_COMPILER=$gcc",
    "-DCMAKE_BUILD_TYPE=Release"
)
& $cmake @configureArguments

if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE."
}

Write-Host "Building native applications..."
& $cmake --build $buildDirectory --config Release --target touch_ray_demo webcam_preview touch_canvas touch_accuracy --parallel

if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

foreach ($name in @("touch_ray_demo", "webcam_preview", "touch_canvas", "touch_accuracy")) {
    $executable = Join-Path $buildDirectory "$name.exe"
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "Build completed but the executable was not found at $executable."
    }

    Copy-Item -LiteralPath $executable -Destination $outputDirectory -Force
    Write-Host "Release build created: $(Join-Path $outputDirectory "$name.exe")" -ForegroundColor Green
}
