$realScript = join-path -path $PSScriptRoot -childpath 'install-3rdparty-full-win1064.ps1'
# $command = "& { Start-Process powershell.exe -Wait -verb RunAs -ArgumentList \`"-ExecutionPolicy ByPass -NoExit -Command $realScript \`" }"
# powershell -Command $command
Start-Process powershell.exe -Wait -verb RunAs -ArgumentList "-ExecutionPolicy ByPass -NoExit -Command $realScript "
