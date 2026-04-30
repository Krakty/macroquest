@echo off
cd /d C:\macroquest\tools\comment-update
if exist global.json del global.json
"C:\VS2022\MSBuild\Current\Bin\MSBuild.exe" src\comment-update\comment-update.csproj /p:Configuration=Release /restore
