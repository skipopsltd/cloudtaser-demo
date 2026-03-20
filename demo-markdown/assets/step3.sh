#!/bin/bash
# Step 3: Set your own secret password in the EU vault
source /tmp/helpers.sh

header "Step 3/7: Set Your Own Secret Password"

info "Type any password you want. It goes to the EU OpenBao vault in Frankfurt."
info "This password will never be stored in Kubernetes."

echo ""
echo -n "  ${BOLD}Enter your secret password: ${RESET}"
read -r USER_PASSWORD

while [ -z "$USER_PASSWORD" ]; do
    echo -n "  ${BOLD}Password cannot be empty. Try again: ${RESET}"
    read -r USER_PASSWORD
done

echo "$USER_PASSWORD" > /tmp/.user_password
echo ""

section "Write password to EU vault (Frankfurt)"

run_cmd "curl -sf -X POST \"https://secret.cloudtaser.io/v1/secret/data/demo/\$(cat /tmp/.session_id)/postgres\" \\
  -H \"X-Vault-Token: \$(cat /tmp/.cloudtaser-session-token)\" \\
  -H \"Content-Type: application/json\" \\
  -d \$(jq -n --arg pw \"$USER_PASSWORD\" '{data: {password: \$pw, username: \"postgres\"}}') \\
  && echo \"Secret updated in EU Vault (Frankfurt)\""

section "Read it back — proof it's stored in EU"

run_cmd "curl -sf \"https://secret.cloudtaser.io/v1/secret/data/demo/\$(cat /tmp/.session_id)/postgres\" \\
  -H \"X-Vault-Token: \$(cat /tmp/.cloudtaser-session-token)\" \\
  | python3 -c 'import sys,json; d=json.load(sys.stdin)[\"data\"][\"data\"]; print(\"Password in vault: \" + d[\"password\"])'"

section "See for yourself in the vault UI"

SESSION_ID=$(cat /tmp/.session_id)
SESSION_TOKEN=$(cat /tmp/.cloudtaser-session-token)

echo "  ${BG_BLUE} https://secret.cloudtaser.io/ui ${RESET}"
echo ""
echo "  ${BOLD}How to find your secret:${RESET}"
echo "  1. Open the link above"
echo "  2. Choose ${BOLD}Token${RESET} method and paste: ${BOLD}${SESSION_TOKEN}${RESET}"
echo "  3. Click ${BOLD}secret/${RESET} → ${BOLD}demo/${RESET} → ${BOLD}${SESSION_ID}/${RESET} → ${BOLD}postgres${RESET}"
echo ""

pause
