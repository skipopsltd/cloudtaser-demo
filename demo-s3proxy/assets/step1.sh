#!/bin/bash
# Step 1: View the confidential file
source /tmp/helpers.sh
step_guard 1
wait_for_setup

header "Step 1/5: The Confidential File"

section "A file with GDPR-sensitive data"

run_cmd "cat /tmp/confidential-report.txt"

info "Financial data, customer contracts, employee salaries."
info "Under GDPR, this data must be protected — even from the cloud provider."

pause
