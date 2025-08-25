#!/system/bin/sh
# scan.sh

# 模块路径兼容处理
if [ -z "$MODDIR" ]; then
  MODDIR=$(dirname "$0")
fi

LOG_TAG="MiPushZygisk"
WHITELIST_FILE="$MODDIR/package.list"
BLACKLIST_FILE="$MODDIR/blacklist.conf"

log -p i -t $LOG_TAG "Starting package scan..."

# 清空旧的列表
> "$WHITELIST_FILE"

# 统计变量
total=0
skipped=0
matched=0

for PKG in $(pm list packages -3 | cut -d ':' -f 2); do
  total=$((total+1))

  # 黑名单检查
  if [ -s "$BLACKLIST_FILE" ] && grep -qw "$PKG" "$BLACKLIST_FILE"; then
    log -p i -t $LOG_TAG "Package [$PKG] is in the blacklist, skipping."
    skipped=$((skipped+1))
    continue
  fi

  # 使用 pm dump 检查是否包含 MiPush 服务
  if pm dump "$PKG" 2>/dev/null | grep -qE '(com.xiaomi.push.service.XMPushService|com.xiaomi.mipush.sdk.PushMessageHandler|com.xiaomi.mipush.sdk.MessageHandleService)'; then
    log -p i -t $LOG_TAG "Found MiPush SDK in: $PKG"
    echo "$PKG" >> "$WHITELIST_FILE"
    matched=$((matched+1))
  fi
done

log -p i -t $LOG_TAG "Package scan finished."
log -p i -t $LOG_TAG "Total: $total, Skipped: $skipped, Matched: $matched"
log -p i -t $LOG_TAG "Whitelist updated at $WHITELIST_FILE"

# 可选：强制 Zygote 重启以加载新的白名单
# stop; start;
