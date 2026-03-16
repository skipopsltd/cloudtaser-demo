#!/bin/bash
# CloudTaser Demo — Pre-setup script
# Runs automatically in background during intro

set -euo pipefail

echo "Waiting for Kubernetes to be ready..."
kubectl wait --for=condition=Ready node --all --timeout=300s

echo "Setting up CloudTaser demo environment..."

# Install Vault in dev mode
helm repo add hashicorp https://helm.releases.hashicorp.com
helm repo update
helm install vault hashicorp/vault \
  --namespace vault --create-namespace \
  --set "server.dev.enabled=true" \
  --set "server.dev.devRootToken=demo-root-token" \
  --wait --timeout=120s

# Wait for vault pod to be fully ready
kubectl -n vault wait --for=condition=Ready pod/vault-0 --timeout=120s

# Configure Vault with demo secrets
kubectl exec -n vault vault-0 -- vault kv put secret/demo/postgres \
  password="CloudTaser-Demo-2026!" \
  username="postgres"

# Enable Kubernetes auth in Vault
kubectl exec -n vault vault-0 -- sh -c '
  vault auth enable kubernetes
  vault write auth/kubernetes/config \
    kubernetes_host="https://$KUBERNETES_PORT_443_TCP_ADDR:443"
  vault policy write postgres-demo - <<POLICY
path "secret/data/demo/postgres" {
  capabilities = ["read"]
}
POLICY
  vault write auth/kubernetes/role/postgres-demo \
    bound_service_account_names=postgres-demo \
    bound_service_account_namespaces=default \
    policies=postgres-demo \
    ttl=1h
'

# Create service account for the demo pod
kubectl create serviceaccount postgres-demo -n default

# Install CloudTaser from public GHCR chart with demo values
helm install cloudtaser oci://ghcr.io/skipopsltd/cloudtaser-helm/cloudtaser \
  --version 0.1.9 \
  --namespace cloudtaser-system --create-namespace \
  -f /tmp/values-demo.yaml \
  --wait --timeout=180s

touch /tmp/.cloudtaser-setup-done
echo "CloudTaser demo environment ready!"
