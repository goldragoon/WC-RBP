enabled_device_drivers=("servo" "touch" "led" "sonar")
for dd in "${enabled_device_drivers[@]}"; do
	cd $dd
	sh loader.sh
	make clean
	cd ..
done
