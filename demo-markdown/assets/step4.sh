#!/bin/bash
# Step 4: Inspect wrapper injection
source /tmp/helpers.sh

header "Step 4/7: Inspect Wrapper Injection"

info "The CloudTaser webhook mutated the pod at creation."
info "Let's verify the wrapper is in place."

section "Pod annotations — what triggered the injection"

run_cmd "kubectl get pod postgres-demo -o jsonpath='{.metadata.annotations}' | python3 -m json.tool"

info "The cloudtaser.io/* annotations tell the operator to inject the wrapper,
  which vault to connect to, and which secrets to fetch."

pause

section "Container command — wrapper wraps postgres"

run_cmd "kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool"

info "The original postgres entrypoint is now wrapped by cloudtaser-wrapper."
info "It fetches secrets from the EU vault into process memory at startup,
  then executes the original command."

pause
