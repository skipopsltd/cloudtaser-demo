# Deploy a Protected PostgreSQL Pod

Take a look at the pod manifest — note the CloudTaser annotations:

```bash
cat /tmp/postgres-demo.yaml
```

Key annotations:
- `vault-address` points to the **EU Vault** in Frankfurt — not a local instance
- `vault-auth-method: "token"` — no K8s secret reference, so the wrapper starts in **sealed mode**

Deploy it:

```bash
kubectl apply -f /tmp/postgres-demo.yaml
```

The wrapper starts in sealed mode — alive but waiting for a Vault token. Check its status:

```bash
kubectl port-forward pod/postgres-demo 8199:8199 &>/dev/null &
sleep 2
curl -s http://localhost:8199/v1/status | python3 -m json.tool
```

You should see `"sealed": true` — like Vault before it's unsealed. Now deliver the EU Vault session token:

```bash
curl -s -X POST http://localhost:8199/v1/unseal \
  -d '{"token":"'$(cat /tmp/.demo-token)'"}'
```

The wrapper unseals, authenticates to the EU Vault, fetches secrets, and starts PostgreSQL:

```bash
kill %1 2>/dev/null
kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s
```

Notice that you didn't specify `POSTGRES_PASSWORD` anywhere in the pod spec. No Kubernetes Secret was created. The token was delivered out-of-band, and the wrapper fetched the password directly from the EU Vault in Frankfurt.

```bash
kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool
```

You should see the entrypoint was rewritten to `/cloudtaser/wrapper`.
