# Deploy a Protected PostgreSQL Pod

Take a look at the pod manifest — note the CloudTaser annotations:

```bash
cat /tmp/postgres-demo.yaml
```

Deploy it:

```bash
kubectl apply -f /tmp/postgres-demo.yaml
```

Wait for the pod to be running:

```bash
kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s
```

Notice that you didn't specify `POSTGRES_PASSWORD` anywhere in the pod spec. No Kubernetes Secret was created. The CloudTaser webhook injected the wrapper, which will fetch the password directly from the EU vault at startup.

```bash
kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool
```

You should see the entrypoint was rewritten to `/cloudtaser/wrapper`.
