#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: poll_rb1_usage.sh [-u URL] [-i SECONDS] [-o LOGFILE]

Polls /logger/status and logs the full response as CSV.

Defaults:
  URL      http://<device-ip>/logger/status
  SECONDS  10
  LOGFILE  rb1_usage.log
EOF
}

URL="${URL:-http://<device-ip>/logger/status}"
INTERVAL="${INTERVAL:-10}"
OUT="${OUT:-rb1_usage.log}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    -u|--url) URL="$2"; shift 2 ;;
    -i|--interval) INTERVAL="$2"; shift 2 ;;
    -o|--output) OUT="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown arg: $1" >&2; usage; exit 1 ;;
  esac
done

if ! command -v curl >/dev/null 2>&1; then
  echo "curl not found in PATH." >&2
  exit 1
fi

if [[ ! -f "$OUT" ]]; then
  echo "timestamp,response" > "$OUT"
fi

while true; do
  ts="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
  if resp="$(curl -fsS --max-time 15 "$URL")"; then
    cleaned_resp="$resp"
    if [[ "$cleaned_resp" == *"}%" ]]; then
      cleaned_resp="${cleaned_resp%\%}"
    fi
    printf '%s,%s\n' "$ts" "$cleaned_resp" >> "$OUT"
  else
    printf '%s,unknown\n' "$ts" >> "$OUT"
  fi
  sleep "$INTERVAL"
done
