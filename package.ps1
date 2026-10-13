param([string]$QtPrefix=(Join-Path $PSScriptRoot '..\..\work\qt6-sdk\mingw64'),[string]$CompilerPrefix='C:\msys64\mingw64',[string]$Executable=(Join-Path $PSScriptRoot 'build\DbSchemaStudio.exe'),[string]$Destination=(Join-Path $PSScriptRoot 'dist\DbSchemaStudio'))
$ErrorActionPreference='Stop'
$QtPrefix=[IO.Path]::GetFullPath($QtPrefix)
if((Test-Path -LiteralPath $Destination) -and (Get-ChildItem -LiteralPath $Destination -Force | Select-Object -First 1)){throw 'Destination must be empty; choose a new release folder.'}
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
$destinationRoot=[IO.Path]::GetFullPath($Destination)
Copy-Item -LiteralPath $Executable -Destination (Join-Path $destinationRoot 'DbSchemaStudio.exe')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'translations') -Destination $destinationRoot -Recurse -Force
$plugins=Join-Path $QtPrefix 'share\qt6\plugins'
foreach($plugin in @('platforms\qwindows.dll')) {
 $target=Join-Path $destinationRoot $plugin
 New-Item -ItemType Directory -Path (Split-Path $target) -Force | Out-Null
 Copy-Item -LiteralPath (Join-Path $plugins $plugin) -Destination $target
}
$queue=[Collections.Generic.Queue[string]]::new()
Get-ChildItem -LiteralPath $destinationRoot -Recurse -File | Where-Object Extension -in @('.dll','.exe') | ForEach-Object { $queue.Enqueue($_.FullName) }
$seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$objdump=Join-Path $CompilerPrefix 'bin\objdump.exe'
while($queue.Count -gt 0){
 $binary=$queue.Dequeue()
 $imports=& $objdump -p $binary | Select-String 'DLL Name:\s*(.+)'
 if($LASTEXITCODE -ne 0){throw "Cannot inspect $binary"}
 foreach($line in $imports){
  $dll=$line.Matches[0].Groups[1].Value.Trim()
  if(-not $seen.Add($dll)){continue}
  $source=Join-Path $QtPrefix "bin\$dll"
  if(-not(Test-Path -LiteralPath $source)){$source=Join-Path $CompilerPrefix "bin\$dll"}
  if(Test-Path -LiteralPath $source){
   $target=Join-Path $destinationRoot $dll
   Copy-Item -LiteralPath $source -Destination $target
   $queue.Enqueue($target)
  }elseif(-not(Test-Path -LiteralPath (Join-Path $env:WINDIR "System32\$dll")) -and -not $dll.StartsWith('api-ms-')){throw "Unresolved dependency: $dll"}
 }
}
@('[Paths]','Prefix=.','Plugins=.','Translations=translations') | Set-Content -LiteralPath (Join-Path $destinationRoot 'qt.conf') -Encoding ascii
$licenseRoot=Join-Path $destinationRoot 'licenses'
New-Item -ItemType Directory -Path $licenseRoot -Force | Out-Null
foreach($prefix in @($CompilerPrefix,$QtPrefix)){
foreach($name in @('qt6-base','qt6-translations','vulkan-loader','gcc-libs','winpthreads','mingw-w64-libraries','zlib','zstd','libpng','freetype','harfbuzz','brotli','pcre2','icu','glib2','gettext-runtime','libiconv','md4c','bzip2','double-conversion','libb2','libjpeg-turbo','graphite2')){
 $source=Join-Path $prefix "share\licenses\$name"
 if(Test-Path -LiteralPath $source){Copy-Item -LiteralPath $source -Destination $licenseRoot -Recurse -Force}
}
}
Get-ChildItem -LiteralPath (Join-Path $CompilerPrefix 'share\icu') -Recurse -Filter 'LICENSE' -File | ForEach-Object { New-Item -ItemType Directory -Path (Join-Path $licenseRoot 'icu') -Force | Out-Null; Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $licenseRoot 'icu\LICENSE') }
$guideTarget=Join-Path $destinationRoot 'guide'
New-Item -ItemType Directory -Path $guideTarget -Force | Out-Null
Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'guide') -Filter '*.pdf' -File | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $guideTarget }
foreach($example in @('Ordini-guida','Esercizi-guidati','UML-libro','Panoramica')){
 $source=Join-Path $PSScriptRoot ('esempi\'+$example)
 $target=Join-Path $destinationRoot ('esempi\'+$example)
 New-Item -ItemType Directory -Path $target -Force | Out-Null
 Get-ChildItem -LiteralPath $source -File | Where-Object { $_.Extension -eq '.schema' -or $_.Name -eq 'LEGGIMI.md' } | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $target }
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'GUIDA-UTENTE.md') -Destination (Join-Path $destinationRoot 'LEGGIMI.md')
$englishReadme=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'README.md') -Raw
$englishReadme.Replace('(README.it.md)','(LEGGIMI.md)').Replace('(GUIDA-UTENTE.md)','(LEGGIMI.md)') | Set-Content -LiteralPath (Join-Path $destinationRoot 'README.md') -Encoding utf8
$hash=(Get-FileHash -LiteralPath (Join-Path $destinationRoot 'DbSchemaStudio.exe') -Algorithm SHA256).Hash
($hash+'  DbSchemaStudio.exe') | Set-Content -LiteralPath (Join-Path $destinationRoot 'SHA256.txt') -Encoding ascii
& (Join-Path $PSScriptRoot 'check-runtime.ps1') -RuntimeFolder $destinationRoot -CompilerPrefix $CompilerPrefix
Write-Output $destinationRoot
