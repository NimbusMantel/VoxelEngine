@echo off

REM --------------------------------------------------

REM List the folders of the header files

set HEADERS=..\src\core ..\src\platform\common ..\src\platform\emscripten

REM List the source files

set SOURCES=..\src\platform\emscripten\main.cpp ..\src\core\game.cpp ..\src\platform\emscripten\platform_asset_utils.cpp ..\src\platform\common\platform_log.cpp ..\src\platform\common\platform_file_utils.cpp ..\src\core\asset_utils.cpp ..\src\core\buffer.cpp ..\src\core\shader.cpp ..\src\core\voxel.cpp

REM Set a folder with files to embed

set EMBED=../shd

REM Set the export target file

set TARGET=game.js

REM --------------------------------------------------

set t=%HEADERS%

set HEADERS=

:hloop
for /f "tokens=1*" %%a in ("%t%") do (
   set HEADERS=%HEADERS% -I%%a
   
   set t=%%b
   )
if defined t goto :hloop

set t=%SOURCES%

set SOURCES=

rem setlocal EnableDelayedExpansion

set m=

:sloop
for /f "tokens=1*" %%a in ("%t%") do (
   set t=%%b
   set m=%%a
   )
   set p=%m:.cpp=.o%
   
   call emcc%HEADERS% %m% -o %p%
   
   set SOURCES=%SOURCES% %p%
if defined t goto :sloop

if defined EMBED goto :emb
set EMBED=
goto :skp

:emb
set EMBED= --preload-file %EMBED%@/
goto :skp

:skp
call emcc%SOURCES% -o %TARGET%%EMBED% -s FULL_ES2=1 -O3

set t=%SOURCES%

:cloop
for /f "tokens=1*" %%a in ("%t%") do (
   call del %%a
   
   set t=%%b
   )
if defined t goto :cloop

pause