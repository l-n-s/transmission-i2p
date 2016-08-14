transmission-i2p
================

Anonymous torrent client powered by [Invisible Internet](http://i2pd.website).

How to build on Debian Jessie
-----------------------------

Clone the repo:

    git clone https://github.com/l-n-s/transmission-i2p.git

Install required packages:

    sudo apt-get install build-essential automake autoconf libtool pkg-config intltool libcurl4-openssl-dev libglib2.0-dev libevent-dev libminiupnpc-dev libappindicator-dev automake1.11 

Then configure and make:

    ./configure --with-gtk && make

Result binary is **gtk/transmission-gtk**.

Using with i2pd
---------------

Install [i2pd](https://github.com/PurpleI2P/i2pd).

Configure i2pd to enable BOB interface. Edit your i2pd.conf:

    [bob]
    ## Uncomment and set to 'true' to enable BOB command channel
    enabled = true 
    ## Address and port service will listen on
    address = 127.0.0.1
    port = 2827

OR run i2pd binary with option **--bob.enabled 1**

Now run **gtk/transmission-gtk** and configure settings:

- Uncheck all options in 'Network' tab, except PEX
- Adjust settings in 'I2P/BOB' tab

If everything is set up correctly, you should be able to share and download
files in Invisible Internet. 

Beware that magnet links are not working yet, use .torrent files.


Original description by COMiX
-----------------------------

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

All work was made by COMiX. I've just cleaned some junk files and published 
this code at GitHub.

