#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: poll_rb1_usage.sh [-u URL] [-i SECONDS] [-o LOGFILE] [--stop-on-frames-lost]

Polls /logger/status and logs the full response as CSV.

Defaults:
  URL      http://<device-ip>/logger/status
  SECONDS  10
  LOGFILE  logs/<timestamp>_rb1_usage.log

Options:
  -s, --stop-on-frames-lost  Request logger stop and exit when frames_lost is true
      --stop-url URL         Override stop URL. Default is <device>/cancontrol.cgi?action=Stop
      --debug-url URL        Override debug URL. Default is <device>/logger/debug
  -h, --help                 Show this help
EOF
}

URL="${URL:-http://<device-ip>/logger/status}"
INTERVAL="${INTERVAL:-10}"
OUT="${OUT:-logs/$(date +"%Y-%m-%dT%H-%M-%S")_rb1_usage.log}"
STOP_ON_FRAMES_LOST=false
STOP_URL="${STOP_URL:-}"
DEBUG_URL="${DEBUG_URL:-}"

derive_base_url() {
  local status_url="$1"

  if [[ "$status_url" == *"/logger/status"* ]]; then
    printf '%s\n' "${status_url%%/logger/status*}"
  else
    printf '%s\n' "${status_url%/*}"
  fi
}

derive_stop_url() {
  local status_url="$1"
  printf '%s/cancontrol.cgi?action=Stop\n' "$(derive_base_url "$status_url")"
}

derive_version_url() {
  local status_url="$1"
  printf '%s/api/version\n' "$(derive_base_url "$status_url")"
}

derive_debug_url() {
  local status_url="$1"
  printf '%s/logger/debug\n' "$(derive_base_url "$status_url")"
}

request_version() {
  local version_url="$1"
  local ts
  local version_resp

  ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"

  if version_resp="$(curl -fsS --max-time 15 "$version_url")"; then
    printf '%s,api_version,%s\n' "$ts" "$version_resp" >> "$OUT"
    echo "Requested API version via $version_url" >&2
  else
    printf '%s,api_version_failed\n' "$ts" >> "$OUT"
    echo "API version request failed: $version_url" >&2
  fi
}

request_debug() {
  local debug_url="$1"
  local ts
  local debug_resp

  ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"

  if debug_resp="$(curl -fsS --max-time 15 "$debug_url")"; then
    printf '%s,logger_debug,%s\n' "$ts" "$debug_resp" >> "$OUT"
    echo "Requested logger debug via $debug_url" >&2
  else
    printf '%s,logger_debug_failed\n' "$ts" >> "$OUT"
    echo "Logger debug request failed: $debug_url" >&2
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -u|--url)
      URL="$2"
      shift 2
      ;;
    -i|--interval)
      INTERVAL="$2"
      shift 2
      ;;
    -o|--output)
      OUT="$2"
      shift 2
      ;;
    -s|--stop-on-frames-lost)
      STOP_ON_FRAMES_LOST=true
      shift
      ;;
    --stop-url)
      STOP_URL="$2"
      shift 2
      ;;
    --debug-url)
      DEBUG_URL="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown arg: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if ! command -v curl >/dev/null 2>&1; then
  echo "curl not found in PATH." >&2
  exit 1
fi

mkdir -p "$(dirname "$OUT")"

if [[ ! -f "$OUT" ]]; then
  echo "timestamp,response" > "$OUT"
fi

version_url="$(derive_version_url "$URL")"
debug_url="${DEBUG_URL:-$(derive_debug_url "$URL")}"

# Record the device/API version before the first status poll.
request_version "$version_url"

# Debug snapshot at the beginning of the monitored logging period.
request_debug "$debug_url"

while true; do
  ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"

  if resp="$(curl -fsS --max-time 15 "$URL")"; then
    cleaned_resp="$resp"

    if [[ "$cleaned_resp" == *"}%" ]]; then
      cleaned_resp="${cleaned_resp%\%}"
    fi

    printf '%s,%s\n' "$ts" "$cleaned_resp" >> "$OUT"

    if [[ "$STOP_ON_FRAMES_LOST" == true ]] \
      && [[ "$cleaned_resp" =~ \"frames_lost\"[[:space:]]*:[[:space:]]*true ]]; then

      stop_url="${STOP_URL:-$(derive_stop_url "$URL")}"

      if curl -fsS --max-time 15 "$stop_url" >/dev/null; then
        printf '%s,logger_stop_requested\n' "$ts" >> "$OUT"
        echo "frames_lost=true detected; requested logger stop via $stop_url" >&2
        exit_code=2
      else
        printf '%s,logger_stop_failed\n' "$ts" >> "$OUT"
        echo "frames_lost=true detected, but logger stop request failed: $stop_url" >&2
        exit_code=3
      fi

      # Debug snapshot at the end of the monitored logging period.
      request_debug "$debug_url"

      exit "$exit_code"
    fi
  else
    printf '%s,unknown\n' "$ts" >> "$OUT"
  fi

  sleep "$INTERVAL"
done