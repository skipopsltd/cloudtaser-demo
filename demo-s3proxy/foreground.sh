echo "Setting up (~1 min)..." && while [ ! -f /tmp/.cloudtaser-setup-done ]; do sleep 2; done && source /tmp/.demo-env && echo "Ready!"
