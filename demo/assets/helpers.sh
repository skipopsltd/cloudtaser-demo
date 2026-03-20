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
    local title="  CloudTaser  |  $1"
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
    local marker="/tmp/.cloudtaser-step${step}-done"
    if [ -f "$marker" ]; then
        echo ""
        echo "  ${YELLOW}Step ${step} already done. Please proceed to the next step on the left side.${RESET}"
        echo ""
        exit 0
    fi
    trap "touch $marker" EXIT
}

wait_for_postgres() {
    local pw
    pw=$(cat /tmp/.user_password 2>/dev/null || echo "")
    local max=30
    local i=0
    printf "  %sWaiting for PostgreSQL to accept connections...%s" "$DIM" "$RESET"
    while ! kubectl exec postgres-demo -- bash -c "PGPASSWORD='$pw' psql -h 127.0.0.1 -U postgres -c 'SELECT 1'" &>/dev/null; do
        i=$((i+1))
        if [ $i -ge $max ]; then
            printf "\n  %sPostgreSQL did not become ready in time.%s\n" "$RED" "$RESET"
            return 1
        fi
        sleep 2
    done
    printf " %sready.%s\n\n" "$GREEN" "$RESET"
}

wait_for_setup() {
    local spin='|/-\'
    local i=0
    while [ ! -f /tmp/.cloudtaser-pre-setup-done ]; do
        printf "\r  %sWaiting for environment to become ready... %s%s" "$DIM" "${spin:i++%4:1}" "$RESET"
        sleep 0.2
    done
    printf "\r  %sEnvironment ready.                %s\n\n" "$GREEN" "$RESET"
}
