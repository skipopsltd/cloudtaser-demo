# View the Audit Trail

CloudTaser logs every security event. Let's see the blocked `/proc/environ` read.

**Check the eBPF agent logs:**

```bash
kubectl logs -n cloudtaser-system daemonset/cloudtaser-ebpf --tail=50 | grep -E "ENVIRON|reactive kill"
```

You should see entries like:
```
"msg":"ENVIRON READ DETECTED","pid":1234,"type":"environ_read","severity":"critical"
"msg":"reactive kill: terminating process","pid":1234,"type":"environ_read"
```

Every `/proc/environ` access attempt on a protected process is logged with full context — PID, command name, timestamp, and severity. In production, these events are forwarded to your SIEM/compliance platform, providing the audit trail required by GDPR, NIS2, and DORA.
