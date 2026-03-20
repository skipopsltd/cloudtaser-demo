#!/bin/bash
# Step 3: Show the problem — secrets visible everywhere
source /tmp/helpers.sh
step_guard 3

header "Step 3/8: The Problem"

USER_PASSWORD=$(cat /tmp/.user_password 2>/dev/null || echo "")

section "1. K8s Secrets are just base64"

run_cmd "kubectl get secret postgres-credentials -o jsonpath='{.data.POSTGRES_PASSWORD}' | base64 -d && echo"

info "Base64 is encoding, not encryption. Anyone with kubectl access can read this."

pause

section "2. Plaintext in etcd"

run_cmd "kubectl exec -n kube-system etcd-controlplane -- etcdctl \\
  --endpoints=https://127.0.0.1:2379 \\
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \\
  --cert=/etc/kubernetes/pki/etcd/server.crt \\
  --key=/etc/kubernetes/pki/etcd/server.key \\
  get /registry/secrets/default/postgres-credentials --print-value-only | strings | grep -E '${USER_PASSWORD}' || echo '(search etcd for your password)'"

info "Your password, in plain text, stored on disk. Most clusters don't enable etcd encryption."

pause

section "3. Visible in pod environment"

run_cmd "kubectl exec postgres-demo -- env | grep POSTGRES_PASSWORD"

info "Any process or sidecar in this container can read it."

pause

section "4. Readable from the host via /proc"

run_cmd "sudo cat /proc/\$(pgrep -x postgres | head -1)/environ 2>/dev/null | tr '\\0' '\\n' | grep POSTGRES_PASSWORD || echo 'Could not find process'"

info "Host access = read every secret from every pod on the node."
info "Under CLOUD Act / FISA 702, the US provider can be compelled to hand this over."

pause
