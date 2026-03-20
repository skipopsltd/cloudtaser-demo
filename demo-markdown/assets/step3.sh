#!/bin/bash
# Step 3: Show the problem — secrets visible everywhere
source /tmp/helpers.sh
step_guard 3

header "Step 3/8: The Problem — Secrets Are Everywhere"

USER_PASSWORD=$(cat /tmp/.user_password 2>/dev/null || echo "")

info "Your password is supposed to be secret. Let's see how secret it really is."

section "1. Kubernetes Secrets are just base64"

run_cmd "kubectl get secret postgres-credentials -o jsonpath='{.data.POSTGRES_PASSWORD}' | base64 -d && echo"

info "Anyone with kubectl access can read it. Base64 is encoding, not encryption."

pause

section "2. Stored in plain text in etcd"

run_cmd "kubectl exec -n kube-system etcd-controlplane -- etcdctl \\
  --endpoints=https://127.0.0.1:2379 \\
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \\
  --cert=/etc/kubernetes/pki/etcd/server.crt \\
  --key=/etc/kubernetes/pki/etcd/server.key \\
  get /registry/secrets/default/postgres-credentials --print-value-only | strings | grep -E '${USER_PASSWORD}' || echo '(search etcd for your password)'"

info "etcd stores K8s Secrets in plain text (unless encryption-at-rest is configured — most clusters don't)."

pause

section "3. Visible as environment variables inside the pod"

run_cmd "kubectl exec postgres-demo -- env | grep POSTGRES_PASSWORD"

info "Any process in the container can read it. Any sidecar too."

pause

section "4. Readable from the host via /proc/environ"

run_cmd "sudo cat /proc/\$(pgrep -x postgres | head -1)/environ 2>/dev/null | tr '\\0' '\\n' | grep POSTGRES_PASSWORD || echo 'Could not find process'"

info "Anyone with host access (or a compromised container with hostPID) can read"
info "every secret from every pod on the node. This is the attack surface."
info ""
info "Under CLOUD Act and FISA 702, the US cloud provider can be compelled"
info "to hand over this data — including your secrets."

pause
