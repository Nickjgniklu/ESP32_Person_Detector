#!/bin/bash

# Step 0: Install Python 3.10.* (this step is assumed to be done manually or by another script)

# Step 1: Create a virtual environment if it doesn't exist
if [ ! -d "venv" ]; then
	echo "Creating virtual environment..."
	python -m venv venv
else
	echo "Virtual environment already exists."
fi

# Step 2: Activate the virtual environment if not already active
if [ -z "$VIRTUAL_ENV" ]; then
	echo "Activating virtual environment..."
	source venv/bin/activate
else
	echo "Virtual environment already activated."
fi

# Step 3: Install dependencies
echo "Installing dependencies from requirements.txt..."
pip install -r requirements.txt

echo "Setup complete."