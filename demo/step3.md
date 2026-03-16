# Confirm Secrets Work Inside the Pod

Even though secrets aren't in Kubernetes, the application has full access to them via environment variables.

**Exec into the pod and check:**

```bash
kubectl exec postgres-demo -- bash -c 'echo "POSTGRES_PASSWORD=$POSTGRES_PASSWORD"'
```

You should see the password printed — the wrapper injected the secret from the EU vault into the process environment.

**Verify PostgreSQL is using the credentials:**

```bash
kubectl exec postgres-demo -- psql -U postgres -c "SELECT 'Connected successfully' as status;"
```

The application works exactly as if you'd used a Kubernetes Secret — but the secret never left EU jurisdiction until it was loaded into memory.
