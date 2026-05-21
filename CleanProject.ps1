# CombinedProjectCleanAndWipeDates_v5.ps1
# Uses the script's own directory as the project root.
#
# What it does:
# 1. Finds all C++ project folders dynamically by locating *.vcxproj files.
# 2. Removes common build output / cache folders from each project folder.
# 3. Removes common solution-level build output / cache folders.
# 4. Sets timestamps on remaining source/project files to Jan 1, 2007.
# 5. Sets timestamps on all remaining files and directories to Jan 1, 2007.
#
# This version is updated for the current VectorKernelSDK layout where example
# projects live under .\Examples\ExampleXXXX_* and app projects may live under
# .\Applications\*.

$root = $PSScriptRoot
$customDateTime = Get-Date -Year 2007 -Month 1 -Day 1 -Hour 0 -Minute 0 -Second 0

function Remove-Folder {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RelativePath
    )

    $path = Join-Path $root $RelativePath
    if (Test-Path $path) {
        Write-Host "Deleting $path"
        Remove-Item $path -Recurse -Force -ErrorAction SilentlyContinue
    }
    else {
        Write-Host "Not found $path"
    }
}

function Get-MatchingFilesRecursive {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BasePath,

        [Parameter(Mandatory = $true)]
        [string[]]$Patterns
    )

    $allFiles = @()

    foreach ($pattern in $Patterns) {
        $files = Get-ChildItem -Path $BasePath -Recurse -Filter $pattern -File -Force -ErrorAction SilentlyContinue
        if ($files) {
            $allFiles += $files
        }
    }

    return $allFiles | Sort-Object FullName -Unique
}

function Set-ItemTimes {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.FileSystemInfo]$Item,

        [Parameter(Mandatory = $true)]
        [datetime]$Timestamp
    )

    if ($Item.PSIsContainer) {
        $dir = [System.IO.DirectoryInfo]$Item
        $dir.CreationTime   = $Timestamp
        $dir.LastWriteTime  = $Timestamp
        $dir.LastAccessTime = $Timestamp
    }
    else {
        $file = [System.IO.FileInfo]$Item
        if ($file.IsReadOnly) {
            $file.IsReadOnly = $false
        }

        $file.CreationTime   = $Timestamp
        $file.LastWriteTime  = $Timestamp
        $file.LastAccessTime = $Timestamp
    }
}

function Get-ProjectDirectories {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BasePath
    )

    $projectDirs = Get-ChildItem -Path $BasePath -Recurse -Filter *.vcxproj -File -Force -ErrorAction SilentlyContinue |
        ForEach-Object { $_.Directory.FullName } |
        Sort-Object -Unique

    return $projectDirs
}

function Get-RelativePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BasePath,

        [Parameter(Mandatory = $true)]
        [string]$FullPath
    )

    $baseUri = [System.Uri]((Resolve-Path $BasePath).Path.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar)
    $fullUri = [System.Uri](Resolve-Path $FullPath).Path
    $relativeUri = $baseUri.MakeRelativeUri($fullUri)
    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace('/', '\\')
}

Write-Host "Root: $root"
Write-Host ""
Write-Host "--- Discovering project folders dynamically ---"

$projectDirectories = Get-ProjectDirectories -BasePath $root

if (-not $projectDirectories -or $projectDirectories.Count -eq 0) {
    Write-Host "No .vcxproj files found under $root"
}
else {
    Write-Host "Found $($projectDirectories.Count) project folder(s):"
    foreach ($projectDir in $projectDirectories) {
        Write-Host "  $projectDir"
    }
}

Write-Host ""
Write-Host "--- Removing build outputs and caches from each project folder ---"

$perProjectBuildFolders = @(
    'x64',
    'Debug',
    'Release',
    '.vs'
)

foreach ($projectDir in $projectDirectories) {
    $projectRelativePath = Get-RelativePath -BasePath $root -FullPath $projectDir

    foreach ($folderName in $perProjectBuildFolders) {
        Remove-Folder (Join-Path $projectRelativePath $folderName)
    }
}

Write-Host ""
Write-Host "--- Removing common solution-level build outputs and caches ---"

$solutionFolders = @(
    'packages',
    'vcpkg_installed',
    '.vs',
    'x64',
    'Debug',
    'Release',
    'ipch',
    '.idea'
)

foreach ($folder in $solutionFolders) {
    Remove-Folder $folder
}

Write-Host ""
Write-Host "--- Touching source files recursively ---"

$patterns = @(
    '*.cpp', '*.h', '*.hpp', '*.inl',
    '*.c', '*.cc', '*.cxx',
    '*.vcxproj', '*.vcxproj.filters', '*.vcxproj.user', '*.filters', '*.props', '*.targets', '*.user',
    '*.sln', '*.slnx',
    '*.ps1', '*.md', '*.txt', '*.json', '*.natvis',
    '*.rc', '*.ico', '*.bmp', '*.png', '*.jpg', '*.jpeg', '*.cur',
    '*.vert', '*.frag', '*.glsl'
)

$files = Get-MatchingFilesRecursive -BasePath $root -Patterns $patterns

Write-Host "Found $($files.Count) matching source/project file(s)."

$touched = 0
$failed = 0

foreach ($file in $files) {
    try {
        Set-ItemTimes -Item $file -Timestamp $customDateTime
        Write-Host "Touched $($file.FullName)"
        $touched++
    }
    catch {
        Write-Host "Failed: $($file.FullName) -- $($_.Exception.Message)"
        $failed++
    }
}

Write-Host ""
Write-Host "--- Wiping dates on all remaining files and directories ---"

$allItems = Get-ChildItem -Path $root -Recurse -Force -ErrorAction SilentlyContinue |
    Sort-Object FullName -Descending

$updatedAll = 0
$failedAll = 0

foreach ($item in $allItems) {
    try {
        Set-ItemTimes -Item $item -Timestamp $customDateTime
        Write-Host "Updated: $($item.FullName)"
        $updatedAll++
    }
    catch {
        Write-Host "Failed: $($item.FullName) -- $($_.Exception.Message)"
        $failedAll++
    }
}

try {
    $rootItem = Get-Item -Path $root -Force
    Set-ItemTimes -Item $rootItem -Timestamp $customDateTime
    Write-Host "Updated root: $root"
}
catch {
    Write-Host "Failed root: $root -- $($_.Exception.Message)"
}

Write-Host ""
Write-Host "Project folders discovered    : $($projectDirectories.Count)"
Write-Host "Source/project files touched : $touched"
Write-Host "Source/project files failed  : $failed"
Write-Host "All items updated            : $updatedAll"
Write-Host "All items failed             : $failedAll"
Write-Host "Combined cleanup complete from root: $root"
