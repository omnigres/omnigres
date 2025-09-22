#! /usr/bin/env bash

if [ -n "$ALLOW_CORE_FILES" ]; then
  # Allow unlimited core files
  ulimit -c unlimited
fi

if [ -n "$TAILSCALE_AUTHKEY" ]; then
  echo "Initializing Tailscale"
  tailscaled --tun=userspace-networking &
  echo "Connecting to Tailscale"
  tailscale up --ssh --authkey=$TAILSCALE_AUTHKEY --hostname=$TAILSCALE_HOSTNAME
fi
source docker-entrypoint.sh

PGUSER="${PGUSER:-$POSTGRES_USER}"
echo "Now starting Omnigres database and application services. (${POSTGRES_INITDB_ARGS})"
echo "You can connect to this service the following ways:"
echo
echo "    psql -h localhost -p ${PGPORT:-5432} -U ${PGUSER:-postgres} omnigres"
echo "    http://localhost:8081/"
echo
echo "If you're using docker, verify the above ports with 'docker ps'"

_main $@

