# Step 0: Install Python 3.10.* (this step is assumed to be done manually or by another script)

# Step 1: Create a virtual environment if it doesn't exist
if (-Not (Test-Path -Path "windows-venv")) {
    Write-Output "Creating virtual environment..."
    python -m venv windows-venv
}
else {
    Write-Output "Virtual environment already exists."
}

# Step 2: Activate the virtual environment if not already active
if (-Not $env:VIRTUAL_ENV) {
    Write-Output "Activating virtual environment..."
    .\windows-venv\Scripts\Activate.ps1
}
else {
    Write-Output "Virtual environment already activated."
}

# Step 3: Install dependencies
Write-Output "Installing dependencies from requirements.txt..."
pip install -r requirements.txt

Write-Output "Setup complete."