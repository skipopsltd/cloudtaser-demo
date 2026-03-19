#!/bin/bash
# CloudTaser Demo — Background setup
# Runs automatically during intro. Does NOT install CloudTaser (step1.sh does that visibly).

set -euo pipefail

EU_VAULT="https://secret.cloudtaser.io"
PROVISIONER_TOKEN="demo-provisioner-v1"

echo "Waiting for Kubernetes to be ready..."
kubectl wait --for=condition=Ready node --all --timeout=300s

echo "Setting up CloudTaser demo environment..."

# Install jq
apt-get update -qq && apt-get install -y -qq jq > /dev/null 2>&1

# Generate unique session ID
SESSION_ID=$(cat /proc/sys/kernel/random/uuid | cut -d- -f1)
echo "$SESSION_ID" > /tmp/.session_id
echo "Session ID: $SESSION_ID"

# Get a scoped session token from the EU Vault (Frankfurt)
echo "Requesting session token from EU Vault..."
SESSION_TOKEN=$(curl -sf -X POST "$EU_VAULT/v1/auth/token/create/demo" \
  -H "X-Vault-Token: $PROVISIONER_TOKEN" \
  -d '{}' | jq -r '.auth.client_token')

if [ -z "$SESSION_TOKEN" ] || [ "$SESSION_TOKEN" = "null" ]; then
  echo "ERROR: Failed to get session token from EU Vault"
  exit 1
fi
echo "$SESSION_TOKEN" > /tmp/.cloudtaser-session-token
echo "Session token acquired (1h TTL, scoped to demo paths)"

# Write demo secrets to session-scoped path in EU Vault
echo "Writing demo secrets to EU Vault (session: $SESSION_ID)..."
curl -sf -X POST "$EU_VAULT/v1/secret/data/demo/$SESSION_ID/postgres" \
  -H "X-Vault-Token: $SESSION_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"data": {"password": "CloudTaser-Demo-2026!", "username": "postgres"}}'

# Template the postgres manifest with session-scoped path
sed -i "s|secret/data/demo/postgres|secret/data/demo/$SESSION_ID/postgres|" /tmp/postgres-demo.yaml

touch /tmp/.cloudtaser-pre-setup-done
echo "Pre-setup complete. Waiting for user to run step1.sh for CloudTaser install."
