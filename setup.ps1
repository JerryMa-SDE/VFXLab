param([string]$radcppSourceDir)

Push-Location $PSScriptRoot
Write-Host "VFXLab: Current working directory: $(Get-Location)"

$vcpkgRoot = $Env:VCPKG_ROOT

$sourceDir = $PSScriptRoot
$buildDir = Join-Path -Path $sourceDir -ChildPath "build/"
$vcpkgToolchainFile = Join-Path -Path $vcpkgRoot -ChildPath "scripts/buildsystems/vcpkg.cmake"

Write-Host "VFXLab: Build directory: $buildDir"
if (Test-Path -Path $vcpkgToolchainFile -PathType Leaf)
{
    Write-Host "VFXLab: Found vcpkg toolchain file: $vcpkgToolchainFile"
}
else
{
    Write-Host "VFXLab: Cannot find vcpkg toolchain file: $vcpkgToolchainFile"
    return
}
Write-Host "VFXLab: Generating project files:"
$command = "cmake -S `"$sourceDir`" -B `"$buildDir`" -DCMAKE_TOOLCHAIN_FILE=`"$vcpkgToolchainFile`" -DRADCPP_PATH=`"$radcppSourceDir`""
Write-Host $command
Invoke-Expression $command

Pop-Location
