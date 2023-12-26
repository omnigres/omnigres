#! /usr/bin/env bash

if [ -n "$TAILSCALE_AUTHKEY" ]; then
  echo "Initializing Tailscale"
  tailscaled --tun=userspace-networking &
  echo "Connecting to Tailscale"
  tailscale up --ssh --authkey=$TAILSCALE_AUTHKEY --hostname=$TAILSCALE_HOSTNAME
fi
source docker-entrypoint.sh
_main $@