#!/bin/bash
# CloudTaser Demo — Pre-setup script
# Runs automatically in background during intro

set -euo pipefail

EU_VAULT="https://secret.cloudtaser.io"
PROVISIONER_TOKEN="demo-provisioner-v1"

echo "Waiting for Kubernetes to be ready..."
kubectl wait --for=condition=Ready node --all --timeout=300s

echo "Setting up CloudTaser demo environment..."

# Get a scoped session token from the EU Vault (Frankfurt)
echo "Requesting session token from EU Vault..."
apt-get update -qq && apt-get install -y -qq jq > /dev/null 2>&1

SESSION_TOKEN=$(curl -sf -X POST "$EU_VAULT/v1/auth/token/create/demo" \
  -H "X-Vault-Token: $PROVISIONER_TOKEN" \
  -d '{}' | jq -r '.auth.client_token')

if [ -z "$SESSION_TOKEN" ] || [ "$SESSION_TOKEN" = "null" ]; then
  echo "ERROR: Failed to get session token from EU Vault"
  exit 1
fi
echo "Session token acquired (1h TTL, scoped to demo paths)"

# Write demo secrets to the EU Vault
echo "Writing demo secrets to EU Vault..."
curl -sf -X POST "$EU_VAULT/v1/secret/data/demo/postgres" \
  -H "X-Vault-Token: $SESSION_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"data": {"password": "CloudTaser-Demo-2026!", "username": "postgres"}}'

# Install CloudTaser from public GHCR chart
helm install cloudtaser oci://ghcr.io/skipopsltd/cloudtaser-helm/cloudtaser \
  --version 0.1.18 \
  --namespace cloudtaser-system --create-namespace \
  -f /tmp/values-demo.yaml \
  --wait --timeout=180s

# Unseal the operator with the EU Vault session token
echo "Unsealing CloudTaser operator..."
OPERATOR_POD=$(kubectl -n cloudtaser-system get pod -l control-plane=controller-manager -o jsonpath='{.items[0].metadata.name}')
kubectl -n cloudtaser-system port-forward "pod/${OPERATOR_POD}" 8199:8199 &>/dev/null &
PF_PID=$!
sleep 2

curl -sf -X POST http://localhost:8199/v1/unseal \
  -H "Content-Type: application/json" \
  -d "{\"token\":\"${SESSION_TOKEN}\"}"

kill $PF_PID 2>/dev/null || true
echo "Operator unsealed — it will auto-unseal wrapper pods"

touch /tmp/.cloudtaser-setup-done
echo "CloudTaser demo environment ready!"
