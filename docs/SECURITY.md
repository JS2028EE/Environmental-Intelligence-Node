# VIGIL-01 Security Notes

## Access-point credentials

The VIGIL-01 local Wi-Fi AP uses the fixed project SSID `VIGIL-01`, while the AP password is provisioned locally and is **not stored in the public repository**.

Firmware credentials are loaded from:

```text
firmware/Secrets.h
```

`Secrets.h` is listed in `.gitignore` and must not be committed. A safe template is provided as:

```text
firmware/Secrets.h.example
```

For a local build:

```text
copy firmware/Secrets.h.example to firmware/Secrets.h
edit AP_PASSWORD
compile/upload
```

If `Secrets.h` is absent, the firmware uses `CHANGE_ME_BEFORE_DEPLOYMENT` as a compile-safe placeholder. That placeholder must be replaced before deploying the device.

## Scope

VIGIL-01 is a local educational prototype. The dashboard is served by the ESP32 itself and is not designed as an internet-facing service. Credential separation prevents the repository from publishing the device's deployment password, but it does not provide authentication, encryption, user accounts, or a hardened production network service.

## Repository hygiene

The repository also ignores Arduino build artifacts and editor/OS files. A project license is provided separately in `LICENSE`.
