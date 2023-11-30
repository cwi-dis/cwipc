$realScript = join-path -path $PSScriptRoot -childpath 'install-3rdparty-full-win1064.ps1'
Start-Process powershell.exe -Wait -verb RunAs -ArgumentList "-ExecutionPolicy ByPass -NoExit -File `"$realScript`" "
