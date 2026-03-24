#!/bin/bash
# CloudTaser S3 Proxy Demo — Background setup
# Runs automatically during intro

exec > /tmp/.setup-log 2>&1
set -euxo pipefail

EU_VAULT="https://secret.cloudtaser.io"
PROVISIONER_TOKEN="demo-provisioner-v1"

# Generate unique session ID for tracking
echo "$(date +%s)-$(head -c 4 /dev/urandom | od -An -tx1 | tr -d ' \n')" > /tmp/.session_id

echo "Installing tools..."
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq jq xxd

# MinIO Client (with timeout and retry)
for attempt in 1 2 3; do
  curl -sfL --connect-timeout 10 --max-time 60 \
    https://dl.min.io/client/mc/release/linux-amd64/mc -o /usr/local/bin/mc && break
  echo "mc download attempt $attempt failed, retrying..."
  sleep 2
done
chmod +x /usr/local/bin/mc

# Pull S3 proxy image
docker pull ghcr.io/skipopsltd/cloudtaser-s3-proxy:v0.2.2-amd64 &

# Get scoped session token from EU Vault (Frankfurt)
SESSION_TOKEN=$(curl -sf -X POST "$EU_VAULT/v1/auth/token/create/demo" \
  -H "X-Vault-Token: $PROVISIONER_TOKEN" \
  -d '{}' | jq -r '.auth.client_token')

if [ -z "$SESSION_TOKEN" ] || [ "$SESSION_TOKEN" = "null" ]; then
  echo "ERROR: Failed to get session token from EU Vault"
  exit 1
fi

# Create unique bucket on play.min.io
BUCKET="cloudtaser-demo-$(cat /proc/sys/kernel/random/uuid | cut -d- -f1)"

# Store environment
cat > /tmp/.demo-env <<DEMOENV
export BUCKET=${BUCKET}
export EU_VAULT=${EU_VAULT}
DEMOENV

echo 'source /tmp/.demo-env 2>/dev/null' >> /root/.bashrc
source /tmp/.demo-env

# Configure mc alias for direct cloud access
mc alias set cloud https://play.min.io Q3AM3UQ867SPQQA43P2F zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG
mc mb cloud/$BUCKET

# Wait for image pull
wait

# Start CloudTaser S3 Proxy
docker run -d --name cloudtaser-s3proxy --network host \
  -e CLOUDTASER_S3PROXY_S3_ENDPOINT=https://play.min.io \
  -e CLOUDTASER_S3PROXY_S3_REGION=us-east-1 \
  -e VAULT_ADDR=$EU_VAULT \
  -e VAULT_TOKEN=$SESSION_TOKEN \
  -e VAULT_AUTH_METHOD=token \
  -e CLOUDTASER_S3PROXY_TRANSIT_KEY=cloudtaser \
  -e AWS_ACCESS_KEY_ID=Q3AM3UQ867SPQQA43P2F \
  -e AWS_SECRET_ACCESS_KEY=zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG \
  ghcr.io/skipopsltd/cloudtaser-s3-proxy:v0.2.2-amd64

# Wait for proxy health
for i in $(seq 1 30); do
  if curl -sf http://localhost:8191/healthz > /dev/null 2>&1; then
    break
  fi
  sleep 1
done

# Set proxy mc alias
mc alias set proxy http://localhost:8190 Q3AM3UQ867SPQQA43P2F zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG

# Track demo session started
_SID=$(cat /tmp/.session_id 2>/dev/null || echo "unknown")
curl -sf --connect-timeout 2 --max-time 5 -X POST "https://t.cloudtaser.io/api/track" \
    -H "Content-Type: application/json" \
    -H "Origin: https://killercoda.com" \
    -H "openpanel-client-id: 3094e171-f235-49ec-87bd-4d5e786a6594" \
    -d "{\"type\":\"track\",\"payload\":{\"name\":\"demo_started\",\"properties\":{\"demo\":\"s3-proxy\",\"session\":\"$_SID\"}}}" \
    >/dev/null 2>&1 || true

touch /tmp/.cloudtaser-setup-done
echo "CloudTaser S3 proxy demo ready!"
