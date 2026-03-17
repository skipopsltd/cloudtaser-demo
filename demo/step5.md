# View the Audit Trail

CloudTaser logs every security event. Let's see the blocked `/proc/environ` read.

**Check the eBPF agent logs:**

```bash
kubectl logs -n cloudtaser-system daemonset/cloudtaser-ebpf --tail=50 | grep -E "ENVIRON|blocked"
```

You should see entries like:
```
"msg":"ENVIRON READ DETECTED","severity":"critical","blocked":true
```

The `"blocked":true` field confirms the read was **synchronously denied** at the kernel level — no data was returned. Every access attempt is logged with full context: PID, command name, timestamp, and severity. In production, these events are forwarded to your SIEM/compliance platform, providing the audit trail required by GDPR, NIS2, and DORA.
