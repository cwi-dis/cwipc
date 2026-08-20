$bindir = $PSScriptRoot
$topdir = Split-Path -Parent $bindir
$wheeldir = join-path $topdir "share\cwipc\python"
$wheels = Get-ChildItem -path $wheeldir -filter "cwipc_*.whl" | ForEach-Object { $_.FullName }
python -m pip install importlib.metadata
python -m pip install --upgrade --find-links="$wheeldir" $wheels
