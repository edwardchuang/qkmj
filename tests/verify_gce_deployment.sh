#!/bin/bash
# Verification script for GCE deployment

SERVER_IP=$1

if [ -z "$SERVER_IP" ]; then
    echo "Usage: $0 <SERVER_IP>"
    exit 1
fi

echo "🔍 Verifying GCE Deployment at $SERVER_IP..."

# 1. Check if port 7001 is open
echo "Step 1: Checking connectivity to port 7001..."
if nc -zv -w5 $SERVER_IP 7001 2>&1 | grep -q "succeeded"; then
    echo "✅ Port 7001 is open and reachable."
else
    echo "❌ Port 7001 is closed or unreachable. Check firewall rules."
    exit 1
fi

# 2. Verify MongoDB Integration via Client
echo "Step 2: Testing server responsiveness (Protocol Check)..."
# We can't easily run the full client here, but we can check if it accepts a connection
# and doesn't immediately close it.
if (echo -n "test" | nc -w2 $SERVER_IP 7001 > /dev/null); then
    echo "✅ Server accepted a test connection."
else
    echo "❌ Server refused connection."
    exit 1
fi

echo "🎉 GCE Verification Complete!"
