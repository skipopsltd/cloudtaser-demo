#!/bin/bash
# Step 6: Verify secrets are gone from K8s, but password and data still work
source /tmp/helpers.sh
step_guard 6

header "Step 6/8: Secrets Gone, Data Intact"

USER_PASSWORD=$(cat /tmp/.user_password 2>/dev/null || echo "")

section "K8s Secrets and etcd"

run_cmd "kubectl get secrets -n default"

run_cmd "kubectl exec -n kube-system etcd-controlplane -- etcdctl \\
  --endpoints=https://127.0.0.1:2379 \\
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \\
  --cert=/etc/kubernetes/pki/etcd/server.crt \\
  --key=/etc/kubernetes/pki/etcd/server.key \\
  get \"\" --prefix --keys-only | grep -i postgres || echo \"Not found in etcd\""

pause

section "Environment variables"

run_cmd "kubectl exec postgres-demo -- bash -c 'echo \"POSTGRES_PASSWORD=\$POSTGRES_PASSWORD\"' || true"

info "Empty. The password only exists in process memory."

pause

section "Password still works"

wait_for_postgres

run_cmd "kubectl exec postgres-demo -- bash -c \\
  \"PGPASSWORD='${USER_PASSWORD}' psql -h 127.0.0.1 -U postgres -c \\\"SELECT 'Connected!' as status;\\\"\""

section "Data survived the migration"

run_cmd "kubectl exec postgres-demo -- bash -c \\
  \"PGPASSWORD='${USER_PASSWORD}' psql -h 127.0.0.1 -U postgres -c 'SELECT * FROM demo_data;'\""

info "Same data, same password — secret never touched Kubernetes."

pause
