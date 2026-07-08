# AeroPico Fault-Injection Smoke

Runs the native tests that model critical failure paths:

- watchdog gate blocks unsafe feeding
- RC loss / failsafe behavior
- stale sensor health
- battery low / brownout conditions
- preflight arm blocking

```bash
python3 tools/fault_injection/fault_injection.py
```

This does not replace physical HIL. It is a CI gate for failure semantics.
