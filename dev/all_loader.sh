enabled_device_drivers=("servo" "dht11" "sonar" "infrared_sensor" "buzzer" "photo_register" "touch" "led")
for dd in "${enabled_device_drivers[@]}"; do
	cd $dd
	sh loader.sh
	make clean
	cd ..
done
