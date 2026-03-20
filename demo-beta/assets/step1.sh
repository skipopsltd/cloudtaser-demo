#!/bin/bash
# Step 1: Install CloudTaser operator and eBPF daemonset
source /tmp/helpers.sh
step_guard 1
wait_for_setup

header "Step 1/8: Install CloudTaser"

section "Install operator + eBPF daemonset"

run_cmd "helm install cloudtaser oci://ghcr.io/skipopsltd/cloudtaser-helm/cloudtaser \\
  --version 0.3.0 \\
  --namespace cloudtaser-system --create-namespace \\
  -f /tmp/values-demo.yaml \\
  --wait --timeout=180s"

pause

section "Unseal operator with EU vault token"

# Find the leader pod (the one that acquired the lease)
LEADER_POD=$(kubectl -n cloudtaser-system get lease cloudtaser-operator-leader -o jsonpath='{.spec.holderIdentity}' 2>/dev/null | sed 's/_.*//')
if [ -z "$LEADER_POD" ]; then
  # Fallback: use the first ready pod
  LEADER_POD=$(kubectl -n cloudtaser-system get pod -l control-plane=controller-manager --field-selector=status.phase=Running -o jsonpath='{.items[0].metadata.name}')
fi
echo "Leader pod: ${LEADER_POD}"

run_cmd "kubectl -n cloudtaser-system port-forward pod/${LEADER_POD} 8199:8199 &>/dev/null &"
PF_PID=$!
sleep 2

run_cmd "curl -sf -X POST http://localhost:8199/v1/unseal \\
  -H \"Content-Type: application/json\" \\
  -d '{\"token\":\"$(cat /tmp/.cloudtaser-session-token)\"}'"

kill $PF_PID 2>/dev/null || true

run_cmd "kubectl get pods -n cloudtaser-system"

pause
