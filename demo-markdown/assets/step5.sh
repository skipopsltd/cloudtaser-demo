#!/bin/bash
# Step 5: Verify secrets are gone from K8s, but password and data still work
source /tmp/helpers.sh
step_guard 5

header "Step 5/7: Verify — Secrets Gone, Data Intact"

info "Same password, same data, same PostgreSQL — but now the secret is nowhere in Kubernetes."

section "K8s Secrets"

run_cmd "kubectl get secrets -n default"

info "No postgres-credentials secret. Just the default service account token."

pause

section "Search etcd"

run_cmd "kubectl exec -n kube-system etcd-controlplane -- etcdctl \\
  --endpoints=https://127.0.0.1:2379 \\
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \\
  --cert=/etc/kubernetes/pki/etcd/server.crt \\
  --key=/etc/kubernetes/pki/etcd/server.key \\
  get \"\" --prefix --keys-only | grep -i postgres || echo \"Not found in etcd\""

pause

section "Environment variables inside the pod"

run_cmd "kubectl exec postgres-demo -- bash -c 'echo \"POSTGRES_PASSWORD=\$POSTGRES_PASSWORD\"' || true"

info "Empty — the password is not in the environment. It only exists in process memory."

pause

section "But the password still works"

run_cmd "kubectl exec postgres-demo -- psql -U postgres -c \"SELECT 'Connected!' as status;\""

pause

section "And your data survived the migration"

run_cmd "kubectl exec postgres-demo -- psql -U postgres -c \"SELECT * FROM demo_data;\""

info "Same data, same password — but the secret never touched Kubernetes."
info "CloudTaser is a drop-in replacement. No data loss, no downtime risk."

pause
