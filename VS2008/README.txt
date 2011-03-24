i have an uncomplete M$VC installation, hence everything is rather complicated:

this is how i built NatNat:

- open the vcproj with M$VC 2008
- press F7 to "build" the 'Release' (this will compile all the sources, but will
  not link them, due to my missing rc.exe/mc.exe)
- open the VC2008 shell
- cd ....\NatNat\VS2008 (this folder)
- cd to NatNat\NatNat\Release
- run "name -f ..\Makefile"
- you should now have a valid NatNat.exe file in the Release folder (where you
  are)

run it
======
NaturalPoint tracking server (192.168.7.229)
 - broadcast data via the "Streaming Properties" panel
   - Type: Multicast
   - Command Port: 1501
   - Data Port: 1511

   - Local Interface: Preferred/192.168.7.229
   - Multicast Interface: 239.255.42.99

OSC receiver (192.168.7.141)
 - listen on UDP/10000

NatNat client (192.168.7.41)
  - run:
      NatNat.exe 192.168.7.229 192.168.7.41 192.168.7.141 10000

