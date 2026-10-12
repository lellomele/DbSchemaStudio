param([Parameter(Mandatory=$true)][string]$RuntimeFolder,[string]$CompilerPrefix='C:\msys64\mingw64')
# Read-only preflight: never launch a partial package or rely on a development PATH.
$ErrorActionPreference='Stop'
$runtimeRoot=(Resolve-Path -LiteralPath $RuntimeFolder).Path
$exe=Join-Path $runtimeRoot 'DbSchemaStudio.exe'
$stamp=Join-Path $runtimeRoot 'SHA256.txt'
if(-not(Test-Path -LiteralPath $stamp)){throw 'Package incomplete: SHA256.txt is written only after packaging finishes.'}
$hash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
if(-not((Get-Content -LiteralPath $stamp -Raw).StartsWith($hash,[StringComparison]::OrdinalIgnoreCase))){throw 'Executable does not match the completed package.'}
if(-not(Test-Path -LiteralPath (Join-Path $runtimeRoot 'platforms\qwindows.dll'))){throw 'Missing Windows platform plugin.'}
$objdump=Join-Path $CompilerPrefix 'bin\objdump.exe'
$missing=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($binary in (Get-ChildItem -LiteralPath $runtimeRoot -Recurse -File | Where-Object Extension -in @('.exe','.dll'))){
 $imports=& $objdump -p $binary.FullName | Select-String 'DLL Name:\s*(.+)'
 if($LASTEXITCODE -ne 0){throw "Cannot inspect dependency imports: $($binary.FullName)"}
 foreach($line in $imports){
  $dll=$line.Matches[0].Groups[1].Value.Trim()
  if(-not(Test-Path -LiteralPath (Join-Path $runtimeRoot $dll)) -and -not(Test-Path -LiteralPath (Join-Path $env:WINDIR "System32\$dll")) -and -not $dll.StartsWith('api-ms-')){[void]$missing.Add($dll)}
 }
}
if($missing.Count){throw ('Missing runtime dependencies: '+(($missing|Sort-Object)-join ', '))}
Write-Output "Runtime complete: $runtimeRoot"
