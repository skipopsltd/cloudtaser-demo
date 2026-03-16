# Deploy a Protected PostgreSQL Pod

Take a look at the pod manifest — note the CloudTaser annotations:

```bash
cat /tmp/postgres-demo.yaml
```

Key annotations:
- `vault-address` points to the **EU Vault** in Frankfurt — not a local instance
- `vault-auth-method: "token"` — the operator will automatically unseal the wrapper

Deploy it:

```bash
kubectl apply -f /tmp/postgres-demo.yaml
```

The operator detects the new pod, creates a scoped child token from its own EU Vault session, and delivers it to the wrapper. Watch it come up:

```bash
kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s
```

That's it — no manual token delivery. The operator was unsealed during setup with an EU Vault session token, and it automatically unseals every wrapper pod it manages.

Notice that you didn't specify `POSTGRES_PASSWORD` anywhere in the pod spec. No Kubernetes Secret was created. The operator delivered a scoped token out-of-band, and the wrapper fetched the password directly from the EU Vault in Frankfurt.

```bash
kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool
```

You should see the entrypoint was rewritten to `/cloudtaser/wrapper`.
