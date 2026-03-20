#!/bin/bash
# CloudTaser Demo — shared helpers

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
    echo "${BOLD}${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo "${BOLD}  CloudTaser  |  $1${RESET}"
    echo "${BOLD}${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
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

wait_for_setup() {
    local spin='|/-\'
    local i=0
    while [ ! -f /tmp/.cloudtaser-pre-setup-done ]; do
        printf "\r  %sWaiting for environment setup... %s%s" "$DIM" "${spin:i++%4:1}" "$RESET"
        sleep 0.2
    done
    printf "\r  %sEnvironment ready.                %s\n\n" "$GREEN" "$RESET"
}
