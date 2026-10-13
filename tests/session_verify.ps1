param([Parameter(Mandatory=$true)][string]$Runtime)
$ErrorActionPreference='Stop'
$Runtime=[IO.Path]::GetFullPath($Runtime)
& (Join-Path $PSScriptRoot '..\check-runtime.ps1') -RuntimeFolder $Runtime
$exe=Join-Path $Runtime 'DbSchemaStudio.exe'
$profile=Join-Path ([IO.Path]::GetTempPath()) ('DbSchemaStudio-session-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $profile | Out-Null
$env:QT_QPA_PLATFORM='windows'
$env:QT_SCALE_FACTOR='1'
Remove-Item Env:QT_PLUGIN_PATH -ErrorAction SilentlyContinue
$argsBase=@('--data-dir',('"'+$profile+'"'),'--session-probe')
$seed=$null
try {
 $seed=Start-Process -FilePath $exe -ArgumentList ($argsBase+@('seed')) -WorkingDirectory $Runtime -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $profile 'seed.log') -RedirectStandardError (Join-Path $profile 'seed.err')
 $limit=[DateTime]::UtcNow.AddSeconds(25)
 while(-not (Test-Path -LiteralPath (Join-Path $profile 'seed.json'))){if($seed.HasExited -or [DateTime]::UtcNow -gt $limit){throw 'Seed process did not write its marker'};Start-Sleep -Milliseconds 100}
 $first=Get-Content -LiteralPath (Join-Path $profile 'seed.json') -Raw | ConvertFrom-Json
 if($first.failures -ne 0){throw 'Seed checks failed'}
 $second=Start-Process -FilePath $exe -ArgumentList ($argsBase+@('seed')) -WorkingDirectory $Runtime -WindowStyle Hidden -Wait -PassThru
 if($second.ExitCode -ne 2){throw 'Session lock did not reject a second process'}
 # Terminate only the exact test process that this driver created.
 if([IO.Path]::GetFullPath($seed.Path) -ne $exe){throw 'Unexpected seed process path'}
 Stop-Process -Id $seed.Id -Force
 if(-not $seed.WaitForExit(10000)){throw 'Seed process did not stop'}
 $recovered=Start-Process -FilePath $exe -ArgumentList ($argsBase+@('recover')) -WorkingDirectory $Runtime -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $profile 'recover.log') -RedirectStandardError (Join-Path $profile 'recover.err')
 if($recovered.ExitCode -ne 0){throw 'Recovery process failed'}
 $last=Get-Content -LiteralPath (Join-Path $profile 'recover.json') -Raw | ConvertFrom-Json
 if($last.failures -ne 0){throw 'Recovery checks failed'}
 Write-Output ('PASS: '+($first.checks.Count+$last.checks.Count+1)+' process/session checks')
 Write-Output ('Results: '+$profile)
} finally {
 if($seed -and -not $seed.HasExited -and [IO.Path]::GetFullPath($seed.Path) -eq $exe){Stop-Process -Id $seed.Id -Force}
}
