git clone https://github.com/djsasha777/hardware.git
mv hardware/OPENWRT/nginx/* /etc/nginx/
mv /etc/nginx/nginx-allconf.sh /root/run.sh
rm -rf hardware
chmod +x /root/run.sh
chmod 644 /etc/nginx/nginx.conf
chmod 664 /etc/nginx/info.html
chmod 664 /etc/nginx/home.html
service nginx restart
echo "files copied and nginx restarted."