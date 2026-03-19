#!/bin/bash
# Step 1: Install CloudTaser operator and eBPF daemonset
source /tmp/helpers.sh
wait_for_setup

header "Step 1/7: Install CloudTaser"

info "Installing the CloudTaser operator and eBPF daemonset from the public Helm chart."

section "Helm install"

run_cmd "helm install cloudtaser oci://ghcr.io/skipopsltd/cloudtaser-helm/cloudtaser \\
  --version 0.1.20 \\
  --namespace cloudtaser-system --create-namespace \\
  -f /tmp/values-demo.yaml \\
  --wait --timeout=180s"

pause

section "Unseal the operator with the EU vault session token"

info "The operator needs the vault token to auto-unseal wrapper pods."

OPERATOR_POD=$(kubectl -n cloudtaser-system get pod -l control-plane=controller-manager -o jsonpath='{.items[0].metadata.name}')

run_cmd "kubectl -n cloudtaser-system port-forward pod/${OPERATOR_POD} 8199:8199 &>/dev/null &"
PF_PID=$!
sleep 2

run_cmd "curl -sf -X POST http://localhost:8199/v1/unseal \\
  -H \"Content-Type: application/json\" \\
  -d '{\"token\":\"$(cat /tmp/.cloudtaser-session-token)\"}'"

kill $PF_PID 2>/dev/null || true

pause

section "Verify installation"

run_cmd "kubectl get pods -n cloudtaser-system"

info "CloudTaser is installed: operator + webhook + eBPF daemonset."

pause
