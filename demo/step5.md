# View the Audit Trail

CloudCondom logs every security event. Let's see the blocked `/proc/environ` read.

**Check the eBPF agent logs:**

```bash
kubectl logs -n cloudcondom-system daemonset/cloudcondom-ebpf --tail=20 | python3 -c "
import sys, json
for line in sys.stdin:
    try:
        evt = json.loads(line.strip())
        if evt.get('msg') in ['ENVIRON READ BLOCKED', 'SECRET LEAK DETECTED', 'reactive kill: terminating process']:
            print(json.dumps(evt, indent=2))
    except:
        pass
" || kubectl logs -n cloudcondom-system daemonset/cloudcondom-ebpf --tail=5
```

You should see an event like:
```json
{
  "msg": "ENVIRON READ BLOCKED",
  "pid": 1234,
  "type": "environ_read",
  "severity": "critical",
  "blocked": true
}
```

This audit event would be forwarded to your SIEM/compliance platform in production, providing the evidence trail required by GDPR, NIS2, and DORA.
