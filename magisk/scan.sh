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
  [ -z "$APK_PATH" ] && continue

  if unzip -l "$APK_PATH" 2>/dev/null | grep -q "com/xiaomi/mipush/"; then
    if pm dump "$PKG" 2>/dev/null | grep -qE '(XMPushService|PushMessageHandler|MessageHandleService)'; then
      log -p i -t $LOG_TAG "Found MiPush SDK in: $PKG"
      echo "$PKG" >> "$WHITELIST_FILE"
    else
      log -p i -t $LOG_TAG "[$PKG] contains MiPush classes but no service registered (skipped)."
    fi
  fi
done

log -p i -t $LOG_TAG "Package scan finished. Whitelist updated."

