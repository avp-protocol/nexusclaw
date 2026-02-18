# NexusClaw AVP Protocol Guide

## Overview

NexusClaw implements the Agent Vault Protocol (AVP) v0.1.0 over USB CDC serial interface. Commands are sent as JSON strings, and responses are returned as JSON.

## Communication

### Connection

NexusClaw appears as a USB CDC device:
- **Linux:** `/dev/ttyACM0`
- **macOS:** `/dev/tty.usbmodem*`
- **Windows:** `COM3` (or similar)

Settings: **115200 baud, 8N1**

### Command Format

Send JSON commands terminated with newline (`\n`):

```
{"op":"DISCOVER"}\n
```

### Response Format

All responses are JSON objects with an `ok` field:

```json
{"ok": true, ...}
{"ok": false, "error": "ERROR_CODE", "message": "Human readable"}
```

---

## Operations

### DISCOVER

Query device capabilities.

**Request:**
```json
{"op": "DISCOVER"}
```

**Response:**
```json
{
  "ok": true,
  "version": "0.1.0",
  "backend_type": "hardware",
  "manufacturer": "AVP Protocol",
  "model": "NexusClaw",
  "serial": "NC00000001",
  "capabilities": {
    "hw_sign": true,
    "hw_attest": true,
    "max_secrets": 32,
    "max_secret_size": 256
  }
}
```

---

### AUTHENTICATE

Start a secure session with PIN.

**Request:**
```json
{
  "op": "AUTHENTICATE",
  "workspace": "default",
  "auth_method": "pin",
  "pin": "123456",
  "requested_ttl": 300
}
```

**Response:**
```json
{
  "ok": true,
  "session_id": "a1b2c3d4e5f6...",
  "expires_in": 300,
  "workspace": "default"
}
```

**Errors:**
- `PIN_INVALID` — Wrong PIN
- `PIN_LOCKED` — Too many failed attempts

---

### STORE

Store a secret value.

**Request:**
```json
{
  "op": "STORE",
  "session_id": "a1b2c3d4e5f6...",
  "name": "anthropic_api_key",
  "value": "sk-ant-..."
}
```

**Response:**
```json
{"ok": true}
```

**Errors:**
- `NOT_AUTHENTICATED` — Session invalid/expired
- `CAPACITY_EXCEEDED` — Storage full

---

### RETRIEVE

Retrieve a stored secret.

**Request:**
```json
{
  "op": "RETRIEVE",
  "session_id": "a1b2c3d4e5f6...",
  "name": "anthropic_api_key"
}
```

**Response:**
```json
{
  "ok": true,
  "value": "sk-ant-..."
}
```

**Errors:**
- `NOT_AUTHENTICATED` — Session invalid/expired
- `SECRET_NOT_FOUND` — Secret doesn't exist

---

### DELETE

Delete a stored secret.

**Request:**
```json
{
  "op": "DELETE",
  "session_id": "a1b2c3d4e5f6...",
  "name": "anthropic_api_key"
}
```

**Response:**
```json
{"ok": true}
```

---

### LIST

List all stored secrets.

**Request:**
```json
{
  "op": "LIST",
  "session_id": "a1b2c3d4e5f6..."
}
```

**Response:**
```json
{
  "ok": true,
  "secrets": ["anthropic_api_key", "openai_api_key", "github_token"]
}
```

---

### ROTATE

Replace a secret value (atomic update).

**Request:**
```json
{
  "op": "ROTATE",
  "session_id": "a1b2c3d4e5f6...",
  "name": "anthropic_api_key",
  "value": "sk-ant-new-..."
}
```

**Response:**
```json
{"ok": true}
```

---

## Hardware Extensions

### HW_CHALLENGE

Verify device authenticity.

**Request:**
```json
{"op": "HW_CHALLENGE"}
```

**Response:**
```json
{
  "ok": true,
  "verified": true,
  "model": "TROPIC01",
  "serial": "NC00000001"
}
```

---

### HW_SIGN

Sign data with a hardware key (key never leaves the device).

**Request:**
```json
{
  "op": "HW_SIGN",
  "session_id": "a1b2c3d4e5f6...",
  "key_name": "signing_key",
  "data": "48656c6c6f"
}
```

Note: `data` is hex-encoded.

**Response:**
```json
{
  "ok": true,
  "signature": "304402..."
}
```

---

### HW_ATTEST

Get signed attestation of device state.

**Request:**
```json
{
  "op": "HW_ATTEST",
  "session_id": "a1b2c3d4e5f6..."
}
```

**Response:**
```json
{
  "ok": true,
  "attestation": "{...signed JSON...}"
}
```

---

## Error Codes

| Code | Description |
|------|-------------|
| `PARSE_ERROR` | Invalid JSON |
| `INVALID_OPERATION` | Unknown operation |
| `INVALID_PARAMETER` | Missing or invalid parameter |
| `NOT_AUTHENTICATED` | Session required |
| `SESSION_EXPIRED` | Session timed out |
| `SECRET_NOT_FOUND` | Secret doesn't exist |
| `CAPACITY_EXCEEDED` | Storage full |
| `HARDWARE_ERROR` | TROPIC01 error |
| `CRYPTO_ERROR` | Cryptographic error |
| `PIN_INVALID` | Wrong PIN |
| `PIN_LOCKED` | Account locked |
| `INTERNAL_ERROR` | Internal error |

---

## Examples

### Python

```python
import serial
import json

class NexusClaw:
    def __init__(self, port="/dev/ttyACM0"):
        self.ser = serial.Serial(port, 115200, timeout=5)
        self.session_id = None

    def send(self, cmd):
        self.ser.write(json.dumps(cmd).encode() + b'\n')
        return json.loads(self.ser.readline())

    def discover(self):
        return self.send({"op": "DISCOVER"})

    def authenticate(self, pin="123456"):
        resp = self.send({
            "op": "AUTHENTICATE",
            "auth_method": "pin",
            "pin": pin,
            "requested_ttl": 300
        })
        if resp.get("ok"):
            self.session_id = resp["session_id"]
        return resp

    def store(self, name, value):
        return self.send({
            "op": "STORE",
            "session_id": self.session_id,
            "name": name,
            "value": value
        })

    def retrieve(self, name):
        resp = self.send({
            "op": "RETRIEVE",
            "session_id": self.session_id,
            "name": name
        })
        if resp.get("ok"):
            return resp["value"]
        raise KeyError(resp.get("error"))

# Usage
vault = NexusClaw()
print(vault.discover())
vault.authenticate("123456")
vault.store("my_key", "secret_value")
print(vault.retrieve("my_key"))
```

### Shell (using jq)

```bash
# Discover
echo '{"op":"DISCOVER"}' > /dev/ttyACM0
cat /dev/ttyACM0 | jq .

# Authenticate
echo '{"op":"AUTHENTICATE","pin":"123456"}' > /dev/ttyACM0

# Store secret
echo '{"op":"STORE","session_id":"...","name":"key","value":"val"}' > /dev/ttyACM0
```

---

## Security Considerations

1. **PIN Protection**: The PIN protects access to stored secrets. After 5 failed attempts, the device locks.

2. **Session Timeout**: Sessions expire after the TTL (default 5 minutes). Re-authenticate to continue.

3. **Secure Data Handling**: Clear sensitive data from memory after use:
   ```python
   api_key = vault.retrieve("key")
   # use api_key...
   api_key = None  # Clear when done
   ```

4. **Hardware Attestation**: Always verify device authenticity before trusting it:
   ```python
   result = vault.send({"op": "HW_CHALLENGE"})
   if not result.get("verified"):
       raise SecurityError("Device not verified!")
   ```

5. **Key Never Exported**: When using `HW_SIGN`, the private key never leaves the TROPIC01 chip.
