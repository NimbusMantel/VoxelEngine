@echo off

REM --------------------------------------------------

REM List the folders of the header files

set HEADERS=..\src\core ..\src\platform\common ..\src\platform\emscripten

REM List the source files

set SOURCES=..\src\platform\emscripten\main.cpp ..\src\core\game.cpp ..\src\platform\common\platform_log.cpp ..\src\platform\common\platform_file_utils.cpp ..\src\core\voxel.cpp ..\src\core\camera.cpp ..\src\core\geo.cpp ..\src\core\colour.cpp ..\lib\libSDL2.a

REM Set a folder with files to embed

set EMBED=../ext

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
   set p=%p:.a=.o%
   
   call emcc%HEADERS% %m% -o %p% -std=c++11
   
   set SOURCES=%SOURCES% %p%
if defined t goto :sloop

if defined EMBED goto :emb
set EMBED=
goto :skp

:emb
set EMBED= --preload-file %EMBED%@/
goto :skp

:skp
call emcc%SOURCES% -o %TARGET%%EMBED% -O3 -s TOTAL_MEMORY=268435456

rem WebGL: -s FULL_ES2=1

set t=%SOURCES%

:cloop
for /f "tokens=1*" %%a in ("%t%") do (
rem   call del %%a
   
   set t=%%b
   )
if defined t goto :cloop

pause