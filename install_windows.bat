@echo off
REM Installs the built Chime 30 VST3 into the standard Windows VST3 folder.
REM Right-click this file and choose "Run as administrator" (the folder is under Program Files).

set "SRC=%~dp0build\Chime30_artefacts\Release\VST3\Chime 30.vst3"
set "DST=%CommonProgramFiles%\VST3\Chime 30.vst3"

if not exist "%SRC%" (
    echo Could not find "%SRC%"
    echo Build the plugin first, or copy the downloaded "Chime 30.vst3" folder to "%CommonProgramFiles%\VST3" by hand.
    pause
    exit /b 1
)

xcopy "%SRC%" "%DST%\" /E /I /Y
echo.
echo Installed to "%DST%". Rescan plugins in your DAW.
pause
