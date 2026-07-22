# rdphost

tiny windows app around rdp wrapper. checks if the wrapper/service/listener/ini look sane, can install/repair the files, and launches mstsc to 127.0.0.2 for a second local session.

put the wrapper stuff next to the exe in a res folder (rdpwrap.dll, rdpwrap.ini, rfxvmt.dll, optional rdpclip.exe), or keep them one level up while developing. run as admin for install.

# buttons

- check - status lines
- install / repair - copy files + registry + firewall + start TermService
- launch rdp - mstsc to 127.0.0.2
- copy status - clipboard dump of the check output

# build

open RdpHost.sln in visual studio and build (x64).

output ends up in bin\Release / bin\Debug

## bugs

if something breaks, spam me about it - what you did, what happened, windows build if you can. no formal bug report needed, just write it like a normal person.
