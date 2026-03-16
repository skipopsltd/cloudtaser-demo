#!/bin/bash
# CloudTaser S3 Proxy Demo — Pre-setup script
# Runs automatically in background during intro

set -euo pipefail

echo "Installing tools..."
apt-get update -qq && apt-get install -y -qq awscli xxd > /dev/null 2>&1

# Configure AWS CLI for path-style addressing (MinIO compatible)
mkdir -p /root/.aws
cat > /root/.aws/config <<'AWSCFG'
[default]
s3 =
  addressing_style = path
AWSCFG

# Pull images in parallel
docker pull hashicorp/vault:1.15 > /dev/null 2>&1 &
docker pull ghcr.io/skipopsltd/cloudtaser-s3-proxy:v0.2.2-amd64 > /dev/null 2>&1 &
wait

# Start Vault in dev mode (simulates EU-hosted key management)
docker run -d --name eu-vault --network host \
  -e 'VAULT_DEV_ROOT_TOKEN_ID=demo-root-token' \
  -e 'VAULT_DEV_LISTEN_ADDRESS=0.0.0.0:8200' \
  hashicorp/vault:1.15

# Wait for Vault
for i in $(seq 1 30); do
  if docker exec -e VAULT_ADDR=http://127.0.0.1:8200 -e VAULT_TOKEN=demo-root-token eu-vault vault status > /dev/null 2>&1; then
    break
  fi
  sleep 1
done

# Enable Transit secrets engine and create encryption key
docker exec -e VAULT_ADDR=http://127.0.0.1:8200 -e VAULT_TOKEN=demo-root-token eu-vault \
  vault secrets enable transit

docker exec -e VAULT_ADDR=http://127.0.0.1:8200 -e VAULT_TOKEN=demo-root-token eu-vault \
  vault write -f transit/keys/cloudtaser

# Create a unique bucket on play.min.io
BUCKET="cloudtaser-demo-$(date +%s)"

# Store demo environment
cat > /tmp/.demo-env <<DEMOENV
export AWS_ACCESS_KEY_ID=Q3AM3UQ867SPQQA43P2F
export AWS_SECRET_ACCESS_KEY=zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG
export AWS_DEFAULT_REGION=us-east-1
export BUCKET=${BUCKET}
DEMOENV

echo 'source /tmp/.demo-env 2>/dev/null' >> /root/.bashrc
source /tmp/.demo-env

# Create bucket on play.min.io
aws --endpoint-url https://play.min.io s3 mb s3://$BUCKET

# Start CloudTaser S3 Proxy
docker run -d --name cloudtaser-s3proxy --network host \
  -e CLOUDTASER_S3PROXY_S3_ENDPOINT=https://play.min.io \
  -e CLOUDTASER_S3PROXY_S3_REGION=us-east-1 \
  -e VAULT_ADDR=http://127.0.0.1:8200 \
  -e VAULT_TOKEN=demo-root-token \
  -e VAULT_AUTH_METHOD=token \
  -e CLOUDTASER_S3PROXY_TRANSIT_KEY=cloudtaser \
  -e AWS_ACCESS_KEY_ID=Q3AM3UQ867SPQQA43P2F \
  -e AWS_SECRET_ACCESS_KEY=zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG \
  ghcr.io/skipopsltd/cloudtaser-s3-proxy:v0.2.2-amd64

# Wait for proxy health endpoint
for i in $(seq 1 30); do
  if curl -sf http://localhost:8191/healthz > /dev/null 2>&1; then
    break
  fi
  sleep 1
done

touch /tmp/.cloudtaser-setup-done
echo "CloudTaser S3 proxy demo ready!"
