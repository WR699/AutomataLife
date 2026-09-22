param(
    [switch]$Elevated
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$ProjectDir = Split-Path -Parent $PSCommandPath
$BuildDir = Join-Path $ProjectDir 'build_windows_auto'
$MinimumCMake = [Version]'3.16.0'
$VcComponent = 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64'
$VcWorkload = 'Microsoft.VisualStudio.Workload.VCTools'
$Vs2022BuildToolsId = 'Microsoft.VisualStudio.2022.BuildTools'
$Vs2022BootstrapperUrl = 'https://aka.ms/vs/17/release/vs_BuildTools.exe'
$Vs2022BuildToolsPath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\2022\BuildTools'
$VsWhereDefault = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

function Write-Step([string]$Message) {
    Write-Host "`n==> $Message" -ForegroundColor Cyan
}

function Write-Ok([string]$Message) {
    Write-Host "[OK] $Message" -ForegroundColor Green
}

function Write-Warn([string]$Message) {
    Write-Host "[AVISO] $Message" -ForegroundColor Yellow
}

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Get-CMakeExe {
    $cmd = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        (Join-Path $env:ProgramFiles 'CMake\bin\cmake.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'CMake\bin\cmake.exe'),
        (Join-Path $env:LOCALAPPDATA 'Programs\CMake\bin\cmake.exe')
    )
    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path $candidate)) { return $candidate }
    }
    return $null
}

function Get-CMakeVersion([string]$CMakeExe) {
    if (-not $CMakeExe) { return $null }
    try {
        $firstLine = (& $CMakeExe --version | Select-Object -First 1)
        $match = [regex]::Match($firstLine, '(\d+\.\d+(?:\.\d+)?)')
        if ($match.Success) { return [Version]$match.Groups[1].Value }
    } catch {}
    return $null
}

function Get-VSWhere {
    if (Test-Path $VsWhereDefault) { return $VsWhereDefault }
    $cmd = Get-Command vswhere.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

function Get-VCToolsInstallation {
    $vswhere = Get-VSWhere
    if (-not $vswhere) { return $null }
    try {
        $path = (& $vswhere -latest -products '*' -requires $VcComponent -property installationPath | Select-Object -First 1)
        if ($path -and (Test-Path $path)) { return $path.Trim() }
    } catch {}
    return $null
}

function Test-WindowsSdk {
    $kitsRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
    $includeRoot = Join-Path $kitsRoot 'Include'
    $libRoot = Join-Path $kitsRoot 'Lib'
    if (-not (Test-Path $includeRoot) -or -not (Test-Path $libRoot)) { return $false }

    $includeVersions = Get-ChildItem $includeRoot -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^10\.\d+\.\d+\.\d+$' }
    foreach ($version in $includeVersions) {
        $umInclude = Join-Path $version.FullName 'um\Windows.h'
        $umLib = Join-Path $libRoot ($version.Name + '\um\x64\kernel32.lib')
        if ((Test-Path $umInclude) -and (Test-Path $umLib)) { return $true }
    }
    return $false
}

function Get-WingetExe {
    $cmd = Get-Command winget.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

function Add-CMakeToCurrentPath {
    $cmakeExe = Get-CMakeExe
    if (-not $cmakeExe) { return }
    $cmakeDir = Split-Path -Parent $cmakeExe
    $pathParts = $env:Path -split ';'
    if ($pathParts -notcontains $cmakeDir) {
        $env:Path = "$cmakeDir;$env:Path"
    }
}

function Add-CMakeToMachinePath {
    $cmakeExe = Get-CMakeExe
    if (-not $cmakeExe) { return }
    $cmakeDir = Split-Path -Parent $cmakeExe
    try {
        $machinePath = [Environment]::GetEnvironmentVariable('Path', 'Machine')
        if (($machinePath -split ';') -notcontains $cmakeDir) {
            [Environment]::SetEnvironmentVariable('Path', ($machinePath.TrimEnd(';') + ';' + $cmakeDir), 'Machine')
        }
    } catch {
        Write-Warn "No se pudo registrar CMake en el PATH global. Se usara igualmente en esta ejecucion."
    }
    Add-CMakeToCurrentPath
}

function Install-CMake {
    Write-Step 'Instalando CMake'
    $winget = Get-WingetExe
    if ($winget) {
        Write-Host 'Se usara winget (Kitware.CMake)...'
        & $winget install --id Kitware.CMake -e --source winget --silent --accept-package-agreements --accept-source-agreements
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "winget devolvio $LASTEXITCODE. Se intentara el instalador oficial como alternativa."
        }
        Add-CMakeToCurrentPath
        $cmake = Get-CMakeExe
        $version = Get-CMakeVersion $cmake
        if ($version -and $version -ge $MinimumCMake) {
            Add-CMakeToMachinePath
            Write-Ok "CMake $version instalado."
            return
        }
    }

    Write-Host 'Descargando la ultima version oficial de CMake desde Kitware/GitHub...'
    $release = Invoke-RestMethod -UseBasicParsing -Headers @{ 'User-Agent' = 'AutomatasCpp-Installer' } -Uri 'https://api.github.com/repos/Kitware/CMake/releases/latest'
    $asset = $release.assets | Where-Object { $_.name -match 'windows-x86_64\.msi$' } | Select-Object -First 1
    if (-not $asset) { throw 'No se encontro el MSI x86_64 de CMake en la version oficial mas reciente.' }

    $msi = Join-Path $env:TEMP $asset.name
    Invoke-WebRequest -UseBasicParsing -Uri $asset.browser_download_url -OutFile $msi
    $process = Start-Process -FilePath 'msiexec.exe' -ArgumentList @('/i', "`"$msi`"", '/qn', '/norestart', 'ALLUSERS=1') -Wait -PassThru
    if ($process.ExitCode -notin @(0, 3010)) {
        throw "El instalador de CMake fallo con codigo $($process.ExitCode)."
    }

    Add-CMakeToCurrentPath
    $cmake = Get-CMakeExe
    $version = Get-CMakeVersion $cmake
    if (-not $version -or $version -lt $MinimumCMake) {
        throw "CMake se instalo, pero no se pudo detectar una version >= $MinimumCMake."
    }
    Add-CMakeToMachinePath
    Write-Ok "CMake $version instalado."
}

function Install-VCTools {
    Write-Step 'Instalando Visual C++ Build Tools y Windows SDK'
    Write-Host 'Esto instala la carga de trabajo Desarrollo para escritorio con C++ (Build Tools), sin requerir el IDE completo.'

    $winget = Get-WingetExe
    if ($winget) {
        Write-Host 'Se intentara primero mediante winget...'
        $override = "--wait --passive --norestart --add $VcWorkload --includeRecommended"
        & $winget install --id $Vs2022BuildToolsId -e --source winget --accept-package-agreements --accept-source-agreements --override $override
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "winget devolvio $LASTEXITCODE. Se intentara el bootstrapper oficial de Microsoft."
        }

        Start-Sleep -Seconds 2
        if ((Get-VCToolsInstallation) -and (Test-WindowsSdk)) {
            Write-Ok 'Visual C++ Build Tools y Windows SDK detectados.'
            return
        }
    }

    Write-Host 'Descargando el bootstrapper oficial de Visual Studio 2022 Build Tools...'
    $bootstrapper = Join-Path $env:TEMP 'vs_BuildTools_AutomatasCpp.exe'
    Invoke-WebRequest -UseBasicParsing -Uri $Vs2022BootstrapperUrl -OutFile $bootstrapper

    $arguments = @(
        '--wait',
        '--passive',
        '--norestart',
        '--nocache',
        '--installPath', "`"$Vs2022BuildToolsPath`"",
        '--add', $VcWorkload,
        '--includeRecommended'
    )
    $process = Start-Process -FilePath $bootstrapper -ArgumentList $arguments -Wait -PassThru
    if ($process.ExitCode -notin @(0, 3010)) {
        throw "Visual Studio Build Tools fallo con codigo $($process.ExitCode)."
    }

    Start-Sleep -Seconds 3
    if (-not (Get-VCToolsInstallation)) {
        throw 'La instalacion termino, pero no se detecto el compilador MSVC C++ x64/x86.'
    }
    if (-not (Test-WindowsSdk)) {
        throw 'La instalacion termino, pero no se detecto un Windows SDK utilizable.'
    }
    Write-Ok 'Visual C++ Build Tools y Windows SDK instalados.'
}

function Invoke-ElevatedSelf {
    Write-Host "`nFaltan dependencias que requieren permisos de administrador. Se abrira UAC..." -ForegroundColor Yellow
    $args = @(
        '-NoLogo',
        '-NoProfile',
        '-ExecutionPolicy', 'Bypass',
        '-File', "`"$PSCommandPath`"",
        '-Elevated'
    )
    $process = Start-Process -FilePath 'powershell.exe' -Verb RunAs -ArgumentList $args -Wait -PassThru
    exit $process.ExitCode
}

try {
    Write-Step 'Comprobando dependencias'

    $cmakeExe = Get-CMakeExe
    $cmakeVersion = Get-CMakeVersion $cmakeExe
    $cmakeOk = $cmakeVersion -and ($cmakeVersion -ge $MinimumCMake)
    if ($cmakeOk) {
        Write-Ok "CMake $cmakeVersion: $cmakeExe"
    } elseif ($cmakeVersion) {
        Write-Warn "CMake $cmakeVersion es demasiado viejo. Se requiere >= $MinimumCMake."
    } else {
        Write-Warn 'CMake no esta instalado o no se puede localizar.'
    }

    $vcInstall = Get-VCToolsInstallation
    if ($vcInstall) {
        Write-Ok "MSVC C++ detectado: $vcInstall"
    } else {
        Write-Warn 'No se detecto MSVC C++ x64/x86.'
    }

    $sdkOk = Test-WindowsSdk
    if ($sdkOk) {
        Write-Ok 'Windows SDK detectado.'
    } else {
        Write-Warn 'No se detecto un Windows SDK utilizable.'
    }

    $needsInstall = (-not $cmakeOk) -or (-not $vcInstall) -or (-not $sdkOk)
    if ($needsInstall -and -not (Test-Administrator)) {
        if ($Elevated) { throw 'Se solicitaron permisos de administrador, pero el proceso no esta elevado.' }
        Invoke-ElevatedSelf
    }

    if (-not $cmakeOk) {
        Install-CMake
    }
    if ((-not $vcInstall) -or (-not $sdkOk)) {
        Install-VCTools
    }

    Write-Step 'Verificacion final'
    Add-CMakeToCurrentPath
    $cmakeExe = Get-CMakeExe
    $cmakeVersion = Get-CMakeVersion $cmakeExe
    $vcInstall = Get-VCToolsInstallation
    $sdkOk = Test-WindowsSdk

    if (-not $cmakeExe -or -not $cmakeVersion -or $cmakeVersion -lt $MinimumCMake) {
        throw "No se pudo verificar CMake >= $MinimumCMake despues de la instalacion."
    }
    if (-not $vcInstall) {
        throw 'No se pudo verificar el compilador Visual C++ despues de la instalacion.'
    }
    if (-not $sdkOk) {
        throw 'No se pudo verificar Windows SDK despues de la instalacion.'
    }

    Write-Ok "CMake $cmakeVersion"
    Write-Ok "Visual C++: $vcInstall"
    Write-Ok 'Windows SDK listo'

    Write-Step 'Configurando proyecto con CMake'
    Push-Location $ProjectDir
    try {
        & $cmakeExe -S $ProjectDir -B $BuildDir -A x64
        if ($LASTEXITCODE -ne 0) { throw "CMake configure fallo con codigo $LASTEXITCODE." }

        Write-Step 'Compilando en Release'
        & $cmakeExe --build $BuildDir --config Release --parallel
        if ($LASTEXITCODE -ne 0) { throw "La compilacion fallo con codigo $LASTEXITCODE." }

        Write-Step 'Ejecutando pruebas'
        $ctestExe = Join-Path (Split-Path -Parent $cmakeExe) 'ctest.exe'
        if (-not (Test-Path $ctestExe)) { $ctestExe = 'ctest.exe' }
        & $ctestExe --test-dir $BuildDir -C Release --output-on-failure
        if ($LASTEXITCODE -ne 0) { throw "Las pruebas fallaron con codigo $LASTEXITCODE." }

        Write-Step 'Iniciando Automatas.exe'
        $exeCandidates = @(
            (Join-Path $BuildDir 'Release\Automatas.exe'),
            (Join-Path $BuildDir 'Automatas.exe')
        )
        $exe = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
        if (-not $exe) { throw 'La compilacion termino, pero no se encontro Automatas.exe.' }

        Write-Ok "Ejecutable: $exe"
        & $exe
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "Automatas.exe termino con codigo $LASTEXITCODE."
        }
    } finally {
        Pop-Location
    }

    exit 0
} catch {
    Write-Host "`n[ERROR] $($_.Exception.Message)" -ForegroundColor Red
    Write-Host 'Revisa el mensaje anterior. Si la instalacion fue bloqueada por Windows, acepta el UAC y vuelve a ejecutar el BAT.' -ForegroundColor Red
    exit 1
}
