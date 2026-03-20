#!/bin/bash
# CloudTaser S3 Proxy Demo — shared helpers

GREEN=$'\033[32m'
YELLOW=$'\033[33m'
CYAN=$'\033[36m'
RED=$'\033[31m'
BOLD=$'\033[1m'
DIM=$'\033[2m'
BG_BLUE=$'\033[44;97m'
RESET=$'\033[0m'

header() {
    clear
    local title="  CloudTaser S3 Proxy  |  $1"
    local len=${#title}
    local w=$(tput cols 2>/dev/null || echo 80)
    if [ "$len" -gt "$w" ]; then len=$w; fi
    local line=""
    for ((i=0; i<len; i++)); do line+="━"; done
    echo "${BOLD}${GREEN}${line}${RESET}"
    echo "${BOLD}${title}${RESET}"
    echo "${BOLD}${GREEN}${line}${RESET}"
    echo ""
}

info() {
    echo "  ${CYAN}$1${RESET}"
    echo ""
}

section() {
    echo "  ${BOLD}${GREEN}── $1 ──${RESET}"
    echo ""
}

run_cmd() {
    echo "  ${YELLOW}\$ $1${RESET}"
    echo ""
    eval "$1" 2>&1 | sed 's/^/  /'
    echo ""
}

pause() {
    echo ""
    echo "  ${DIM}Press ENTER to continue...${RESET}"
    read -r
}

step_guard() {
    local step="$1"
    local marker="/tmp/.cloudtaser-s3proxy-step${step}-done"
    if [ -f "$marker" ]; then
        echo ""
        echo "  ${YELLOW}Step ${step} already done. Please proceed to the next step on the left side.${RESET}"
        echo ""
        exit 0
    fi
    trap "touch $marker" EXIT
}

wait_for_setup() {
    local spin='|/-\'
    local i=0
    while [ ! -f /tmp/.cloudtaser-setup-done ]; do
        printf "\r  %sWaiting for environment to become ready... %s%s" "$DIM" "${spin:i++%4:1}" "$RESET"
        sleep 0.2
    done
    printf "\r  %sEnvironment ready.                %s\n\n" "$GREEN" "$RESET"
}
