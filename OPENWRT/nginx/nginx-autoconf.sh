#!/bin/bash

REPO_URL="https://github.com/djsasha777/hardware.git"
REMOTE_FILE_PATH="OPENWRT/nginx/nginx.conf"
LOCAL_FILE="/etc/nginx/nginx.conf"

CURRENT_SHA=""

while true; do

    NEW_SHA=$(curl -s "$REPO_URL/commits/main/$REMOTE_FILE_PATH")
    
    if [[ ! -z "$NEW_SHA" && "$NEW_SHA" != "$CURRENT_SHA" ]]; then
    
        echo "Обнаружены изменения в файле!"
        
        curl -sL "$REPO_URL/blob/main/$REMOTE_FILE_PATH" > "$LOCAL_FILE.tmp"
        
        if [[ -f "$LOCAL_FILE.tmp" ]]; then
            mv "$LOCAL_FILE.tmp" "$LOCAL_FILE"
            chmod 644 "$LOCAL_FILE"
            
            CURRENT_SHA="$NEW_SHA"
            service nginx restart
            echo "Файл успешно обновлён."
        else
            echo "Ошибка загрузки файла."
        fi
    fi
    
    sleep 10
done