#!/bin/bash
# CloudCondom Demo — Pre-setup script
# This runs before the user sees the environment

set -euo pipefail

echo "Setting up CloudCondom demo environment..."

# Install Vault in dev mode
helm repo add hashicorp https://helm.releases.hashicorp.com
helm repo update
helm install vault hashicorp/vault \
  --namespace vault --create-namespace \
  --set "server.dev.enabled=true" \
  --set "server.dev.devRootToken=demo-root-token" \
  --wait --timeout=120s

# Configure Vault with demo secrets
kubectl exec -n vault vault-0 -- vault kv put secret/demo/postgres \
  password="CloudCondom-Demo-2026!" \
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

# Install CloudCondom from bundled chart with demo values
# Images are pulled from ghcr.io/skipopsltd (public packages)
helm install cloudcondom /tmp/chart \
  --namespace cloudcondom-system --create-namespace \
  -f /tmp/values-demo.yaml \
  --wait --timeout=180s

echo "CloudCondom demo environment ready!"
