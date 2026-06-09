# Flask Backend for Unreal SentryTower Game

This directory contains the Flask backend server that the Unreal SentryTower game communicates with for checkout operations.

## Overview

The Unreal game's `BuyUpgrade` function (in `Source/SentryTower/SentryTowerGameInstance.cpp`) makes HTTP POST requests to the `/checkout` endpoint to process upgrade purchases.

## Bug Fix (UNREAL-5)

**Issue:** The `/checkout` endpoint was returning HTTP 500 errors due to an `UnboundLocalError` in the inventory validation code.

**Root Cause:** The `quantities` variable was referenced at line 225 before being assigned at line 228, causing Python to raise an `UnboundLocalError` when `validate_inventory` was enabled.

**Fix:** Moved the `quantities` assignment before the length check to ensure the variable is defined before use.

## Endpoints

### POST /checkout
Processes checkout requests from the Unreal game.

**Request Body:**
```json
{
  "cart": {
    "items": [...],
    "quantities": {"3": 3},
    "total": 465
  },
  "form": {
    "email": "user@example.com",
    ...
  },
  "validate_inventory": "true"
}
```

**Response:**
- 200 OK with `{"status": "success"}` if checkout is successful
- 200 OK with `{"status": "failed"}` if all items are out of stock
- 200 OK with `{"status": "partial", "out_of_stock": [...]}` if some items fulfilled
- 500 Error if validation fails (before fix)

## Running Locally

```bash
# Install dependencies
pip install -r requirements.txt

# Set environment variables (see _.env.template)
export FLASK_DSN="your-sentry-dsn"
export FLASK_ENVIRONMENT="local"

# Run the server
./run_local.sh
```

## Testing

Run the verification test to ensure the fix is working:

```bash
cd src/test
python3 test_checkout_fix.py -v
```

## Source

This Flask backend is from the [sentry-demos/empower](https://github.com/sentry-demos/empower) repository and has been included here with the bug fix for UNREAL-5.
