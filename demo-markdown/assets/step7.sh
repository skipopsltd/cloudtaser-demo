#!/bin/bash
# Step 7: Comparison table and credits
source /tmp/helpers.sh

header "Step 7/7: CloudTaser vs Alternatives"

RED='\033[31m'
GREEN='\033[32m'
YELLOW='\033[33m'
BOLD='\033[1m'
RESET='\033[0m'

echo -e "  +---------------------+------+------+-------+------+-----------+"
echo -e "  |                     | K8s  | Ext  | Vault | CSI  |  Cloud    |"
echo -e "  |                     | Sec  | Sec  | Injct | Drv  |  Taser    |"
echo -e "  +---------------------+------+------+-------+------+-----------+"
echo -e "  | Secrets in etcd     |${RED} YES  ${RESET}|${RED} YES  ${RESET}|${GREEN}  NO   ${RESET}|${YELLOW} opt  ${RESET}|${GREEN}    NO     ${RESET}|"
echo -e "  | Secrets on tmpfs    |${RED} YES  ${RESET}|${RED} YES  ${RESET}|${RED} YES   ${RESET}|${RED} YES  ${RESET}|${GREEN}    NO     ${RESET}|"
echo -e "  | /proc blocked       |${RED}  NO  ${RESET}|${RED}  NO  ${RESET}|${RED}  NO   ${RESET}|${RED}  NO  ${RESET}|${GREEN}   YES     ${RESET}|"
echo -e "  | Runtime enforcement |${RED}  NO  ${RESET}|${RED}  NO  ${RESET}|${RED}  NO   ${RESET}|${RED}  NO  ${RESET}|${GREEN}   YES     ${RESET}|"
echo -e "  | Provider can read   |${RED} YES  ${RESET}|${RED} YES  ${RESET}|${RED} YES   ${RESET}|${RED} YES  ${RESET}|${GREEN}    NO     ${RESET}|"
echo -e "  | CLOUD Act resistant |${RED}  NO  ${RESET}|${RED}  NO  ${RESET}|${RED}  NO   ${RESET}|${RED}  NO  ${RESET}|${GREEN}   YES     ${RESET}|"
echo -e "  +---------------------+------+------+-------+------+-----------+"
echo ""
echo ""

echo -e "  ${BOLD}You've seen CloudTaser in action:${RESET}"
echo ""
echo -e "  ${GREEN}*${RESET} Secrets never touch Kubernetes — no etcd, no K8s Secrets, no disk"
echo -e "  ${GREEN}*${RESET} Applications work normally — secrets available in process memory"
echo -e "  ${GREEN}*${RESET} eBPF enforcement blocks /proc/environ at kernel level — even as root"
echo -e "  ${GREEN}*${RESET} Full audit trail — every security event logged for compliance"
echo ""
echo ""
echo -e "  ${BOLD}Learn more:${RESET}    cloudtaser.io"
echo -e "  ${BOLD}Contact us:${RESET}    hello@cloudtaser.io"
echo ""
echo ""
echo -e "  ${DIM}The terminal is still live — feel free to explore the cluster.${RESET}"
echo ""
