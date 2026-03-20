#!/bin/bash
# Step 2: Deploy PostgreSQL the traditional way with a K8s Secret
source /tmp/helpers.sh
step_guard 2

header "Step 2/8: Deploy PostgreSQL (Traditional Way)"

info "Standard Kubernetes pattern: password in a K8s Secret, referenced by the pod."

echo ""
echo -n "  ${BOLD}Choose a password for PostgreSQL: ${RESET}"
read -r USER_PASSWORD

while [ -z "$USER_PASSWORD" ]; do
    echo -n "  ${BOLD}Password cannot be empty. Try again: ${RESET}"
    read -r USER_PASSWORD
done

echo "$USER_PASSWORD" > /tmp/.user_password
echo ""

section "Create K8s Secret and deploy"

run_cmd "kubectl create secret generic postgres-credentials \\
  --from-literal=POSTGRES_PASSWORD='${USER_PASSWORD}' \\
  --from-literal=POSTGRES_USER=postgres"

run_cmd "cat /tmp/postgres-traditional.yaml"

run_cmd "kubectl apply -f /tmp/postgres-traditional.yaml"

run_cmd "kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s"

wait_for_postgres

pause

section "Write a record to the database"

run_cmd "kubectl exec postgres-demo -- bash -c \\
  \"PGPASSWORD=\\\$POSTGRES_PASSWORD psql -h 127.0.0.1 -U postgres -c \\
  'CREATE TABLE demo_data (id SERIAL, message TEXT, created_at TIMESTAMP DEFAULT NOW());'\""

run_cmd "kubectl exec postgres-demo -- bash -c \\
  \"PGPASSWORD=\\\$POSTGRES_PASSWORD psql -h 127.0.0.1 -U postgres -c \\
  \\\"INSERT INTO demo_data (message) VALUES ('Created BEFORE CloudTaser migration');\\\"\""

run_cmd "kubectl exec postgres-demo -- bash -c \\
  \"PGPASSWORD=\\\$POSTGRES_PASSWORD psql -h 127.0.0.1 -U postgres -c \\
  'SELECT * FROM demo_data;'\""

info "Working database with persistent storage. Now let's check how secure this really is."

pause
