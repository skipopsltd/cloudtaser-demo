#!/bin/bash
# Step 5: Summary and comparison
source /tmp/helpers.sh
step_guard 5

header "Step 5/5: CloudTaser S3 Proxy vs Alternatives"

echo "  +---------------------+---------+---------+----------+------------+"
echo "  |                     |   AWS   |   AWS   |  Client  | CloudTaser |"
echo "  |                     |   SSE   | SSE-KMS |   Side   |  S3 Proxy  |"
echo "  +---------------------+---------+---------+----------+------------+"
echo "  | Provider sees data  |${RED} YES     ${RESET}|${RED} YES     ${RESET}|${GREEN} NO       ${RESET}|${GREEN} NO         ${RESET}|"
echo "  | Key held by         |${RED} AWS     ${RESET}|${YELLOW} AWS KMS ${RESET}|${GREEN} You      ${RESET}|${GREEN} EU Vault   ${RESET}|"
echo "  | EU key sovereignty  |${RED} NO      ${RESET}|${RED} NO      ${RESET}|${YELLOW} Partial  ${RESET}|${GREEN} YES        ${RESET}|"
echo "  | CLOUD Act resistant |${RED} NO      ${RESET}|${RED} NO      ${RESET}|${YELLOW} Partial  ${RESET}|${GREEN} YES        ${RESET}|"
echo "  | Works with any S3   |${RED} NO      ${RESET}|${RED} NO      ${RESET}|${YELLOW} Depends  ${RESET}|${GREEN} YES        ${RESET}|"
echo "  | No app changes      |${GREEN} YES     ${RESET}|${GREEN} YES     ${RESET}|${RED} NO       ${RESET}|${GREEN} YES        ${RESET}|"
echo "  +---------------------+---------+---------+----------+------------+"
echo ""
echo ""

echo "  ${BOLD}You've seen the CloudTaser S3 Proxy in action:${RESET}"
echo ""
echo "  ${GREEN}*${RESET} Data encrypted before it leaves your application"
echo "  ${GREEN}*${RESET} Cloud provider stores only ciphertext"
echo "  ${GREEN}*${RESET} Encryption keys stay in EU-hosted vault"
echo "  ${GREEN}*${RESET} Transparent to the application — just change the S3 endpoint"
echo ""
echo ""
echo "  ${BOLD}Learn more:${RESET}    cloudtaser.io"
echo "  ${BOLD}Contact us:${RESET}    hello@cloudtaser.io"
echo ""
echo ""
echo "  ${DIM}The terminal is still live — feel free to explore.${RESET}"
echo ""
