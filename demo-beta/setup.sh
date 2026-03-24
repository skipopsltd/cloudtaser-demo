#!/bin/bash
# CloudTaser Demo — Background setup
# Deploys in-cluster OpenBao vault (KillerCoda pods can't reach external internet)

set -euo pipefail

echo "Waiting for Kubernetes to be ready..."
kubectl wait --for=condition=Ready node --all --timeout=300s

echo "Setting up CloudTaser demo environment..."
apt-get update -qq && apt-get install -y -qq jq > /dev/null 2>&1

# Deploy in-cluster OpenBao vault
echo "Deploying in-cluster vault..."
kubectl create namespace cloudtaser-vault 2>/dev/null || true

cat <<'EOF' | kubectl -n cloudtaser-vault apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: vault
  labels:
    app: vault
spec:
  containers:
    - name: vault
      image: hashicorp/vault:1.19
      args: ["server", "-dev", "-dev-root-token-id=demo-root-token", "-dev-listen-address=0.0.0.0:8200"]
      ports:
        - containerPort: 8200
      env:
        - name: VAULT_DEV_ROOT_TOKEN_ID
          value: "demo-root-token"
      readinessProbe:
        httpGet:
          path: /v1/sys/health
          port: 8200
        initialDelaySeconds: 3
        periodSeconds: 3
---
apiVersion: v1
kind: Service
metadata:
  name: vault
spec:
  selector:
    app: vault
  ports:
    - port: 8200
      targetPort: 8200
EOF

echo "Waiting for vault to be ready..."
kubectl -n cloudtaser-vault wait --for=condition=Ready pod/vault --timeout=120s

# Configure vault
echo "Configuring vault..."
kubectl -n cloudtaser-vault exec vault -- sh -c '
  export VAULT_ADDR=http://127.0.0.1:8200 VAULT_TOKEN=demo-root-token
  vault secrets enable -path=secret kv-v2 2>/dev/null || true
  vault policy write demo - <<POLICY
path "secret/data/*" { capabilities = ["create", "read", "update", "delete", "list"] }
path "auth/token/create" { capabilities = ["update"] }
POLICY
  vault token create -policy=demo -ttl=1h -id=demo-session-token 2>/dev/null || true
' >/dev/null 2>&1

VAULT_ADDR="http://vault.cloudtaser-vault.svc:8200"

# Save for use by step scripts
echo "$VAULT_ADDR" > /tmp/.vault_addr
echo "demo-session-token" > /tmp/.cloudtaser-session-token
echo "demo" > /tmp/.session_id

# Update postgres-demo.yaml vault address
sed -i "s|https://secret.cloudtaser.io|${VAULT_ADDR}|" /tmp/postgres-demo.yaml
# Remove vault-skip-verify annotation (not needed for HTTP)
sed -i '/vault-skip-verify/d' /tmp/postgres-demo.yaml

touch /tmp/.cloudtaser-pre-setup-done
echo "Pre-setup complete. In-cluster vault running at ${VAULT_ADDR}"
