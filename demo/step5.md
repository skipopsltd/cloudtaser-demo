# View the Audit Trail

CloudTaser logs every security event. Let's see the blocked `/proc/environ` read.

**Check the eBPF agent logs:**

```bash
kubectl logs -n cloudtaser-system daemonset/cloudtaser-ebpf --tail=20 | python3 -c "
import sys, json
for line in sys.stdin:
    try:
        evt = json.loads(line.strip())
        if 'ENVIRON' in evt.get('msg', '') or 'SECRET' in evt.get('msg', ''):
            print(json.dumps(evt, indent=2))
    except:
        pass
" || kubectl logs -n cloudtaser-system daemonset/cloudtaser-ebpf --tail=5
```

You should see an event like:
```json
{
  "msg": "ENVIRON READ DETECTED",
  "pid": 1234,
  "type": "environ_read",
  "severity": "critical",
  "matched": true
}
```

Every `/proc/environ` access attempt on a protected process is logged with full context — PID, command name, timestamp, and severity. In production, these events are forwarded to your SIEM/compliance platform, providing the audit trail required by GDPR, NIS2, and DORA.
