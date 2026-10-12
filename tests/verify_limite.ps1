param([string]$QtPrefix=(Join-Path $PSScriptRoot '..\..\..\work\qt6-sdk\mingw64'),[string]$CompilerPrefix='C:\msys64\mingw64',[string]$BuildDirectory=(Join-Path $PSScriptRoot '..\..\..\work\qt6-build-093'))
$ErrorActionPreference='Stop'
$QtPrefix=[IO.Path]::GetFullPath($QtPrefix)
$BuildDirectory=[IO.Path]::GetFullPath($BuildDirectory)
$env:PATH="$QtPrefix\bin;$CompilerPrefix\bin;"+$env:PATH
$env:QT_PLUGIN_PATH="$QtPrefix\share\qt6\plugins"
$env:QT_QPA_PLATFORM='offscreen'
$env:QT_SCALE_FACTOR='1'
$env:QT_FONT_DPI='96'
$include="$QtPrefix\include\qt6"
$run=Join-Path ([IO.Path]::GetTempPath()) ('DbSchemaStudio-test-limite-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $run | Out-Null
Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'fixtures\test-limite-azienda') -Filter '*.schema' | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $run }
$exe=Join-Path $run 'verify_limite.exe'
& "$CompilerPrefix\bin\g++.exe" '-I',(Join-Path $PSScriptRoot '..\src'),'-std=c++17' '-O0' '-DQT_NO_DEBUG' '-isystem' $include '-isystem' "$include\QtWidgets" '-isystem' "$include\QtGui" '-isystem' "$include\QtCore" '-isystem' "$include\QtPrintSupport" (Join-Path $PSScriptRoot 'verify_limite.cpp') (Join-Path $BuildDirectory 'model.o') (Join-Path $BuildDirectory 'logical.o') (Join-Path $BuildDirectory 'rise_import.o') (Join-Path $BuildDirectory 'qrc_icons.o') (Join-Path $BuildDirectory 'physical.o') "-L$QtPrefix\lib" '-lQt6Widgets' '-lQt6PrintSupport' '-lQt6Gui' '-lQt6Core' '-o' $exe
if($LASTEXITCODE -ne 0){throw 'Test compilation failed; run build.ps1 first.'}
& $exe $run '--final' 2>&1 | Set-Content -LiteralPath (Join-Path $run 'execution.log') -Encoding utf8
if($LASTEXITCODE -ne 0){throw "Test failed: $run"}
Get-Content -LiteralPath (Join-Path $run 'execution.log')
Write-Output "Results: $run"
