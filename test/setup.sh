#!/usr/bin/env bash
# Setup script for Knoodle end-to-end tests.
# Creates a Python venv, installs regina, and optionally builds plantri.
#
# Usage:
#   ./setup.sh                  # basic setup (Tier 1 tests)
#   ./setup.sh --with-plantri   # also download and build plantri (Tier 2)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

WITH_PLANTRI=false
KNOODLESIMPLIFY="${SCRIPT_DIR}/../tools/knoodlesimplify"

for arg in "$@"; do
    case "$arg" in
        --with-plantri) WITH_PLANTRI=true ;;
        --knoodlesimplify=*) KNOODLESIMPLIFY="${arg#--knoodlesimplify=}" ;;
        *) echo "Unknown option: $arg"; exit 2 ;;
    esac
done

# Step 1: Verify knoodlesimplify binary
echo "=== Checking knoodlesimplify ==="
if [ -x "$KNOODLESIMPLIFY" ]; then
    echo "Found: $KNOODLESIMPLIFY"
else
    echo "knoodlesimplify not found at $KNOODLESIMPLIFY"
    echo "Build it with: make -C ../tools"
    exit 2
fi

# Step 2: Create Python venv and install regina
echo ""
echo "=== Setting up Python venv ==="
if [ ! -d venv ]; then
    python3 -m venv venv
    echo "Created venv"
else
    echo "venv already exists"
fi

source venv/bin/activate

echo "Installing requirements..."
pip install -q -r requirements.txt

echo "Verifying regina..."
python3 -c "import regina; print(f'regina {regina.versionString()} OK')"

# Step 3: Optionally download and build plantri
if $WITH_PLANTRI; then
    echo ""
    echo "=== Setting up plantri ==="
    if [ -x plantri58/plantri ]; then
        echo "plantri already built"
    else
        echo "Downloading plantri..."
        curl -sL https://users.cecs.anu.edu.au/~bdm/plantri/plantri58.tar.gz -o plantri58.tar.gz
        tar xzf plantri58.tar.gz
        rm plantri58.tar.gz
        echo "Building plantri..."
        make -C plantri58
        echo "plantri built successfully"
    fi
fi

echo ""
echo "=== Setup complete ==="
echo "Run tests with: source venv/bin/activate && python run_tests.py --smoke-only"
