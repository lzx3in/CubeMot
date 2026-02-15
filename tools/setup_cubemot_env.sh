#!/bin/bash
set -e

echo "=========================================="
echo "CubeMot Build Environment Setup Script"
echo "For Debian Trixie"
echo "=========================================="
echo

echo "Step 1: Updating package lists..."
apt-get update

echo
echo "Step 2: Installing system dependencies..."
apt-get install -y \
    git \
    build-essential \
    ninja-build \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    libnewlib-arm-none-eabi \
    python3 \
    python3-pip \
    python3-venv \
    cmake \
    wget \
    ca-certificates \
    curl

echo
echo "Step 3: Verifying toolchain installation..."
arm-none-eabi-gcc --version
echo "✓ GCC ARM Embedded toolchain installed"

cmake --version
echo "✓ CMake installed"

ninja --version
echo "✓ Ninja installed"

echo
echo "Step 4: Setting up Python virtual environment..."
if [ -d ".venv" ]; then
    echo "Virtual environment already exists, skipping creation"
else
    python3 -m venv .venv
    echo "✓ Python virtual environment created"
fi

source .venv/bin/activate
echo "✓ Virtual environment activated"

echo
echo "Step 5: Installing Python dependencies..."
pip install --upgrade pip
pip install -r tools/requirements.txt
echo "✓ Python dependencies installed"

echo
echo "=========================================="
echo "Environment setup complete!"
echo "=========================================="
echo
echo "To use this environment in the future:"
echo "  source .venv/bin/activate"
echo
