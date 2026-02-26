#!/bin/bash

# ST-Link USB Device Connectivity Validator for WSL
# This script validates the WSL environment and USB/IP prerequisites for ST-Link debugging
#
# USAGE:
#   Standalone: Run this script directly to validate WSL environment
#   With Manager: Run stlink-wsl-usbip-manager.ps1 in Windows PowerShell (Admin)
#                 to automatically attach/detach ST-Link devices
#   From other scripts: Call validate_stlink_environment() function
#
# OPTIONS:
#   -q, --quiet    Quiet mode, minimal output
#   -j, --json     JSON output for script consumption
#   -h, --help     Show this help message

# Global variables
QUIET_MODE=0
JSON_OUTPUT=0
SCRIPT_RESULT=0

# Color definitions - use tput for portability
if command -v tput &> /dev/null && [ -t 1 ]; then
  readonly COLOR_RESET=$(tput sgr0)
  readonly COLOR_GREEN=$(tput setaf 2)
  readonly COLOR_RED=$(tput setaf 1)
  readonly COLOR_YELLOW=$(tput setaf 3)
  readonly COLOR_BLUE=$(tput setaf 4)
  readonly COLOR_DIM=$(tput dim)
else
  readonly COLOR_RESET=''
  readonly COLOR_GREEN=''
  readonly COLOR_RED=''
  readonly COLOR_YELLOW=''
  readonly COLOR_BLUE=''
  readonly COLOR_DIM=''
fi

# Check results storage
declare -a CHECK_NAMES
declare -a CHECK_STATUSES
declare -a CHECK_MESSAGES
declare -a CHECK_SUGGESTIONS

# Status codes
readonly STATUS_PASS=0
readonly STATUS_FAIL=1
readonly STATUS_WARN=2

# Print message based on quiet mode
print_info() {
  if [ $QUIET_MODE -eq 0 ]; then
    echo -e "$@"
  fi
}

# Print error message (always shown)
print_error() {
  echo -e "$@" >&2
}

# Record a check result
# Usage: record_result <name> <status> <message> [suggestion]
record_result() {
  local name="$1"
  local status=$2
  local message="$3"
  local suggestion="${4:-}"

  CHECK_NAMES+=("$name")
  CHECK_STATUSES+=($status)
  CHECK_MESSAGES+=("$message")
  CHECK_SUGGESTIONS+=("$suggestion")
}

# Get status symbol and color
get_status_display() {
  local status=$1
  case $status in
    $STATUS_PASS)
      echo -e "${COLOR_GREEN}✓${COLOR_RESET}"
      ;;
    $STATUS_FAIL)
      echo -e "${COLOR_RED}✗${COLOR_RESET}"
      ;;
    $STATUS_WARN)
      echo -e "${COLOR_YELLOW}⚠${COLOR_RESET}"
      ;;
    *)
      echo "?"
      ;;
  esac
}

# Get status text
get_status_text() {
  local status=$1
  case $status in
    $STATUS_PASS)
      echo "Pass"
      ;;
    $STATUS_FAIL)
      echo "Fail"
      ;;
    $STATUS_WARN)
      echo "Warn"
      ;;
    *)
      echo "Unknown"
      ;;
  esac
}

# Show usage
show_help() {
  cat << EOF
ST-Link WSL USB/IP Environment Validator

USAGE:
    $0 [OPTIONS]

OPTIONS:
    -q, --quiet    Quiet mode, minimal output (for script calling)
    -j, --json     JSON output format
    -h, --help     Show this help message

EXAMPLES:
    # Interactive use (default)
    $0

    # Quiet validation from another script
    $0 --quiet

    # Get JSON results
    $0 --json

    # Call from another script
    source $0
    validate_stlink_environment
    if [ $? -eq 0 ]; then
        echo "Environment is ready"
    fi
EOF
}

# Parse command line arguments
parse_args() {
  while [[ $# -gt 0 ]]; do
    case $1 in
      -q | --quiet)
        QUIET_MODE=1
        shift
        ;;
      -j | --json)
        JSON_OUTPUT=1
        QUIET_MODE=1
        shift
        ;;
      -h | --help)
        show_help
        exit 0
        ;;
      *)
        print_error "Unknown option: $1"
        show_help
        exit 1
        ;;
    esac
  done
}

# Individual check functions
# Each returns 0 for pass, 1 for fail, 2 for warning

check_wsl_environment() {
  if grep -qEi "(Microsoft|WSL)" /proc/version &> /dev/null; then
    echo "WSL2"
    return $STATUS_PASS
  else
    echo "Not WSL"
    return $STATUS_WARN
  fi
}

check_usbip_client() {
  # WSL2: usbip command not required - devices attached via Windows usbipd
  # Just verify USB subsystem is functional by checking if lsusb works
  if command -v lsusb &> /dev/null && lsusb &> /dev/null; then
    echo "wsl2_usbip_active"
    return $STATUS_PASS
  elif command -v usbip &> /dev/null; then
    echo "available"
    return $STATUS_PASS
  else
    echo "missing"
    return $STATUS_FAIL
  fi
}

check_usbip_module() {
  if lsmod | grep -q "usbip_host"; then
    echo "loaded"
    return $STATUS_PASS
  else
    # Try to load it
    if sudo modprobe usbip_host 2> /dev/null; then
      echo "loaded_now"
      return $STATUS_PASS
    else
      echo "failed"
      return $STATUS_FAIL
    fi
  fi
}

check_lsusb_available() {
  if command -v lsusb &> /dev/null; then
    echo "available"
    return $STATUS_PASS
  else
    echo "missing"
    return $STATUS_FAIL
  fi
}

get_usb_devices() {
  lsusb 2> /dev/null || echo ""
}

check_stlink_device() {
  local usb_devices
  usb_devices=$(get_usb_devices)
  # Match ST-Link variants: "ST-Link", "STLINK", case insensitive
  if echo "$usb_devices" | grep -qiE "st[-_]?link"; then
    echo "found"
    return $STATUS_PASS
  else
    local count
    count=$(echo "$usb_devices" | grep -v "^$" | wc -l)
    if [ "$count" -eq 0 ]; then
      echo "no_usb_devices"
      return $STATUS_WARN
    else
      echo "not_found"
      return $STATUS_WARN
    fi
  fi
}

# Core validation function for use by other scripts
# Returns: 0 on success, 1 on failure
validate_stlink_environment() {
  local total_checks=4
  local current=0
  local result=0
  local errors=""
  local checks=""

  # Clear previous results
  CHECK_NAMES=()
  CHECK_STATUSES=()
  CHECK_MESSAGES=()
  CHECK_SUGGESTIONS=()

  # Check 1: WSL Environment
  ((current++))
  local wsl_status wsl_detail
  wsl_detail=$(check_wsl_environment)
  wsl_status=$?

  if [ $wsl_status -eq $STATUS_PASS ]; then
    checks="${checks}\"wsl_environment\": true,"
    record_result "WSL Environment" $STATUS_PASS "Running in WSL2" ""
  else
    checks="${checks}\"wsl_environment\": false,"
    errors="${errors}\"Not running in WSL2 environment."
    record_result "WSL Environment" $STATUS_WARN "Not in WSL (script designed for WSL2)" ""
  fi

  # Check 2: USB/IP Client
  ((current++))
  local usbip_status usbip_detail
  usbip_detail=$(check_usbip_client)
  usbip_status=$?

  if [ $usbip_status -eq $STATUS_PASS ]; then
    checks="${checks}\"usbip_client\": true,"
    if [ "$usbip_detail" = "wsl2_usbip_active" ]; then
      record_result "USB/IP Client" $STATUS_PASS "WSL2 USB/IP active (via Windows usbipd)" ""
    else
      record_result "USB/IP Client" $STATUS_PASS "usbip command available" ""
    fi
  else
    checks="${checks}\"usbip_client\": false,"
    errors="${errors}\"usbip client not found. Install with: sudo apt install -y usbip"
    record_result "USB/IP Client" $STATUS_FAIL "usbip command not found" "Install: sudo apt install -y usbip"
    result=1
  fi

  # Check 3: USB/IP Kernel Module
  ((current++))
  local module_status module_detail
  module_detail=$(check_usbip_module)
  module_status=$?

  if [ $module_status -eq $STATUS_PASS ]; then
    if [ "$module_detail" = "loaded" ]; then
      checks="${checks}\"usbip_host_module\": true,"
      record_result "USB/IP Kernel Module" $STATUS_PASS "usbip_host module loaded" ""
    else
      checks="${checks}\"usbip_host_module\": true,"
      record_result "USB/IP Kernel Module" $STATUS_PASS "usbip_host module loaded dynamically" ""
    fi
  else
    checks="${checks}\"usbip_host_module\": false,"
    errors="${errors}\"Failed to load usbip_host module. Try: sudo apt-get install linux-modules-extra-\$(uname -r)"
    record_result "USB/IP Kernel Module" $STATUS_FAIL "Failed to load usbip_host module" "Install: sudo apt-get install linux-modules-extra-\$(uname -r)"
    result=1
  fi

  # Check 4: USB Devices and ST-Link
  ((current++))
  local lsusb_status lsusb_detail stlink_status stlink_detail
  local stlink_found=0

  lsusb_detail=$(check_lsusb_available)
  lsusb_status=$?

  if [ $lsusb_status -eq $STATUS_PASS ]; then
    checks="${checks}\"lsusb_available\": true,"

    stlink_detail=$(check_stlink_device)
    stlink_status=$?

    if [ $stlink_status -eq $STATUS_PASS ]; then
      stlink_found=1
      record_result "USB Device Enumeration" $STATUS_PASS "ST-Link device detected" ""
    else
      result=1
      if [ "$stlink_detail" = "no_usb_devices" ]; then
        record_result "USB Device Enumeration" $STATUS_FAIL "No USB devices detected" "Use usbipd attach in Windows to connect devices first"
      else
        record_result "USB Device Enumeration" $STATUS_FAIL "USB devices present but no ST-Link found" "Check device connection or run lsusb to list devices"
      fi
    fi
  else
    checks="${checks}\"lsusb_available\": false,"
    errors="${errors}\"lsusb command not available. Install with: sudo apt-get install usbutils"
    record_result "USB Device Enumeration" $STATUS_FAIL "lsusb command not available" "Install: sudo apt-get install usbutils"
    result=1
  fi

  checks="${checks}\"stlink_device_found\": $stlink_found"

  # JSON output
  if [ $JSON_OUTPUT -eq 1 ]; then
    local json_output="{"
    json_output="${json_output}\"success\": $([ $result -eq 0 ] && echo "true" || echo "false"),"
    json_output="${json_output}\"checks\": {${checks}},"
    if [ -n "$errors" ]; then
      errors="${errors%,}"
      json_output="${json_output}\"errors\": ["
      local first=1
      IFS='"' read -ra ADDR <<< "$errors"
      for i in "${ADDR[@]}"; do
        if [ -n "$i" ] && [ "$i" != " " ]; then
          if [ $first -eq 1 ]; then
            first=0
          else
            json_output="${json_output},"
          fi
          json_output="${json_output}\"$i\""
        fi
      done
      json_output="${json_output}]"
    else
      json_output="${json_output}\"errors\": []"
    fi
    json_output="${json_output}}"
    echo "$json_output"
  fi

  return $result
}

# Print check progress
print_check_progress() {
  local current=$1
  local total=$2
  local name=$3

  print_info "${COLOR_BLUE}[${current}/${total}]${COLOR_RESET} ${name} ..."
}

# Display results summary table
display_summary() {
  local total=${#CHECK_NAMES[@]}
  local pass=0 fail=0 warn=0
  local i

  for ((i = 0; i < total; i++)); do
    case ${CHECK_STATUSES[$i]} in
      $STATUS_PASS) ((pass++)) ;;
      $STATUS_FAIL) ((fail++)) ;;
      $STATUS_WARN) ((warn++)) ;;
    esac
  done

  # Print separator line
  print_info ""
  print_info "${COLOR_DIM}────────────────────────────────────────────────────────${COLOR_RESET}"
  print_info ""

  # Results table header
  print_info "${COLOR_BLUE}Validation Summary${COLOR_RESET}"
  print_info ""

  # Print each check result
  for ((i = 0; i < total; i++)); do
    local status_symbol
    status_symbol=$(get_status_display "${CHECK_STATUSES[$i]}")
    printf "  %s %-25s %s\n" "$status_symbol" "${CHECK_NAMES[$i]}" "${CHECK_MESSAGES[$i]}"

    # Print suggestion if exists
    if [ -n "${CHECK_SUGGESTIONS[$i]}" ]; then
      print_info "       ${COLOR_YELLOW}→ ${CHECK_SUGGESTIONS[$i]}${COLOR_RESET}"
    fi
  done

  print_info ""
  print_info "${COLOR_DIM}────────────────────────────────────────────────────────${COLOR_RESET}"
  print_info ""

  # Overall status
  if [ $fail -gt 0 ]; then
    print_info "${COLOR_RED}● Status: Not Ready${COLOR_RESET} (${total} checks: ${COLOR_GREEN}${pass} passed${COLOR_RESET}, ${COLOR_RED}${fail} failed${COLOR_RESET}, ${COLOR_YELLOW}${warn} warning${COLOR_RESET})"
  elif [ $warn -gt 0 ]; then
    print_info "${COLOR_YELLOW}● Status: Partially Ready${COLOR_RESET} (${total} checks: ${COLOR_GREEN}${pass} passed${COLOR_RESET}, ${COLOR_YELLOW}${warn} warning${COLOR_RESET})"
  else
    print_info "${COLOR_GREEN}● Status: Ready${COLOR_RESET} (All ${total} checks passed)"
  fi

  print_info ""
}

# Display detailed USB device list if available
display_usb_details() {
  if command -v lsusb &> /dev/null; then
    local usb_devices
    usb_devices=$(lsusb 2> /dev/null)
    if [ -n "$usb_devices" ]; then
      print_info "${COLOR_BLUE}Detected USB Devices:${COLOR_RESET}"
      echo "$usb_devices" | sed 's/^/  /'
      print_info ""
    fi
  fi
}

# Display next steps
display_next_steps() {
  local has_fail=$1

  print_info "${COLOR_DIM}────────────────────────────────────────────────────────${COLOR_RESET}"
  print_info ""
  print_info "${COLOR_BLUE}Next Steps${COLOR_RESET}"
  print_info ""

  if [ $has_fail -eq 1 ]; then
    print_info "${COLOR_YELLOW}Please fix the failed items above, then re-run this script.${COLOR_RESET}"
    print_info ""
  fi

  print_info "${COLOR_DIM}Option 1 - Automated (Recommended):${COLOR_RESET}"
  print_info "  Run in Windows PowerShell (as Administrator):"
  print_info "  ${COLOR_BLUE}./stlink-wsl-usbip-manager.ps1${COLOR_RESET}"
  print_info ""
  print_info "${COLOR_DIM}Option 2 - Manual:${COLOR_RESET}"
  print_info "  1. In Windows PowerShell/CMD (as Administrator):"
  print_info "     ${COLOR_BLUE}usbipd list${COLOR_RESET}"
  print_info ""
  print_info "  2. Locate your ST-Link device (typically Vendor ID: 0483)"
  print_info ""
  print_info "  3. Attach to WSL:"
  print_info "     ${COLOR_BLUE}usbipd attach --wsl --busid=<BUS-ID>${COLOR_RESET}"
  print_info ""
  print_info "${COLOR_DIM}────────────────────────────────────────────────────────${COLOR_RESET}"
}

# Interactive mode with user-friendly output
run_interactive() {
  # Header
  print_info ""
  print_info "${COLOR_BLUE}╔══════════════════════════════════════════════════════════╗${COLOR_RESET}"
  print_info "${COLOR_BLUE}║${COLOR_RESET}     ST-Link WSL USB/IP Environment Validator             ${COLOR_BLUE}║${COLOR_RESET}"
  print_info "${COLOR_BLUE}╚══════════════════════════════════════════════════════════╝${COLOR_RESET}"
  print_info ""
  print_info "${COLOR_DIM}Tip: Use stlink-wsl-usbip-manager.ps1 for automated device management${COLOR_RESET}"
  print_info ""

  # Run validation (this populates CHECK_* arrays and shows progress via print_check_progress)
  # We need to show progress as checks happen, then show summary

  # Override QUIET_MODE temporarily to show progress during checks
  local old_quiet=$QUIET_MODE
  QUIET_MODE=1 # Suppress individual check messages, we'll show our own

  validate_stlink_environment
  local result=$?

  QUIET_MODE=$old_quiet

  # Display progress line for each check
  local total_checks=4
  local i
  for ((i = 0; i < ${#CHECK_NAMES[@]}; i++)); do
    local idx=$((i + 1))
    local status_symbol
    status_symbol=$(get_status_display "${CHECK_STATUSES[$i]}")
    print_info "${COLOR_BLUE}[${idx}/${total_checks}]${COLOR_RESET} ${CHECK_NAMES[$i]} ... ${status_symbol}"
  done

  # Display summary table
  display_summary

  # Show USB details
  display_usb_details

  # Show next steps
  local has_fail=0
  [ $result -ne 0 ] && has_fail=1
  display_next_steps $has_fail

  return $result
}

# Function to get validation result without side effects
# Usage: result=$(check_stlink_environment)
check_stlink_environment() {
  local quiet=$QUIET_MODE
  local json=$JSON_OUTPUT
  QUIET_MODE=1
  JSON_OUTPUT=0
  validate_stlink_environment
  local result=$?
  QUIET_MODE=$quiet
  JSON_OUTPUT=$json
  return $result
}

# Main execution block
# Detect if script is being sourced or executed
if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
  # Script is being sourced
  # Export functions for use by other scripts
  export -f validate_stlink_environment
  export -f check_stlink_environment
  export -f print_info
  export -f print_error
else
  # Script is being executed directly
  parse_args "$@"

  if [ $JSON_OUTPUT -eq 1 ]; then
    validate_stlink_environment
    exit $?
  elif [ $QUIET_MODE -eq 1 ]; then
    validate_stlink_environment
    exit $?
  else
    run_interactive
    exit $?
  fi
fi
