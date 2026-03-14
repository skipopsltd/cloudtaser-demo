# Confirm Secrets Work Inside the Pod

Even though secrets aren't in Kubernetes, the application has full access to them via environment variables.

**Exec into the pod and check:**

```bash
kubectl exec -it postgres-demo -- bash -c 'echo "PGPASSWORD is set: ${PGPASSWORD:+yes}"'
```

You should see `PGPASSWORD is set: yes` — the wrapper injected the secret from the EU vault into the process environment.

**Verify PostgreSQL is using the password:**

```bash
kubectl exec -it postgres-demo -- psql -U "$POSTGRES_USER" -c "SELECT 'Connected successfully' as status;"
```

The application works exactly as if you'd used a Kubernetes Secret — but the secret never left EU jurisdiction until it was loaded into memory.
