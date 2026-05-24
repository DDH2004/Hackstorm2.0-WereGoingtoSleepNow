(Root)
./setup.sh
source export.sh
cd AIoTHackStorm/apps/hackstorm_clock
python ../../tos.py build
python ../../tos.py flash -p /dev/cu.usbmodem5AAE1670581

Start Bridge
pip install pyserial        # one-time
python serial_bridge.py --port /dev/cu.usbmodem5AAE1670581

