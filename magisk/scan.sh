#!/system/bin/sh

if [ -z "$MODDIR" ]; then
  MODDIR=$(dirname "$0")
fi

LOG_TAG="MiPushZygisk"
WHITELIST_FILE="$MODDIR/package.list"
BLACKLIST_FILE="$MODDIR/blacklist.conf"

log -p i -t $LOG_TAG "Starting package scan..."

> "$WHITELIST_FILE"

pm list packages -3 | cut -d ':' -f 2 | while read -r PKG; do
  if [ -s "$BLACKLIST_FILE" ] && grep -qw "$PKG" "$BLACKLIST_FILE"; then
    log -p i -t $LOG_TAG "Package [$PKG] is in the blacklist, skipping."
    continue
  fi

  APK_PATH=$(pm path "$PKG" | head -n 1 | cut -d ':' -f 2)

  if [ -z "$APK_PATH" ]; then
    continue
  fi

  if aapt d badging "$APK_PATH" 2>/dev/null | grep -qE 'service:[[:space:]]name=.*(XMPushService|PushMessageHandler|MessageHandleService)'; then
    log -p i -t $LOG_TAG "Found MiPush SDK in: $PKG (Passed blacklist check)"
    echo "$PKG" >> "$WHITELIST_FILE"
  fi
done

log -p i -t $LOG_TAG "Package scan finished. Whitelist updated."

