@echo off

:: Only execute in main folder
setlocal enabledelayedexpansion
set "current_dir=%cd%"
for %%A in ("%current_dir%") do set "last_part=%%~nxA"
if /i "!last_part!"=="build" (
    cd ..
)
set "main_dir=%cd%"

cls

:: Defaults
set "src_dir=src/world/*.cpp"
set "debug_mode=false"
set "release_mode=false"

:: Parse arguments
for %%a in (%*) do (
    if /i "%%a"=="-d" set "debug_mode=true"
    if /i "%%a"=="--debug" set "debug_mode=true"
    if /i "%%a"=="--dev" set "debug_mode=true"
    if /i "%%a"=="-r" set "release_mode=true"
    if /i "%%a"=="--release" set "release_mode=true"
    if /i "%%a"=="world" set "src_dir=src/world/*.cpp"
    if /i "%%a"=="level" set "src_dir=src/level/*.cpp"
)

:: Compile Command
set "compile_cmd=g++ build/resource.o src/glad.c src/*.cpp !src_dir! src/font/*.cpp src/math/*.cpp src/popup/*.cpp --std=c++17 -I"include/" -L"lib/GLFW/" -o FloodForge.exe -lglfw3 -lgdi32 -lopengl32 -luser32 -lcomdlg32 -lole32"

if "%debug_mode%"=="false" (
    set "compile_cmd=!compile_cmd! -mwindows"
)
if "%release_mode%"=="true" (
    set "compile_cmd=!compile_cmd! -static -O2"
)


:: Compile
!compile_cmd!


:: Execute
if "%release_mode%"=="false" (
	if %errorlevel%==0 (
	    FloodForge.exe
	) else (
	    echo Compilation failed.
	)
)


:: Zip
if "%release_mode%"=="true" (
	"C:\Program Files\7-Zip\7z.exe" a "FloodForge-Windows.zip" "FloodForge.exe" "assets\*" "README.md" "LICENSE" -r
)

:: Return
cd %current_dir%