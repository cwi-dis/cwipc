#!/bin/bash
# This script generates the documentation for the project using Doxygen.
# It assumes that Doxygen is installed and available in the system's PATH.
docdir=$(dirname "$0")
cd "$docdir" || exit
# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null
then
    echo "Doxygen could not be found. Please install Doxygen to generate documentation."
    exit 1
fi
# Setup our Python environment
source ../build/venv/bin/activate
# Setup the required modules
pip install -r requirements.txt
# Generate the documentation
python -m sphinx -b html . _build/html
echo "Documentation generated in _build/html"