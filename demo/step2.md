# Verify Secrets Are Not in Kubernetes

The whole point of CloudTaser is that secrets never touch Kubernetes storage. Let's verify.

**Check for any Kubernetes Secrets in the namespace:**

```bash
kubectl get secrets -n default
```

You should see only the default service account token — no `PGPASSWORD`, no database credentials.

**Search etcd directly (if accessible):**

```bash
kubectl exec -n kube-system etcd-controlplane -- etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  get "" --prefix --keys-only | grep -i pgpassword || echo "Not found in etcd - secrets are safe"
```

**Check environment variables in the pod spec:**

```bash
kubectl get pod postgres-demo -o json | python3 -c "
import sys, json
pod = json.load(sys.stdin)
for c in pod['spec']['containers']:
    for e in c.get('env', []):
        if 'PASSWORD' in e.get('name', '').upper() or 'SECRET' in e.get('name', '').upper():
            print(f'FOUND: {e[\"name\"]}')
            sys.exit(1)
print('No secrets in pod spec - CloudTaser is working')
"
```

The secrets exist only in the wrapper's process memory, fetched directly from the EU vault.
