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
run_cmd "kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s"

wait_for_postgres

section "Wrapper is injected"

run_cmd "kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool"

info "Postgres entrypoint is now wrapped. Secrets are fetched from the EU vault into process memory at startup."

pause
