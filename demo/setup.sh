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

# Save token for the unseal step
echo "$SESSION_TOKEN" > /tmp/.demo-token

# Install CloudTaser from public GHCR chart
helm install cloudtaser oci://ghcr.io/skipopsltd/cloudtaser-helm/cloudtaser \
  --version 0.1.16 \
  --namespace cloudtaser-system --create-namespace \
  -f /tmp/values-demo.yaml \
  --wait --timeout=180s

# Create pod manifest with EU Vault token auth (sealed mode)
cat > /tmp/postgres-demo.yaml <<'MANIFEST'
apiVersion: v1
kind: Pod
metadata:
  name: postgres-demo
  namespace: default
  annotations:
    cloudtaser.io/inject: "true"
    cloudtaser.io/ebpf: "true"
    cloudtaser.io/vault-address: "https://secret.cloudtaser.io"
    cloudtaser.io/vault-auth-method: "token"
    cloudtaser.io/secret-paths: "secret/data/demo/postgres"
    cloudtaser.io/env-map: "password=POSTGRES_PASSWORD,username=POSTGRES_USER"
spec:
  containers:
    - name: postgres
      image: postgres:16
      ports:
        - containerPort: 5432
MANIFEST

touch /tmp/.cloudtaser-setup-done
echo "CloudTaser demo environment ready!"
