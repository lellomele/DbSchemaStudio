param([string]$SdkDirectory=(Join-Path $PSScriptRoot '..\..\work\qt6-sdk'))
$ErrorActionPreference='Stop'
$sdkRoot=[IO.Path]::GetFullPath($SdkDirectory)
$cacheRoot=Join-Path (Split-Path $sdkRoot) 'qt6-packages'
New-Item -ItemType Directory -Force -Path $sdkRoot,$cacheRoot | Out-Null
$packages=@(
 @{File='mingw-w64-x86_64-qt6-base-6.11.2-2-any.pkg.tar.zst';Hash='c15187bfa6c4c15a8d64788af4ba8b830c3faab6e2395201e2aaa7e0d993cb84'},
 @{File='mingw-w64-x86_64-qt6-translations-6.11.2-1-any.pkg.tar.zst';Hash='4d3217e43767863ff61fe8369263a8d5a01c71fd44dc97584f4eb19c2b5a4db0'}
)
foreach($package in $packages){
 $archive=Join-Path $cacheRoot $package.File
 if(-not(Test-Path -LiteralPath $archive)){Invoke-WebRequest -Uri ('https://mirror.msys2.org/mingw/mingw64/'+$package.File) -OutFile $archive}
 if((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $package.Hash){throw "Checksum mismatch: $archive"}
 & tar.exe -xf $archive -C $sdkRoot
 if($LASTEXITCODE -ne 0){throw "Extraction failed: $archive"}
}
Write-Output "Qt 6.11.2 installed in $sdkRoot. Use build.ps1 with a compatible MinGW-w64 toolchain."
