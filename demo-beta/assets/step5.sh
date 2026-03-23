#!/bin/bash
# Step 5: Redeploy with CloudTaser annotations
source /tmp/helpers.sh
step_guard 5

header "Step 5/8: Redeploy with CloudTaser"

section "Pod manifest — no secrets, just annotations"

run_cmd "cat /tmp/postgres-demo.yaml"

pause

section "Deploy"

run_cmd "kubectl apply -f /tmp/postgres-demo.yaml"

section "Deliver vault token to wrapper (never touches K8s)"

# Wait for the pod to be running (init container done, wrapper started in sealed mode)
echo "Waiting for wrapper to start..."
kubectl wait --for=condition=Initialized pod/postgres-demo --timeout=60s >/dev/null 2>&1
sleep 3

SESSION_TOKEN=$(cat /tmp/.cloudtaser-session-token)

# Port-forward to the wrapper's health endpoint and POST the token
kubectl port-forward pod/postgres-demo 8199:8199 &>/dev/null &
PF_PID=$!
sleep 2

run_cmd "curl -sf -X POST http://localhost:8199/v1/unseal \\
  -H \"Content-Type: application/json\" \\
  -d '{\"token\":\"${SESSION_TOKEN}\"}' && echo 'Token delivered to wrapper'"

kill $PF_PID 2>/dev/null || true

info "The vault token was delivered directly to process memory via the wrapper API."
info "It never touched etcd, K8s Secrets, or any disk."

run_cmd "kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s"

wait_for_postgres

section "Wrapper is injected"

run_cmd "kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool"

info "Postgres entrypoint is now wrapped. Secrets are fetched from the EU vault into process memory at startup."

pause
