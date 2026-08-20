@echo on
:removed set "params=%*"
:removed cd /d "%~dp0" && ( if exist "%temp%\getadmin.vbs" del "%temp%\getadmin.vbs" ) && fsutil dirty query %systemdrive% 1>nul 2>nul || (  echo Set UAC = CreateObject^("Shell.Application"^) : UAC.ShellExecute "cmd.exe", "/k cd ""%~sdp0"" && %~s0 %params%", "", "runas", 1 >> "%temp%\getadmin.vbs" && "%temp%\getadmin.vbs" && exit /B )

set bindir=%~dp0
set wheeldir=%bindir%..\share\cwipc\python
set python=python
cd %wheeldir%
%python% -m pip --quiet install importlib.metadata
for %%f in ("cwipc_*.whl") do ( %python% -m pip install --upgrade --find-links="." "%%f" )
