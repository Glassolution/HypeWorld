@echo off
rem Atualizar e abrir o Hype World no PC Windows (dois cliques neste arquivo).
rem   1. git pull (pega o que o Claude da nuvem mandou para o GitHub)
rem   2. compila o C++ (editor fechado)
rem   3. abre o jogo (ou o editor: atualizar_e_abrir.bat editor)
rem Os scripts de Tools\pendentes.txt NAO rodam aqui (eles usam caminhos do Mac): rode-os no Mac.
rem Ajuste UE abaixo se a Unreal estiver instalada em outro lugar.
setlocal
set "UE=C:\Program Files\Epic Games\UE_5.8"
set "PROJ=%~dp0.."
set "UPROJECT=%PROJ%\HypeWorld.uproject"

tasklist /FI "IMAGENAME eq UnrealEditor.exe" | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo A Unreal ^(editor ou jogo^) esta aberta. Salve, feche e rode de novo.
    pause & exit /b 1
)

echo === 1/3 Puxando do GitHub ===
cd /d "%PROJ%"
git diff --quiet && git diff --cached --quiet
if errorlevel 1 (
    git status --short
    echo Voce tem mudancas locais sem commit. Faca commit antes de puxar.
    pause & exit /b 1
)
git pull --ff-only origin main || (echo Nao deu para puxar. & pause & exit /b 1)
git lfs pull
git log --oneline -1

echo === 2/3 Compilando o C++ ===
call "%UE%\Engine\Build\BatchFiles\Build.bat" HypeWorldEditor Win64 Development -project="%UPROJECT%" -waitmutex
if errorlevel 1 (
    echo A compilacao falhou. Mande os erros acima para o Claude.
    pause & exit /b 1
)

echo === 3/3 Abrindo ===
if /I "%1"=="editor" (
    start "" "%UE%\Engine\Binaries\Win64\UnrealEditor.exe" "%UPROJECT%"
) else (
    start "" "%UE%\Engine\Binaries\Win64\UnrealEditor.exe" "%UPROJECT%" -game -windowed -ResX=1600 -ResY=900
)
endlocal
