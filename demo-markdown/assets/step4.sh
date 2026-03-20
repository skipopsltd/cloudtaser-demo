#!/bin/bash
# Step 4: Move password to EU vault, redeploy with CloudTaser
source /tmp/helpers.sh
step_guard 4

header "Step 4/7: Fix It — Move to EU Vault + CloudTaser"

USER_PASSWORD=$(cat /tmp/.user_password 2>/dev/null || echo "")

info "Now let's fix this. We'll move the password to an EU-hosted OpenBao vault"
info "and let CloudTaser inject it directly into process memory."

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
echo "  3. Click ${BOLD}secret/${RESET} → ${BOLD}demo/${RESET} → ${BOLD}${SESSION_ID}/${RESET} → ${BOLD}postgres${RESET}"
echo ""

pause

section "Remove the old K8s Secret and pod"

run_cmd "kubectl delete pod postgres-demo --grace-period=0 --force 2>/dev/null"
run_cmd "kubectl delete secret postgres-credentials"

info "The K8s Secret is gone. The password now lives only in the EU vault."

pause

section "Redeploy with CloudTaser annotations"

run_cmd "cat /tmp/postgres-demo.yaml"

info "No secrets in the manifest — just annotations telling CloudTaser where"
info "to fetch them from. The operator webhook injects the wrapper automatically."

pause

run_cmd "kubectl apply -f /tmp/postgres-demo.yaml"
run_cmd "kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s"

section "Verify the wrapper is injected"

run_cmd "kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool"

info "The original postgres entrypoint is now wrapped by cloudtaser-wrapper."
info "It fetches secrets from the EU vault into process memory at startup."

pause
