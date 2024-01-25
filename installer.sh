#!/bin/bash
git clone https://github.com/Avdan-OS/Imager/
echo "Fetched Update"
cd Imager
read -p "Check for node js install and update (y/n)?" confirm
case "$choice" in
  y|Y ) echo "checking" && sudo apt update && sudo apt install npm;;
  n|N ) echo "skipping";;
  * ) echo "EXITING!";;
esac
echo "Installing Imager as local user"
#cd ~/.local/share/applications
#touch AOSImager.desktop
echo "[Desktop Entry]" #> AOSImager.desktop
echo "Name=AvdanOS Imager" #>> AOSImager.desktop
echo "Exec=cd ~/Imager && npm run start" #>> AOSImager.desktop
echo "Terminal=false" #>> AOSImager.desktop
echo "Type=Application" #>> AOSImager.desktop
echo "Icon=~/Imager/src/components/images/titlelogo.png" #>> AOSImager.desktop
echo "Category=Accesories" #>> AOSImager.desktop
# Terminal is disabled for release and change Type if needed
echo "Unfortunatly it doesnt work so you should manually add the .desktop but here is an config that would work"
sleep 12
#cd ~/Imager
npm install
npm run start
