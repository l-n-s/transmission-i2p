transmission-i2p
================

This version of Transmission is based on the excellent work of dinamic (thank a lot and respect to you, Man)


First, enable BOB bridge in your router config client (Yes, we use BOB, not SAM bridge, but future version will support both)


Extract Transmission, open a terminal in the extracted path and type: ./configure --with-gtk && make


If all is fine, your new builded Transmission for i2p is now located in the gtk folder. You can use it "on the fly" or type a: make install as root to install on your system (remove previous installed version if any)


Run Transmission-gtk, open 'settings' and go to the tab 'BOB/I2P', adjust your settings, check the box 'enable BOB bridge' and apply your changes
Open the tab 'network' and uncheck all 'options' (DHT,uTP....) except PEX


This is not a mixed version, if you enable I2P, you can ONLY download torrent in I2P. But if you disable I2P, Transmission will download clearnet torrent as usual.

Magnet link are broken, must be improved
Web console is not patched now, I am working on this


Sometime, transferts can take a while to start (like 3~5 min) Be patient, don't stress your system, we have the time with us


Disclaimer
----------

[Project website in I2P](http://bioq5jbcnfopqwvk7qssaxcl7avzeta6mu72jmxjeowflpcrhf6q.b32.i2p/transmission)

All work was made by COMiX. I've just cleaned some junk files and published this code at GitHub.

