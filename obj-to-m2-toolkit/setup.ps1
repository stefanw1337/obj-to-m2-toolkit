$ErrorActionPreference = 'Stop'
python -m venv "$PSScriptRoot\.venv"
if ($LASTEXITCODE -ne 0) { throw 'Virtual environment creation failed' }
& "$PSScriptRoot\.venv\Scripts\python.exe" -m pip install -r "$PSScriptRoot\requirements.txt"
if ($LASTEXITCODE -ne 0) { throw 'Dependency installation failed' }
Write-Host 'Ready. Run .venv\Scripts\python.exe scripts\pipeline.py --help'
