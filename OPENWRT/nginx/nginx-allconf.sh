git clone https://github.com/djsasha777/hardware.git
cp -rf hardware/OPENWRT/nginx/* /etc/nginx/
mv /etc/nginx/nginx-allconf.sh /root/run.sh
rm -rf hardware
chmod +x /root/run.sh
chmod 664 -R /etc/nginx
service nginx restart
echo "files copied and nginx restarted."