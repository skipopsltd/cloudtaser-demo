#!/bin/bash
# Step 2: Deploy a PostgreSQL pod with CloudTaser annotations
source /tmp/helpers.sh

header "Step 2/7: Deploy a Protected PostgreSQL Pod"

info "No secrets in the manifest — they come from the EU vault."
info "The operator webhook will auto-inject the CloudTaser wrapper."

section "Pod manifest"

run_cmd "cat /tmp/postgres-demo.yaml"

info "Notice: no POSTGRES_PASSWORD anywhere. Just annotations telling CloudTaser
  where to fetch secrets from."

pause

section "Deploy the pod"

run_cmd "kubectl apply -f /tmp/postgres-demo.yaml"

run_cmd "kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s"

run_cmd "kubectl get pod postgres-demo -o wide"

info "Pod is running. The webhook mutated it at creation to inject the wrapper."

pause
