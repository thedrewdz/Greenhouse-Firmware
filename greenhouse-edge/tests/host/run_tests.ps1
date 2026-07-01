<#
.SYNOPSIS
    Build and run the Greenhouse Edge host unit tests natively with MSVC.

.DESCRIPTION
    Compiles the real firmware modules under test (src/codec_json.c,
    src/retry_backoff.c) together with vendored cJSON + Unity and the host test
    sources, then runs the resulting executable. No ESP-IDF or hardware needed.

    Requires a Visual Studio C/C++ toolchain (cl.exe). The script locates it via
    vswhere and imports the MSVC environment automatically.

.EXAMPLE
    pwsh -File tests/host/run_tests.ps1
#>
$ErrorActionPreference = 'Stop'

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$build = Join-Path $repo 'build\host'
New-Item -ItemType Directory -Force -Path $build | Out-Null

# --- Locate vcvars64.bat -------------------------------------------------
function Find-VcVars {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $install = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath 2>$null | Select-Object -First 1
        if ($install) {
            $vc = Join-Path $install 'VC\Auxiliary\Build\vcvars64.bat'
            if (Test-Path $vc) { return $vc }
        }
    }
    # Fallback: scan common install roots.
    foreach ($root in @(
            'C:\Program Files\Microsoft Visual Studio',
            'C:\Program Files (x86)\Microsoft Visual Studio')) {
        if (Test-Path $root) {
            $vc = Get-ChildItem -Path $root -Recurse -Filter 'vcvars64.bat' -ErrorAction SilentlyContinue |
                  Select-Object -First 1
            if ($vc) { return $vc.FullName }
        }
    }
    throw 'Could not find vcvars64.bat. Install the Visual Studio C++ build tools.'
}

$vcvars = Find-VcVars
Write-Host "Using MSVC env: $vcvars"

# --- Sources & include paths --------------------------------------------
$srcs = @(
    # Real firmware under test
    'src\codec_json.c',
    'src\retry_backoff.c',
    'src\services_mqtt.c',
    'src\services_network.c',
    'src\services_provisioning_config.c',
    # Vendored deps + fake ESP-IDF layer
    'tests\host\vendor\cjson\cJSON.c',
    'tests\host\vendor\unity\unity.c',
    'tests\host\fakes\fake_esp_idf.c',
    # Test suites
    'tests\host\src\test_main.c',
    'tests\host\src\test_codec_provisioning.c',
    'tests\host\src\test_codec_serialization.c',
    'tests\host\src\test_retry_backoff.c',
    'tests\host\src\test_service_mqtt.c',
    'tests\host\src\test_service_network.c',
    'tests\host\src\test_service_provisioning_nvs.c'
) | ForEach-Object { '"' + (Join-Path $repo $_) + '"' }

$includes = @(
    'src',
    'tests\host\shims',
    'tests\host\fakes',
    'tests\host\vendor\cjson',
    'tests\host\vendor\unity'
) | ForEach-Object { '/I"' + (Join-Path $repo $_) + '"' }

$exe = Join-Path $build 'gh_host_tests.exe'
$clArgs = @(
    '/nologo', '/W3', '/wd4996',
    '/D_CRT_SECURE_NO_WARNINGS', '/DCJSON_HIDE_SYMBOLS', '/DUNITY_INCLUDE_DOUBLE'
) + $includes + $srcs + @(
    ('/Fe:"' + $exe + '"'),
    ('/Fo:"' + $build + '\\"')
)

$clLine = 'cl ' + ($clArgs -join ' ')
$cmd = "`"$vcvars`" >nul && $clLine"

Write-Host 'Compiling host tests...'
& cmd.exe /c $cmd
if ($LASTEXITCODE -ne 0) { throw "Compilation failed (exit $LASTEXITCODE)." }

Write-Host "`nRunning $exe`n"
& $exe
$testExit = $LASTEXITCODE
Write-Host "`nTest process exit code: $testExit"
exit $testExit
