#!/bin/bash
# Step 4: Move password to EU vault, delete old K8s Secret
source /tmp/helpers.sh
step_guard 4

header "Step 4/8: Move Password to EU Vault"

USER_PASSWORD=$(cat /tmp/.user_password 2>/dev/null || echo "")

info "Now let's fix this. We'll move the password to an EU-hosted OpenBao vault."

section "Write password to EU vault (Frankfurt)"

run_cmd "curl -sf -X POST \"https://secret.cloudtaser.io/v1/secret/data/demo/\$(cat /tmp/.session_id)/postgres\" \\
  -H \"X-Vault-Token: \$(cat /tmp/.cloudtaser-session-token)\" \\
  -H \"Content-Type: application/json\" \\
  -d \$(jq -n --arg pw \"$USER_PASSWORD\" '{data: {password: \$pw, username: \"postgres\"}}') \\
  && echo \"Secret stored in EU Vault (Frankfurt)\""

section "Verify it's in the vault"

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
echo "  3. Click ${BOLD}secret/${RESET}, then enter this path: ${BOLD}demo/${SESSION_ID}/postgres${RESET}"
echo ""

pause

section "Remove the old K8s Secret and pod"

run_cmd "kubectl delete pod postgres-demo --grace-period=0 --force 2>/dev/null"
run_cmd "kubectl delete secret postgres-credentials"

info "The K8s Secret is gone. The password now lives only in the EU vault."

pause
