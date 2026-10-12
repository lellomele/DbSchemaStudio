param([string]$QtPrefix=(Join-Path $PSScriptRoot '..\..\work\qt6-sdk\mingw64'),[string]$CompilerPrefix='C:\msys64\mingw64',[string]$BuildDirectory=(Join-Path $PSScriptRoot '..\..\work\qt6-build-097'))
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Path $BuildDirectory -Force | Out-Null
$buildRoot=[IO.Path]::GetFullPath($BuildDirectory)
$QtPrefix=[IO.Path]::GetFullPath($QtPrefix)
$env:PATH=(Join-Path $QtPrefix 'bin')+';'+(Join-Path $CompilerPrefix 'bin')+';'+$env:PATH
$compiler=Join-Path $CompilerPrefix 'bin\g++.exe'
$include=Join-Path $QtPrefix 'include\qt6'
$compileArgs=@('-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-DQT_NO_DEBUG','-DUNICODE','-D_UNICODE','-isystem',"$include",'-isystem',"$include\QtWidgets",'-isystem',"$include\QtGui",'-isystem',"$include\QtCore",'-isystem',"$include\QtPrintSupport",'-c')
$rcc=Join-Path $QtPrefix 'share\qt6\bin\rcc.exe'
$resourceTemp=Join-Path ([IO.Path]::GetTempPath()) ('DbSchemaStudio-icons-'+[Guid]::NewGuid().ToString('N')+'.cpp')
try {
 & $rcc (Join-Path $PSScriptRoot 'assets\icons.qrc') '-o' $resourceTemp
 if($LASTEXITCODE -ne 0){throw 'Icon resource compilation failed'}
 Copy-Item -LiteralPath $resourceTemp -Destination (Join-Path $buildRoot 'qrc_icons.cpp')
} finally { if(Test-Path -LiteralPath $resourceTemp){Remove-Item -LiteralPath $resourceTemp} }
& $compiler @compileArgs (Join-Path $buildRoot 'qrc_icons.cpp') '-o' (Join-Path $buildRoot 'qrc_icons.o')
if($LASTEXITCODE -ne 0){throw 'Qt icon resource compilation failed'}
& (Join-Path $CompilerPrefix 'bin\windres.exe') '--codepage=65001' '-I' $PSScriptRoot (Join-Path $PSScriptRoot 'app.rc') '-o' (Join-Path $buildRoot 'app.res.o')
if($LASTEXITCODE -ne 0){throw 'Windows icon resource compilation failed'}
foreach($source in @('main','model','logical','rise_import','physical')) {
 & $compiler @compileArgs (Join-Path $PSScriptRoot "$source.cpp") '-o' (Join-Path $buildRoot "$source.o")
 if($LASTEXITCODE -ne 0){throw "Compilation failed: $source"}
}
& $compiler (Join-Path $buildRoot 'main.o') (Join-Path $buildRoot 'model.o') (Join-Path $buildRoot 'logical.o') (Join-Path $buildRoot 'rise_import.o') (Join-Path $buildRoot 'qrc_icons.o') (Join-Path $buildRoot 'physical.o') (Join-Path $buildRoot 'app.res.o') "-L$QtPrefix\lib" '-mwindows' '-lQt6Widgets' '-lQt6PrintSupport' '-lQt6Gui' '-lQt6Core' '-o' (Join-Path $buildRoot 'DbSchemaStudio.exe')
if($LASTEXITCODE -ne 0){throw 'Link failed'}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'translations') -Destination $buildRoot -Recurse -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'fixtures') -Destination $buildRoot -Recurse -Force
Write-Output (Join-Path $buildRoot 'DbSchemaStudio.exe')

$testFixtures=Join-Path $buildRoot 'test-fixtures'
New-Item -ItemType Directory -Path $testFixtures -Force | Out-Null
foreach($folder in @('esempi\Ordini-guida','esempi\Esercizi-guidati','tests\fixtures\test-limite-azienda')) { Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot $folder) -Filter '*.schema' | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $testFixtures -Force } }

Copy-Item -LiteralPath (Join-Path $PSScriptRoot "esempi") -Destination $buildRoot -Recurse -Force
