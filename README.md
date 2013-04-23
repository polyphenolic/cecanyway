cec anyway
==========

A tiny tool to help control xbmc by CEC. 

What?
=====

CEC (Consumer Electronics Control) is an HDMI feature which is implemented in many A/V products (TVs, receivers, consoles)
these days. Manufacturers of course use their own brands for it (Sony: BRAVIA Sync, Samsung: Anynet+, Onkyo: RIHD). Among
other things CEC allows controlling several devices with a single remote. 

There is an open source library called libcec which provides an interface to cec hardware. This is not only handy to limit
the amount of remotes with redundant buttons, but especially useful for devices like the raspberry pi, which do not have
ir hardware built in. Meanwhile it has been integrated well into XBMC.

Why?
====

Picture the following scenario:


 +------+                   +------+                    +------+
 |`.    |`.                 |`.    |`.                  |`.    |`. 
 |  `+--+---+               |  `+--+---+                |  `+--+---+  
 |   |  | o=|=================o |  | o=|==================o |  |   |  
 +---+--+   |               +---+--+   |                +---+--+   |
  `. |   `. |                `. |   `. |                 `. |   `. |
    `+------+                   +------+                    +------+

 Raspberry Pi with             CEC-enabled               TV, panel or beamer 
 libcec installed              A/V receiver              without CEC support  

Now, in theory the XBMC instance on Raspberry Pi could be controlled by the A/V receiver's remote. If you enable CEC input
in XBMC and inspect the debug logs you can see the keypress events if you press butons on the remote. However it does not
work, the buttons are ignored. This is because the spec requires a root display device (usually a TV) to be present, so it
is not a bug in libcec or XBMC, but rather an unsupported scenario. Your TV might be older or your display device unusual 
(like a beamer or a computer monitor). In this case you cannot use handy CEC. Therefore i wrote "cec anyway", which ignores
the spec, captures button presses, and uses those to control XBMC.

How?
====

cec anyway is a tiny c++ app, which uses libcec and can be run as daemon in the backgroud. It communicates with xbmc using
its json-rpc api.