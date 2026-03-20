#!/bin/bash
# Step 2: Deploy PostgreSQL the traditional way with a K8s Secret
source /tmp/helpers.sh
step_guard 2

header "Step 2/8: Deploy PostgreSQL (Traditional Way)"

info "First, let's deploy PostgreSQL the way most teams do it today:"
info "store the password in a Kubernetes Secret and reference it from the pod."

echo ""
echo -n "  ${BOLD}Choose a password for PostgreSQL: ${RESET}"
read -r USER_PASSWORD

while [ -z "$USER_PASSWORD" ]; do
    echo -n "  ${BOLD}Password cannot be empty. Try again: ${RESET}"
    read -r USER_PASSWORD
done

echo "$USER_PASSWORD" > /tmp/.user_password
echo ""

section "Create a K8s Secret with your password"

run_cmd "kubectl create secret generic postgres-credentials \\
  --from-literal=POSTGRES_PASSWORD='${USER_PASSWORD}' \\
  --from-literal=POSTGRES_USER=postgres"

pause

section "View the pod manifest — references the secret"

run_cmd "cat /tmp/postgres-traditional.yaml"

info "Standard pattern: envFrom pulls all keys from the K8s Secret into env vars."
info "Data is stored on a persistent volume so it survives pod restarts."

pause

section "Deploy the pod"

run_cmd "kubectl apply -f /tmp/postgres-traditional.yaml"

run_cmd "kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s"

wait_for_postgres

section "Verify it works and write a record"

info "PostgreSQL requires authentication — we use the password from the env var."

run_cmd "kubectl exec postgres-demo -- bash -c \\
  \"PGPASSWORD=\\\$POSTGRES_PASSWORD psql -h 127.0.0.1 -U postgres -c \\
  'CREATE TABLE demo_data (id SERIAL, message TEXT, created_at TIMESTAMP DEFAULT NOW());'\""

run_cmd "kubectl exec postgres-demo -- bash -c \\
  \"PGPASSWORD=\\\$POSTGRES_PASSWORD psql -h 127.0.0.1 -U postgres -c \\
  \\\"INSERT INTO demo_data (message) VALUES ('Created BEFORE CloudTaser migration');\\\"\""

run_cmd "kubectl exec postgres-demo -- bash -c \\
  \"PGPASSWORD=\\\$POSTGRES_PASSWORD psql -h 127.0.0.1 -U postgres -c \\
  'SELECT * FROM demo_data;'\""

info "PostgreSQL is running with your password and has data stored."
info "Let's see what's actually happening under the hood..."

pause
