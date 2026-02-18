# NexusClaw User Manual

<p align="center">
  <img src="../assets/nexusclaw-logo.svg" alt="NexusClaw" width="150"/>
</p>

<p align="center">
  <strong>Hardware Security Key for AI Agents</strong><br/>
  <em>Version 2.0.0 | AVP Protocol v0.1.0</em>
</p>

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Package Contents](#2-package-contents)
3. [Getting Started](#3-getting-started)
4. [Basic Operations](#4-basic-operations)
5. [Using with AI Frameworks](#5-using-with-ai-frameworks)
6. [Advanced Features](#6-advanced-features)
7. [Security Best Practices](#7-security-best-practices)
8. [Troubleshooting](#8-troubleshooting)
9. [Technical Specifications](#9-technical-specifications)
10. [Support](#10-support)

---

## 1. Introduction

### What is NexusClaw?

NexusClaw is a hardware security key designed specifically for AI agents and developers. It securely stores API keys, tokens, and credentials in tamper-resistant silicon, ensuring your sensitive data never touches disk or memory in plaintext.

### Key Benefits

- **Hardware Security** — Secrets stored in TROPIC01 secure element
- **Never Exported** — Private keys never leave the chip
- **PIN Protected** — 6-digit PIN with lockout protection
- **Open Source** — Fully auditable firmware
- **Universal** — Works with any system via USB

### How It Works

```
┌──────────────┐     USB      ┌──────────────┐     SPI      ┌──────────────┐
│  Your Agent  │◄───────────►│   NexusClaw  │◄───────────►│   TROPIC01   │
│  (Python,    │   AVP JSON   │   STM32U5    │  Encrypted   │   Secure     │
│   Node.js)   │              │   Firmware   │   Session    │   Element    │
└──────────────┘              └──────────────┘              └──────────────┘
```

---

## 2. Package Contents

Your NexusClaw package includes:

- 1x NexusClaw USB Security Key
- 1x Quick Start Card
- 1x Keychain attachment

**Note:** Keep your default PIN card in a safe place. You'll need it for initial setup.

---

## 3. Getting Started

### 3.1 Connecting NexusClaw

1. **Insert** NexusClaw into any USB port
2. **Wait** for the LED to turn solid (ready state)
3. **Verify** the device appears on your system

**LED States:**
| LED | Meaning |
|-----|---------|
| Off | No power |
| Solid | Ready |
| Blinking | Error / Not connected |
| Flashing | Activity |

### 3.2 Finding Your Device

**Linux:**
```bash
ls /dev/ttyACM*
# Usually: /dev/ttyACM0
```

**macOS:**
```bash
ls /dev/tty.usbmodem*
# Example: /dev/tty.usbmodem14201
```

**Windows:**
- Open Device Manager
- Look under "Ports (COM & LPT)"
- Note the COM port number (e.g., COM3)

### 3.3 Installing the AVP Client

**Python:**
```bash
pip install avp-py pyserial
```

**Node.js:**
```bash
npm install @avp-protocol/avp-ts serialport
```

**Rust:**
```bash
cargo add avp-rs
```

### 3.4 First Connection Test

```python
from avp import Vault

# Connect to NexusClaw
vault = Vault("avp+usb:///dev/ttyACM0")  # Adjust port for your system

# Query device info
info = vault.discover()
print(f"Connected to: {info['model']}")
print(f"Serial: {info['serial']}")
print(f"AVP Version: {info['version']}")
```

Expected output:
```
Connected to: NexusClaw
Serial: NC00000001
AVP Version: 0.1.0
```

---

## 4. Basic Operations

### 4.1 Authentication

Before storing or retrieving secrets, you must authenticate with your PIN.

```python
from avp import Vault

vault = Vault("avp+usb:///dev/ttyACM0")

# Authenticate with default PIN (change this!)
vault.authenticate(pin="123456")

print("Authenticated successfully!")
print(f"Session expires in: {vault.session_expires_in} seconds")
```

**Default PIN:** `123456`

⚠️ **Change your PIN immediately after first use!**

### 4.2 Storing Secrets

```python
# Store an API key
vault.store("anthropic_api_key", "sk-ant-api03-...")

# Store multiple secrets
vault.store("openai_api_key", "sk-...")
vault.store("github_token", "ghp_...")
vault.store("aws_access_key", "AKIA...")

print("Secrets stored securely!")
```

### 4.3 Retrieving Secrets

```python
# Retrieve a single secret
api_key = vault.retrieve("anthropic_api_key")
print(f"Retrieved key: {api_key[:20]}...")

# Use it immediately, then clear from memory
import anthropic
client = anthropic.Anthropic(api_key=api_key)
api_key = None  # Clear sensitive data
```

### 4.4 Listing Secrets

```python
# List all stored secrets
secrets = vault.list()
print("Stored secrets:")
for name in secrets:
    print(f"  - {name}")
```

### 4.5 Deleting Secrets

```python
# Delete a secret
vault.delete("old_api_key")
print("Secret deleted")
```

### 4.6 Rotating Secrets

```python
# Atomically update a secret
vault.rotate("anthropic_api_key", "sk-ant-new-key-...")
print("Secret rotated successfully")
```

---

## 5. Using with AI Frameworks

### 5.1 LangChain

```python
from langchain_anthropic import ChatAnthropic
from langchain_openai import ChatOpenAI
from avp import Vault

# Initialize vault
vault = Vault("avp+usb:///dev/ttyACM0")
vault.authenticate(pin="123456")

# Use with Anthropic
claude = ChatAnthropic(
    model="claude-sonnet-4-20250514",
    api_key=vault.retrieve("anthropic_api_key")
)

# Use with OpenAI
gpt = ChatOpenAI(
    model="gpt-4",
    api_key=vault.retrieve("openai_api_key")
)

# Chat with Claude
response = claude.invoke("Hello! How are you?")
print(response.content)
```

### 5.2 CrewAI

```python
from crewai import Agent, Task, Crew
from avp import Vault

vault = Vault("avp+usb:///dev/ttyACM0")
vault.authenticate(pin="123456")

# Create agent with hardware-secured API key
researcher = Agent(
    role="Senior Researcher",
    goal="Research and analyze topics",
    backstory="Expert researcher with years of experience",
    llm_config={
        "model": "gpt-4",
        "api_key": vault.retrieve("openai_api_key")
    }
)

# Create and run crew
crew = Crew(
    agents=[researcher],
    tasks=[Task(description="Research AI security", agent=researcher)]
)
result = crew.kickoff()
```

### 5.3 Direct API Usage

```python
import anthropic
from avp import Vault

vault = Vault("avp+usb:///dev/ttyACM0")
vault.authenticate(pin="123456")

# Get API key from hardware
api_key = vault.retrieve("anthropic_api_key")

# Use with Anthropic client
client = anthropic.Anthropic(api_key=api_key)
message = client.messages.create(
    model="claude-sonnet-4-20250514",
    max_tokens=1024,
    messages=[{"role": "user", "content": "Hello, Claude!"}]
)

# Clear sensitive data
api_key = None

print(message.content[0].text)
```

### 5.4 Environment Variable Replacement

Instead of `.env` files, use NexusClaw:

**Before (insecure):**
```python
import os
api_key = os.environ["ANTHROPIC_API_KEY"]  # Stored in plaintext!
```

**After (secure):**
```python
from avp import Vault
vault = Vault("avp+usb:///dev/ttyACM0")
vault.authenticate(pin="123456")
api_key = vault.retrieve("anthropic_api_key")  # From hardware!
```

---

## 6. Advanced Features

### 6.1 Hardware Signing

Sign data without exposing private keys:

```python
# Sign data with hardware key
data = b"Message to sign"
signature = vault.hw_sign("signing_key", data)

print(f"Signature: {signature.hex()}")

# The private key NEVER leaves the TROPIC01 chip!
```

### 6.2 Device Attestation

Verify your NexusClaw is genuine:

```python
# Verify device authenticity
attestation = vault.hw_challenge()

if attestation["verified"]:
    print(f"✓ Verified {attestation['model']}")
    print(f"  Serial: {attestation['serial']}")
else:
    print("✗ WARNING: Device verification failed!")
    # Do not trust this device!
```

### 6.3 Session Management

```python
# Check session status
if vault.session_valid():
    print("Session active")
else:
    print("Session expired, re-authenticating...")
    vault.authenticate(pin="123456")

# Get session info
print(f"Session ID: {vault.session_id}")
print(f"Expires in: {vault.session_expires_in}s")

# Manually end session
vault.logout()
```

### 6.4 Multiple Workspaces

Organize secrets into workspaces:

```python
# Authenticate to different workspaces
vault.authenticate(pin="123456", workspace="production")
prod_key = vault.retrieve("api_key")

vault.authenticate(pin="123456", workspace="development")
dev_key = vault.retrieve("api_key")
```

### 6.5 Custom Session TTL

```python
# Short session for sensitive operations
vault.authenticate(pin="123456", ttl=60)  # 1 minute

# Long session for batch processing
vault.authenticate(pin="123456", ttl=3600)  # 1 hour
```

---

## 7. Security Best Practices

### 7.1 PIN Security

| Do | Don't |
|----|-------|
| Change default PIN immediately | Use "123456" or "000000" |
| Use 6 random digits | Use birthdays or patterns |
| Keep PIN private | Share PIN with others |
| Enter PIN on trusted systems | Enter PIN on public computers |

**Changing Your PIN:**
```python
vault.change_pin(old_pin="123456", new_pin="847291")
```

### 7.2 Secret Hygiene

```python
# GOOD: Clear secrets after use
api_key = vault.retrieve("key")
client = APIClient(api_key)
api_key = None  # Clear immediately

# BAD: Leaving secrets in memory
api_key = vault.retrieve("key")
# ... api_key persists in memory
```

### 7.3 Session Security

- **Short TTL** for sensitive operations
- **Logout** when done
- **Re-authenticate** for critical actions

```python
# Secure pattern
vault.authenticate(pin=pin, ttl=60)
try:
    # Do sensitive work
    secret = vault.retrieve("critical_key")
    use_secret(secret)
    secret = None
finally:
    vault.logout()
```

### 7.4 Physical Security

- Don't leave NexusClaw unattended
- Remove from USB when not in use
- Store in secure location
- Report loss immediately

### 7.5 Attestation

Always verify device before first use:

```python
# First thing after connecting
att = vault.hw_challenge()
if not att["verified"]:
    raise SecurityError("Untrusted device!")
```

---

## 8. Troubleshooting

### 8.1 Device Not Found

**Symptom:** `Device not found` or `Permission denied`

**Linux Fix:**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Or create udev rule
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", MODE="0666"' | \
    sudo tee /etc/udev/rules.d/99-nexusclaw.rules
sudo udevadm control --reload-rules

# Logout and login again
```

**macOS Fix:**
```bash
# Usually works automatically
# Try different USB port if not detected
```

**Windows Fix:**
- Install driver from Device Manager if needed
- Try different USB port

### 8.2 Authentication Fails

**Symptom:** `PIN_INVALID` error

**Solution:**
1. Verify you're using correct PIN
2. Check Caps Lock is off
3. Count remaining attempts

```python
try:
    vault.authenticate(pin="123456")
except AVPError as e:
    if e.code == "PIN_INVALID":
        print(f"Wrong PIN. {e.attempts_remaining} attempts left")
```

### 8.3 PIN Locked

**Symptom:** `PIN_LOCKED` error after 5 failed attempts

**Solution:**
Device must be factory reset. Contact support.

⚠️ **Factory reset erases all stored secrets!**

### 8.4 Session Expired

**Symptom:** `SESSION_EXPIRED` error

**Solution:**
```python
# Re-authenticate
vault.authenticate(pin="123456")
# Continue operations
```

### 8.5 Timeout Errors

**Symptom:** `TimeoutError` during operations

**Solution:**
```python
# Increase timeout
vault = Vault("avp+usb:///dev/ttyACM0", timeout=10)

# Or retry
import time
for attempt in range(3):
    try:
        value = vault.retrieve("key")
        break
    except TimeoutError:
        time.sleep(0.5)
```

### 8.6 LED Blinking Continuously

**Meaning:** USB not properly connected

**Solution:**
1. Unplug and replug NexusClaw
2. Try different USB port
3. Try different computer
4. Check USB cable (if using extension)

---

## 9. Technical Specifications

### Hardware

| Component | Specification |
|-----------|---------------|
| MCU | STM32U535 (Cortex-M33, TrustZone) |
| Secure Element | TROPIC01 |
| Interface | USB 2.0 Full Speed |
| Connector | USB Type-A |
| Dimensions | 45 × 18 × 8 mm |
| Weight | 8g |

### Electrical

| Parameter | Value |
|-----------|-------|
| Supply Voltage | 5V (USB) |
| Current (idle) | < 10 mA |
| Current (active) | < 50 mA |

### Environmental

| Parameter | Value |
|-----------|-------|
| Operating Temperature | -20°C to +70°C |
| Storage Temperature | -40°C to +85°C |
| Humidity | 0-95% non-condensing |

### Security

| Feature | Specification |
|---------|---------------|
| Secure Element | TROPIC01 (CC EAL5+ pending) |
| Storage Slots | 128 (32 for secrets) |
| Max Secret Size | 256 bytes |
| Algorithms | ECC P-256, Ed25519, AES-256-GCM, SHA-3 |
| Random Source | TRNG + PUF |
| Tamper Protection | Active mesh, sensors |
| PIN Protection | 6 digits, 5 attempts before lockout |

### Protocol

| Parameter | Value |
|-----------|-------|
| Protocol | AVP v0.1.0 |
| Transport | USB CDC (Serial) |
| Baud Rate | 115200 |
| Format | JSON |
| Session TTL | 5-3600 seconds (default: 300) |

---

## 10. Support

### Documentation

- **Protocol Guide:** [docs/PROTOCOL.md](PROTOCOL.md)
- **GitHub:** https://github.com/avp-protocol/nexusclaw
- **AVP Specification:** https://github.com/avp-protocol/spec

### Getting Help

1. Check [Troubleshooting](#8-troubleshooting) section
2. Search [GitHub Issues](https://github.com/avp-protocol/nexusclaw/issues)
3. Open new issue with:
   - NexusClaw serial number
   - Firmware version (`VER` command)
   - Operating system
   - Error message / LED state
   - Steps to reproduce

### Firmware Updates

Check for updates:
```bash
# Current version
echo "VER" > /dev/ttyACM0
cat /dev/ttyACM0
```

Update procedure:
1. Download latest firmware from GitHub releases
2. Hold button while connecting USB (DFU mode)
3. Flash with dfu-util:
```bash
dfu-util -a 0 -s 0x08000000:leave -D nexusclaw-v2.x.x.bin
```

### Contact

- **Email:** support@avp-protocol.org
- **GitHub:** https://github.com/avp-protocol
- **Website:** https://avp-protocol.org

---

<p align="center">
  <strong>NexusClaw</strong> — Hardware Security for the AI Age<br/>
  <sub>Part of the <a href="https://github.com/avp-protocol">Agent Vault Protocol</a> ecosystem</sub>
</p>

---

**Document Version:** 1.0
**Last Updated:** February 2025
**Applicable Firmware:** 2.0.0-avp and later
