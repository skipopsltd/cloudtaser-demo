#!/bin/bash
# Step 4: Move password to EU vault, delete old K8s Secret
source /tmp/helpers.sh
step_guard 4

header "Step 4/8: Move Password to EU Vault"

USER_PASSWORD=$(cat /tmp/.user_password 2>/dev/null || echo "")
SESSION_ID=$(cat /tmp/.session_id)
SESSION_TOKEN=$(cat /tmp/.cloudtaser-session-token)

section "Write password to EU vault (Frankfurt)"

jq -n --arg pw "$USER_PASSWORD" '{data: {password: $pw, username: "postgres"}}' > /tmp/.vault_payload

run_cmd "curl -sf -X POST https://secret.cloudtaser.io/v1/secret/data/demo/${SESSION_ID}/postgres \\
  -H \"X-Vault-Token: ${SESSION_TOKEN}\" \\
  -H \"Content-Type: application/json\" \\
  -d @/tmp/.vault_payload \\
  && echo \"Secret stored in EU Vault (Frankfurt)\""

section "Read it back"

run_cmd "curl -sf https://secret.cloudtaser.io/v1/secret/data/demo/${SESSION_ID}/postgres \\
  -H \"X-Vault-Token: ${SESSION_TOKEN}\" \\
  | python3 -c \"import sys,json; d=json.load(sys.stdin)['data']['data']; print('Password in vault: ' + d['password'])\""

section "Vault UI"

echo "  ${BG_BLUE} https://secret.cloudtaser.io/ui ${RESET}"
echo ""
echo "  1. Choose ${BOLD}Token${RESET} and paste: ${BOLD}${SESSION_TOKEN}${RESET}"
echo "  2. Click ${BOLD}secret/${RESET}, enter path: ${BOLD}demo/${SESSION_ID}/postgres${RESET}"
echo ""

pause

section "Delete K8s Secret and pod"

run_cmd "kubectl delete pod postgres-demo --grace-period=0 --force 2>/dev/null"
run_cmd "kubectl delete secret postgres-credentials"

info "K8s Secret gone. Password now lives only in the EU vault."

pause
