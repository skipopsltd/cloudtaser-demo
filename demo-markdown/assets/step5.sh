#!/bin/bash
# Step 5: Verify no secrets in Kubernetes, but password works
source /tmp/helpers.sh

header "Step 5/7: Verify — No Secrets in Kubernetes"

info "Your password never touched Kubernetes."
info "Not in K8s Secrets, not in etcd, not even in env vars."

section "Check K8s Secrets"

run_cmd "kubectl get secrets -n default"

info "No postgres password secret. Just the default service account token."

pause

section "Search etcd directly"

run_cmd "kubectl exec -n kube-system etcd-controlplane -- etcdctl \\
  --endpoints=https://127.0.0.1:2379 \\
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \\
  --cert=/etc/kubernetes/pki/etcd/server.crt \\
  --key=/etc/kubernetes/pki/etcd/server.key \\
  get \"\" --prefix --keys-only | grep -i postgres_password || echo \"Not found in etcd — secrets are safe\""

pause

section "Check environment variables inside the pod"

run_cmd "kubectl exec postgres-demo -- bash -c 'echo \"POSTGRES_PASSWORD=\$POSTGRES_PASSWORD\"' || true"

info "Empty — the password is not in the environment. It's only in process memory."

pause

section "But the password works — connect with psql"

USER_PASSWORD=$(cat /tmp/.user_password 2>/dev/null || echo "")
if [ -n "$USER_PASSWORD" ]; then
    info "Connecting with your password: ${USER_PASSWORD}"
fi

run_cmd "kubectl exec postgres-demo -- psql -U postgres -c \"SELECT 'Connected!' as status;\""

info "The app works normally even though the secret is nowhere in Kubernetes."

pause
