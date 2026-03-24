#!/bin/bash
# Step 4: Move password to EU vault, delete old K8s Secret
source /tmp/helpers.sh
step_guard 4

header "Step 4/8: Move Password to EU Vault"

USER_PASSWORD=$(cat /tmp/.user_password 2>/dev/null || echo "")
SESSION_TOKEN=$(cat /tmp/.cloudtaser-session-token)

section "Write password to vault"

run_cmd "kubectl -n cloudtaser-vault exec vault -- sh -c 'export VAULT_ADDR=http://127.0.0.1:8200 VAULT_TOKEN=${SESSION_TOKEN}; vault kv put secret/demo/postgres password=${USER_PASSWORD} username=postgres' && echo 'Secret stored in vault'"

section "Read it back"

run_cmd "kubectl -n cloudtaser-vault exec vault -- sh -c 'export VAULT_ADDR=http://127.0.0.1:8200 VAULT_TOKEN=${SESSION_TOKEN}; vault kv get -format=json secret/demo/postgres' | python3 -c \"import sys,json; d=json.load(sys.stdin)['data']['data']; print('Password in vault: ' + d['password'])\""

pause

section "Delete K8s Secret and pod"

run_cmd "kubectl delete pod postgres-demo --grace-period=0 --force 2>/dev/null"
run_cmd "kubectl delete secret postgres-credentials"

info "K8s Secret gone. Password now lives only in the vault."
info "In production, this vault runs in the EU — the cloud provider never sees the keys."

pause
