#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: poll_rb1_usage.sh [-u URL] [-i SECONDS] [-o LOGFILE] [-t SECONDS] [--stop-on-frames-lost]

Polls /logger/status and logs the full response as CSV.

Defaults:
  URL      http://<device-ip>/logger/status
  SECONDS  10
  LOGFILE  logs/<timestamp>_rb1_usage.log

Options:
  -t, --max-duration SECONDS  Request logger stop, then exit, after SECONDS have
                             elapsed (0 = run until stopped, the default)
  -s, --stop-on-frames-lost  Request logger stop and exit when frames_lost is true
      --stop-url URL         Override stop URL. Default is <device>/cancontrol.cgi?action=Stop
      --version-url URL      Override version URL. Default is <device>/api/version
      --card-url URL         Override card-info URL. Default is <device>/api/card
      --debug-url URL        Override debug URL. Default is <device>/logger/debug
      --errors-url URL       Override errors URL. Default is <device>/logger/errors
  -h, --help                 Show this help

Environment overrides:
  STARTUP_RETRIES   Attempts to reach <device>/api/version before the first
                    poll, spaced RETRY_DELAY apart (default 60 -> ~5 min).
                    Covers the logger still booting when polling starts.
  SNAPSHOT_RETRIES  Attempts for each one-shot request (version/card/debug/
                    errors) once the API is reachable (default 3).
  RETRY_DELAY       Seconds between retries (default 5).
EOF
}

URL="${URL:-http://<device-ip>/logger/status}"
INTERVAL="${INTERVAL:-10}"
MAX_DURATION="${MAX_DURATION:-0}"
OUT="${OUT:-logs/$(date +"%Y-%m-%dT%H-%M-%S")_rb1_usage.log}"
STOP_ON_FRAMES_LOST=false
STOP_URL="${STOP_URL:-}"
VERSION_URL="${VERSION_URL:-}"
CARD_URL="${CARD_URL:-}"
DEBUG_URL="${DEBUG_URL:-}"
ERRORS_URL="${ERRORS_URL:-}"

STARTUP_RETRIES="${STARTUP_RETRIES:-60}"
SNAPSHOT_RETRIES="${SNAPSHOT_RETRIES:-3}"
RETRY_DELAY="${RETRY_DELAY:-5}"

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

derive_card_url() {
  local status_url="$1"
  printf '%s/api/card\n' "$(derive_base_url "$status_url")"
}

derive_debug_url() {
  local status_url="$1"
  printf '%s/logger/debug\n' "$(derive_base_url "$status_url")"
}

derive_errors_url() {
  local status_url="$1"
  printf '%s/logger/errors\n' "$(derive_base_url "$status_url")"
}

# GET a URL, retrying transient failures. Prints the response body on success.
# Args: URL [ATTEMPTS] [DELAY]
http_get_retry() {
  local url="$1"
  local attempts="${2:-$SNAPSHOT_RETRIES}"
  local delay="${3:-$RETRY_DELAY}"
  local i body

  for (( i = 1; i <= attempts; i++ )); do
    if body="$(curl -fsS --max-time 15 "$url")"; then
      printf '%s' "$body"
      return 0
    fi
    if (( i < attempts )); then
      echo "GET $url failed (attempt $i/$attempts); retry in ${delay}s..." >&2
      sleep "$delay" &
      wait "$!" || true
    fi
  done

  return 1
}

# Block until <device>/api/version answers, or STARTUP_RETRIES are exhausted.
# The logger's HTTP server may not be up yet when polling starts right after boot.
wait_for_api() {
  local version_url="$1"

  if http_get_retry "$version_url" "$STARTUP_RETRIES" >/dev/null; then
    echo "Device API reachable." >&2
    return 0
  fi

  echo "Device API unreachable after ${STARTUP_RETRIES} attempts; capturing what is available." >&2
  return 1
}

request_version() {
  local version_url="$1"
  local attempts="${2:-$SNAPSHOT_RETRIES}"
  local ts
  local version_resp

  if version_resp="$(http_get_retry "$version_url" "$attempts")"; then
    ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"
    version_resp="${version_resp%\%}"
    printf '%s,api_version,%s\n' "$ts" "$version_resp" >> "$OUT"
    echo "Requested API version via $version_url" >&2
  else
    ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"
    printf '%s,api_version_failed\n' "$ts" >> "$OUT"
    echo "API version request failed after ${attempts} attempts: $version_url" >&2
  fi
}

request_card() {
  local card_url="$1"
  local attempts="${2:-$SNAPSHOT_RETRIES}"
  local ts
  local card_resp

  if card_resp="$(http_get_retry "$card_url" "$attempts")"; then
    ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"
    card_resp="${card_resp%\%}"
    printf '%s,api_card,%s\n' "$ts" "$card_resp" >> "$OUT"
    echo "Requested API card info via $card_url" >&2
  else
    ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"
    printf '%s,api_card_failed\n' "$ts" >> "$OUT"
    echo "API card request failed after ${attempts} attempts: $card_url" >&2
  fi
}

request_debug() {
  local debug_url="$1"
  local attempts="${2:-$SNAPSHOT_RETRIES}"
  local ts
  local debug_resp

  if debug_resp="$(http_get_retry "$debug_url" "$attempts")"; then
    ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"
    printf '%s,logger_debug,%s\n' "$ts" "$debug_resp" >> "$OUT"
    echo "Requested logger debug via $debug_url" >&2
  else
    ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"
    printf '%s,logger_debug_failed\n' "$ts" >> "$OUT"
    echo "Logger debug request failed after ${attempts} attempts: $debug_url" >&2
  fi
}

request_errors() {
  local errors_url="$1"
  local attempts="${2:-$SNAPSHOT_RETRIES}"
  local ts
  local errors_resp

  if errors_resp="$(http_get_retry "$errors_url" "$attempts")"; then
    ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"
    printf '%s,logger_errors,%s\n' "$ts" "$errors_resp" >> "$OUT"
    echo "Requested logger errors via $errors_url" >&2
  else
    ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"
    printf '%s,logger_errors_failed\n' "$ts" >> "$OUT"
    echo "Logger errors request failed after ${attempts} attempts: $errors_url" >&2
  fi
}

# Ask the device to stop logging. Returns non-zero if the request failed.
request_stop() {
  local stop_url="$1"
  local reason="$2"
  local ts

  ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"

  if curl -fsS --max-time 15 "$stop_url" >/dev/null; then
    printf '%s,logger_stop_requested\n' "$ts" >> "$OUT"
    echo "$reason; requested logger stop via $stop_url" >&2
    return 0
  else
    printf '%s,logger_stop_failed\n' "$ts" >> "$OUT"
    echo "$reason, but logger stop request failed: $stop_url" >&2
    return 1
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
    -t|--max-duration)
      MAX_DURATION="$2"
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
    --version-url)
      VERSION_URL="$2"
      shift 2
      ;;
    --card-url)
      CARD_URL="$2"
      shift 2
      ;;
    --debug-url)
      DEBUG_URL="$2"
      shift 2
      ;;
    --errors-url)
      ERRORS_URL="$2"
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

if ! [[ "$MAX_DURATION" =~ ^[0-9]+$ ]]; then
  echo "Invalid --max-duration: '$MAX_DURATION' (expected a non-negative integer)." >&2
  exit 1
fi

for retry_var in STARTUP_RETRIES SNAPSHOT_RETRIES RETRY_DELAY; do
  if ! [[ "${!retry_var}" =~ ^[0-9]+$ ]]; then
    echo "Invalid ${retry_var}: '${!retry_var}' (expected a non-negative integer)." >&2
    exit 1
  fi
done
[[ "$SNAPSHOT_RETRIES" -lt 1 ]] && SNAPSHOT_RETRIES=1

mkdir -p "$(dirname "$OUT")"

if [[ ! -f "$OUT" ]]; then
  echo "timestamp,response" > "$OUT"
fi

version_url="${VERSION_URL:-$(derive_version_url "$URL")}"
card_url="${CARD_URL:-$(derive_card_url "$URL")}"
debug_url="${DEBUG_URL:-$(derive_debug_url "$URL")}"
errors_url="${ERRORS_URL:-$(derive_errors_url "$URL")}"

# When stopped externally (e.g. via SIGTERM from a supervising script), still
# capture a snapshot at the end of the monitored logging period.
on_term() {
  local ts
  ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"
  printf '%s,logger_stopped_by_signal\n' "$ts" >> "$OUT"
  # Single attempt: a stop signal is time-sensitive and the supervising script
  # only waits a few seconds for this process to exit.
  request_card "$card_url" 1
  request_debug "$debug_url" 1
  request_errors "$errors_url" 1
  exit 0
}
trap on_term TERM INT

start_epoch="$(date +%s)"
if [[ "$MAX_DURATION" -gt 0 ]]; then
  echo "Polling will stop automatically after ${MAX_DURATION}s." >&2
fi

# The logger's HTTP server may still be booting when polling starts; wait for it
# so the version/card provenance is captured instead of logged as failed.
wait_for_api "$version_url" || true

# Record the device/API version and card identity before the first status poll.
request_version "$version_url"
request_card "$card_url"

# Debug snapshot at the beginning of the monitored logging period.
request_debug "$debug_url"
request_errors "$errors_url"

while true; do
  ts="$(date +"%Y-%m-%dT%H:%M:%S%:z")"

  if [[ "$MAX_DURATION" -gt 0 ]] \
    && (( $(date +%s) - start_epoch >= MAX_DURATION )); then
    printf '%s,max_duration_reached\n' "$ts" >> "$OUT"
    echo "Max duration ${MAX_DURATION}s reached; stopping." >&2

    stop_url="${STOP_URL:-$(derive_stop_url "$URL")}"

    if request_stop "$stop_url" "max duration ${MAX_DURATION}s reached"; then
      exit_code=0
    else
      exit_code=3
    fi

    # Snapshot at the end of the monitored logging period.
    request_card "$card_url"
    request_debug "$debug_url"
    request_errors "$errors_url"

    exit "$exit_code"
  fi

  if resp="$(curl -fsS --max-time 15 "$URL")"; then
    cleaned_resp="$resp"

    if [[ "$cleaned_resp" == *"}%" ]]; then
      cleaned_resp="${cleaned_resp%\%}"
    fi

    printf '%s,%s\n' "$ts" "$cleaned_resp" >> "$OUT"

    if [[ "$STOP_ON_FRAMES_LOST" == true ]] \
      && [[ "$cleaned_resp" =~ \"frames_lost\"[[:space:]]*:[[:space:]]*true ]]; then

      stop_url="${STOP_URL:-$(derive_stop_url "$URL")}"

      if request_stop "$stop_url" "frames_lost=true detected"; then
        exit_code=2
      else
        exit_code=3
      fi

      # Debug snapshot at the end of the monitored logging period.
      request_debug "$debug_url"
      request_errors "$errors_url"

      exit "$exit_code"
    fi
  else
    printf '%s,unknown\n' "$ts" >> "$OUT"
  fi

  # Background the sleep and wait so a TERM/INT signal breaks out immediately
  # instead of being deferred until the sleep returns.
  sleep "$INTERVAL" &
  wait "$!" || true
done
