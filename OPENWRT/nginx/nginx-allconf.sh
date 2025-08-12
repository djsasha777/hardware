git clone https://github.com/djsasha777/hardware.git
mv hardware/OPENWRT/nginx/nginx.conf /etc/nginx/nginx.conf
mv hardware/OPENWRT/startpage/info.html /etc/nginx/info.html
mv hardware/OPENWRT/home/home.html /etc/nginx/home.html
mv hardware/OPENWRT/nginx/nginx-allconf.sh /root/run.sh
rm -rf hardware
chmod 777 /root/run.sh
chmod 644 /etc/nginx/nginx.conf
chmod 664 /etc/nginx/info.html
chmod 664 /etc/nginx/home.html
if [[ ! -f "/etc/nginx/acme/acme.html" ]]; then
    cat > /etc/nginx/acme/acme.html << EOF
<!DOCTYPE html>
<html>
<head>
    <title>acme</title>
</head>
</html>
EOF
fi
service nginx restart
echo "files copied and nginx restarted."