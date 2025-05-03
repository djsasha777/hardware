git clone https://github.com/djsasha777/hardware.git
mv hardware/OPENWRT/nginx/nginx.conf /etc/nginx/nginx.conf
mv hardware/OPENWRT/startpage/info.html /etc/nginx/info.html
rm -rf hardware
chmod 644 /etc/nginx/nginx.conf
chmod 664 /etc/nginx/info.html
service nginx restart
echo "files copied and nginx restarted."