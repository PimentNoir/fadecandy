Fadecandy systemd Examples
==========================

Once installed and enabled, these example systemd .service files will automatically start the Fadecandy server and one of the Python example scripts on boot.  For Linux systems only.

Installation Procedure
----------------------

1. Ensure that the __fcserver__ executable resides in the directory specified in the .service file. (/opt/fadecandy/server/ in this example)
2. Copy the .service files into the /etc/systemd/system/ directory
    ~~~
    $ cp fadecandy.service example-leds.service /etc/systemd/system/
    ~~~
3. Enable the services
    ~~~
    $ systemctl enable fadecandy.service
    $ systemctl enable example-leds.service
    ~~~
4. Start the services
    ~~~
    $ systemctl start fadecandy.service
    $ systemctl start example-leds.service
    ~~~
