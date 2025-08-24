#!/system/bin/sh

if [ -z "$MODDIR" ]; then
  MODDIR=$(dirname "$0")
fi

LOG_TAG="MiPushZygisk"
WHITELIST_FILE="$MODDIR/package.list"

log -p i -t $LOG_TAG "Starting package scan..."

> "$WHITELIST_FILE"

for PKG in $(pm list packages -3 | cut -d ':' -f 2); do
  if pm dump "$PKG" | grep -Eq "com.xiaomi.push.service.XMPushService|com.xiaomi.mipush.sdk.PushMessageHandler|com.xiaomi.mipush.sdk.MessageHandleService"; then
    log -p i -t $LOG_TAG "Found MiPush SDK in: $PKG"
    echo "$PKG" >> "$WHITELIST_FILE"
  fi
done

log -p i -t $LOG_TAG "Package scan finished. Whitelist updated."

