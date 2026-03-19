#!/bin/bash
# CloudTaser Demo — shared helpers

GREEN='\033[32m'
YELLOW='\033[33m'
CYAN='\033[36m'
RED='\033[31m'
BOLD='\033[1m'
DIM='\033[2m'
BG_BLUE='\033[44;97m'
RESET='\033[0m'

header() {
    clear
    echo -e "${BOLD}${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo -e "${BOLD}  CloudTaser  |  $1${RESET}"
    echo -e "${BOLD}${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo ""
}

info() {
    echo -e "  ${CYAN}$1${RESET}"
    echo ""
}

section() {
    echo -e "  ${BOLD}${GREEN}── $1 ──${RESET}"
    echo ""
}

run_cmd() {
    echo -e "  ${YELLOW}\$ $1${RESET}"
    echo ""
    eval "$1" 2>&1 | sed 's/^/  /'
    echo ""
}

pause() {
    echo ""
    echo -e "  ${DIM}Press ENTER to continue...${RESET}"
    read -r
}

wait_for_setup() {
    local spin='|/-\'
    local i=0
    while [ ! -f /tmp/.cloudtaser-pre-setup-done ]; do
        printf "\r  ${DIM}Waiting for environment setup... ${spin:i++%4:1}${RESET}"
        sleep 0.2
    done
    printf "\r  ${GREEN}Environment ready.                ${RESET}\n\n"
}
