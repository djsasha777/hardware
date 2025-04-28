#!/bin/sh

REPO_URL="https://github.com/djsasha777/hardware.git"
LOCAL_FILE="/etc/nginx/nginx.conf"

mkdir autoconf && cd autoconf
git clone $REPO_URL
cd hardware/OPENWRT/nginx
cp nginx.conf $LOCAL_FILE.tmp
cd && rm -rf autoconf

if [[ -f "$LOCAL_FILE.tmp" ]]; then
    mv "$LOCAL_FILE.tmp" "$LOCAL_FILE"
    chmod 644 "$LOCAL_FILE"
    service nginx restart
    echo "Файл успешно обновлён."
else
    echo "Ошибка загрузки файла."
fi
