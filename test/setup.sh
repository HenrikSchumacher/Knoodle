#!/usr/bin/env bash
# Setup script for Knoodle end-to-end tests.
# Creates a Python venv and installs regina (for run_tests.py --cross-check).
#
# Usage:
#   ./setup.sh                  # basic setup
#   ./setup.sh --with-plantri   # also build the vendored plantri (Tier 2)
#
# run_tests.py's Tier 2 runs the vendored test/vendor/plantri/plantri, which
# `make -C test` builds anyway (plantri_check uses it), so --with-plantri is
# only a shortcut for `make -C test vendor/plantri/plantri`. It used to
# download plantri 5.8 into test/plantri58/, which run_tests.py still accepts.

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

# Step 3: Optionally build the vendored plantri
if $WITH_PLANTRI; then
    echo ""
    echo "=== Building vendored plantri ==="
    make vendor/plantri/plantri
fi

echo ""
echo "=== Setup complete ==="
echo "Run tests with: source venv/bin/activate && python run_tests.py --smoke-only"
