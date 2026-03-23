#!/bin/bash
# Step 1: Install CloudTaser operator and eBPF daemonset
source /tmp/helpers.sh
step_guard 1
wait_for_setup

header "Step 1/8: Install CloudTaser"

section "Install operator + eBPF daemonset"

run_cmd "helm repo add cloudtaser https://charts.cloudtaser.io && helm repo update"

run_cmd "helm install cloudtaser cloudtaser/cloudtaser \
  --namespace cloudtaser-system --create-namespace \
  -f /tmp/values-demo.yaml \
  --wait --timeout=180s"

run_cmd "kubectl get pods -n cloudtaser-system"

pause
