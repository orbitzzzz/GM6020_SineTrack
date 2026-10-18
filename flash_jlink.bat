@echo off
setlocal
cd /d "%~dp0"

rem ============================================================
rem  flash_jlink.bat - program the RoboMaster C board (STM32F407IGH6)
rem  through a SEGGER J-Link, using the J-Link tools bundled in STM32CubeIDE.
rem
rem  Usage:
rem    flash_jlink.bat                 flash build\GM6020_SineTrack_C.elf
rem    flash_jlink.bat <file.elf>      flash a specific ELF
rem
rem  The board must be the C board (F407). For the F103 board use
rem  ..\NO.2 in CubeIDE - its launch config selects STM32F103C8.
rem ============================================================

set "IDE_ROOT=C:\ST"
set "DEVICE=STM32F407IG"
set "SPEED=1000"
set "ELF=%~1"
if "%ELF%"=="" set "ELF=%~dp0build\GM6020_SineTrack_C.elf"

if not exist "%ELF%" (
  echo [ERROR] ELF not found: "%ELF%"
  echo         Build the project first ^(cmake --build build^).
  pause
  exit /b 1
)

rem ---- locate JLink.exe (CubeIDE external tools plugin) ----
set "JL="
for /d %%d in ("%IDE_ROOT%\STM32CubeIDE_*") do (
  for /d %%p in ("%%~fd\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.jlink.win32_*") do (
    if not defined JL if exist "%%~fp\tools\bin\JLink.exe" set "JL=%%~fp\tools\bin\JLink.exe"
  )
)
if not defined JL (
  echo [ERROR] JLink.exe not found under "%IDE_ROOT%".
  pause
  exit /b 1
)

rem ---- write the Commander script (paths with forward slashes) ----
set "SCRIPT=%~dp0_jlink_flash.jlink"
> "%SCRIPT%" echo connect
>>"%SCRIPT%" echo h
>>"%SCRIPT%" echo loadfile %ELF:\=/%
>>"%SCRIPT%" echo r
>>"%SCRIPT%" echo g
>>"%SCRIPT%" echo q

rem ---- program + verify (loadfile verifies automatically) ----
"%JL%" -device %DEVICE% -if SWD -speed %SPEED% -autoconnect 1 -NoGui 1 -ExitOnError 1 -CommanderScript "%SCRIPT%"
set "RC=%ERRORLEVEL%"
del "%SCRIPT%" >nul 2>&1

if not "%RC%"=="0" (
  echo.
  echo [ERROR] flashing failed, exit code %RC%
  echo         If the probe reports "Out of sync", unplug/replug the J-Link USB
  echo         cable and run again. If it reports a communication timeout,
  echo         keep SWD speed at 1000 kHz.
) else (
  echo.
  echo [OK] flashed and verified: "%ELF%"
)
pause
