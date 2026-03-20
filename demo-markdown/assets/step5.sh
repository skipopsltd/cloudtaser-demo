#!/bin/bash
# Step 5: Redeploy with CloudTaser annotations
source /tmp/helpers.sh
step_guard 5

header "Step 5/8: Redeploy with CloudTaser"

info "Now we redeploy PostgreSQL with CloudTaser annotations."
info "No secrets in the manifest — the wrapper fetches them from the EU vault."

section "CloudTaser pod manifest"

run_cmd "cat /tmp/postgres-demo.yaml"

info "No passwords, no K8s Secrets — just annotations telling CloudTaser"
info "which vault to connect to and which secrets to fetch."

pause

section "Deploy the pod"

run_cmd "kubectl apply -f /tmp/postgres-demo.yaml"
run_cmd "kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s"

wait_for_postgres

section "Verify the wrapper is injected"

run_cmd "kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool"

info "The original postgres entrypoint is now wrapped by cloudtaser-wrapper."
info "It fetches secrets from the EU vault into process memory at startup."

pause
