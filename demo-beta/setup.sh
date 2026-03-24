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
# Retry up to 10 times — vault may still be starting after infra changes
echo "Requesting session token from EU Vault..."
SESSION_TOKEN=""
for i in $(seq 1 10); do
  SESSION_TOKEN=$(curl -sfk -X POST "$EU_VAULT/v1/auth/token/create/demo" \
    -H "X-Vault-Token: $PROVISIONER_TOKEN" \
    -d '{}' 2>/dev/null | jq -r '.auth.client_token' 2>/dev/null)
  if [ -n "$SESSION_TOKEN" ] && [ "$SESSION_TOKEN" != "null" ]; then
    break
  fi
  echo "Vault not ready, retrying ($i/10)..."
  sleep 3
done

if [ -z "$SESSION_TOKEN" ] || [ "$SESSION_TOKEN" = "null" ]; then
  echo "ERROR: Failed to get session token from EU Vault after 10 attempts"
  exit 1
fi
echo "$SESSION_TOKEN" > /tmp/.cloudtaser-session-token
echo "Session token acquired (1h TTL, scoped to demo paths)"

# Template the postgres manifest with session-scoped path
sed -i "s|secret/data/demo/postgres|secret/data/demo/$SESSION_ID/postgres|" /tmp/postgres-demo.yaml

# Track demo session started
curl -sf --connect-timeout 2 --max-time 5 -X POST "https://t.cloudtaser.io/api/track" \
    -H "Content-Type: application/json" \
    -H "Origin: https://killercoda.com" \
    -H "openpanel-client-id: 3094e171-f235-49ec-87bd-4d5e786a6594" \
    -d "{\"type\":\"track\",\"payload\":{\"name\":\"operator-beta Started\",\"profileId\":\"$SESSION_ID\",\"properties\":{\"demo\":\"operator-beta\",\"session\":\"$SESSION_ID\"}}}" \
    >/dev/null 2>&1 || true

touch /tmp/.cloudtaser-pre-setup-done
echo "Pre-setup complete. Waiting for user to run step1.sh for CloudTaser install."
