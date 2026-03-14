# Deploy a Protected PostgreSQL Pod

Create a PostgreSQL deployment with CloudCondom annotations. These annotations tell the operator to inject the secret-fetching wrapper.

```bash
cat <<'EOF' | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: postgres-demo
  namespace: default
  annotations:
    cloudcondom.io/inject: "true"
    cloudcondom.io/vault-address: "http://vault.vault.svc:8200"
    cloudcondom.io/vault-role: "postgres-demo"
    cloudcondom.io/secret-paths: "secret/data/demo/postgres"
    cloudcondom.io/env-map: "password=PGPASSWORD,username=POSTGRES_USER"
spec:
  serviceAccountName: postgres-demo
  containers:
    - name: postgres
      image: postgres:16
      ports:
        - containerPort: 5432
EOF
```

Wait for the pod to be running:

```bash
kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s
```

Notice that you didn't specify `PGPASSWORD` anywhere in the pod spec. No Kubernetes Secret was created. The CloudCondom webhook injected the wrapper, which will fetch the password directly from the EU vault at startup.

```bash
kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool
```

You should see the entrypoint was rewritten to `/cloudcondom/wrapper`.
