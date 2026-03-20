# eBPF Runtime Enforcement

In step 3 you could read the password from /proc. Now eBPF blocks /proc/environ, /proc/mem, and ptrace — even as root.

Run in the terminal:

```bash
bash /tmp/step7.sh
```{{exec}}
