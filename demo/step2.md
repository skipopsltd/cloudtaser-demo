# Verify Secrets Are Not in Kubernetes

The whole point of CloudTaser is that secrets never touch Kubernetes storage. Let's verify.

**Check for any Kubernetes Secrets in the namespace:**

```bash
kubectl get secrets -n default
```

You should see only the default service account token — no `POSTGRES_PASSWORD`, no database credentials.

**Search etcd directly (if accessible):**

```bash
kubectl exec -n kube-system etcd-controlplane -- etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  get "" --prefix --keys-only | grep -i postgres_password || echo "Not found in etcd - secrets are safe"
```

**Check environment variables in the pod spec:**

```bash
kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].env[*].name}'
```

You'll see CloudTaser metadata vars (`CLOUDTASER_*`) but no `POSTGRES_PASSWORD` — the actual secret is only in process memory, never in the pod spec.

The secrets exist only in the wrapper's process memory, fetched directly from the EU vault.
