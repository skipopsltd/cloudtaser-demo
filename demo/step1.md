# Deploy a Protected PostgreSQL Pod

Create a PostgreSQL deployment with CloudTaser annotations. These annotations tell the operator to inject the secret-fetching wrapper.

```bash
cat <<'EOF' | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: postgres-demo
  namespace: default
  annotations:
    cloudtaser.io/inject: "true"
    cloudtaser.io/vault-address: "http://vault.vault.svc:8200"
    cloudtaser.io/vault-role: "postgres-demo"
    cloudtaser.io/secret-paths: "secret/data/demo/postgres"
    cloudtaser.io/env-map: "password=PGPASSWORD,username=POSTGRES_USER"
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

Notice that you didn't specify `PGPASSWORD` anywhere in the pod spec. No Kubernetes Secret was created. The CloudTaser webhook injected the wrapper, which will fetch the password directly from the EU vault at startup.

```bash
kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool
```

You should see the entrypoint was rewritten to `/cloudtaser/wrapper`.
