# Confirm Secrets Work Inside the Pod

Even though secrets aren't in Kubernetes, the application has full access to them. PostgreSQL requires `POSTGRES_PASSWORD` to start — and it's running:

```bash
kubectl exec postgres-demo -- psql -U postgres -c "SELECT 'Connected successfully' as status;"
```

The wrapper injected `POSTGRES_PASSWORD` directly into the postgres process memory. You can see it in the wrapper's startup logs:

```bash
kubectl logs postgres-demo -c postgres | head -5
```

Notice `secrets loaded` and `secret_count:2` — the password and username were fetched from Vault and passed to postgres at startup, without ever being stored in Kubernetes.
