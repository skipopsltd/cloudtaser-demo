#!/bin/bash
# Step 8: Comparison table and credits
source /tmp/helpers.sh
step_guard 8

header "Step 8/8: CloudTaser vs Alternatives"

echo "  +----------------------+---------+----------+----------+---------+------------+"
echo "  |                      |   K8s   | External |  Vault   |   CSI   |            |"
echo "  |                      | Secrets | Secrets  | Injector |  Driver | CloudTaser |"
echo "  +----------------------+---------+----------+----------+---------+------------+"
echo "  | Secrets in etcd      |${RED} YES     ${RESET}|${RED} YES      ${RESET}|${GREEN} NO       ${RESET}|${YELLOW} opt     ${RESET}|${GREEN} NO         ${RESET}|"
echo "  | Secrets on tmpfs     |${RED} YES     ${RESET}|${RED} YES      ${RESET}|${RED} YES      ${RESET}|${RED} YES     ${RESET}|${GREEN} NO         ${RESET}|"
echo "  | /proc/environ blocked|${RED} NO      ${RESET}|${RED} NO       ${RESET}|${RED} NO       ${RESET}|${RED} NO      ${RESET}|${GREEN} YES        ${RESET}|"
echo "  | /proc/mem blocked    |${RED} NO      ${RESET}|${RED} NO       ${RESET}|${RED} NO       ${RESET}|${RED} NO      ${RESET}|${GREEN} YES        ${RESET}|"
echo "  | ptrace blocked       |${RED} NO      ${RESET}|${RED} NO       ${RESET}|${RED} NO       ${RESET}|${RED} NO      ${RESET}|${GREEN} YES        ${RESET}|"
echo "  | Memory encryption    |${RED} NO      ${RESET}|${RED} NO       ${RESET}|${RED} NO       ${RESET}|${RED} NO      ${RESET}|${GREEN} YES        ${RESET}|"
echo "  | Swap protection      |${RED} NO      ${RESET}|${RED} NO       ${RESET}|${RED} NO       ${RESET}|${RED} NO      ${RESET}|${GREEN} YES        ${RESET}|"
echo "  | Core dump excluded   |${RED} NO      ${RESET}|${RED} NO       ${RESET}|${RED} NO       ${RESET}|${RED} NO      ${RESET}|${GREEN} YES        ${RESET}|"
echo "  | Provider can read    |${RED} YES     ${RESET}|${RED} YES      ${RESET}|${RED} YES      ${RESET}|${RED} YES     ${RESET}|${GREEN} NO         ${RESET}|"
echo "  | CLOUD Act resistant  |${RED} NO      ${RESET}|${RED} NO       ${RESET}|${RED} NO       ${RESET}|${RED} NO      ${RESET}|${GREEN} YES        ${RESET}|"
echo "  +----------------------+---------+----------+----------+---------+------------+"
echo ""
echo ""

echo "  ${BOLD}You've seen CloudTaser in action:${RESET}"
echo ""
echo "  ${GREEN}*${RESET} Secrets never touch Kubernetes — no etcd, no K8s Secrets, no disk"
echo "  ${GREEN}*${RESET} Memory protection — memfd_secret + mlock + no core dumps"
echo "  ${GREEN}*${RESET} eBPF blocks /proc/environ, /proc/mem, and ptrace — even as root"
echo "  ${GREEN}*${RESET} Full audit trail — every security event logged for compliance"
echo ""
echo ""
echo "  ${BOLD}Learn more:${RESET}    cloudtaser.io"
echo "  ${BOLD}Contact us:${RESET}    hello@cloudtaser.io"
echo ""
echo ""
echo "  ${DIM}The terminal is still live — feel free to explore the cluster.${RESET}"
echo ""
