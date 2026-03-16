#!/bin/bash
echo "Setting up CloudTaser demo environment..."
echo "This takes about 2 minutes."
echo ""
while [ ! -f /tmp/.cloudtaser-setup-done ]; do sleep 2; done
echo "Ready! Click Next to begin."
