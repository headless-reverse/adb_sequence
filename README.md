**Struktura Sequence JSON**
```ini
[
  {
    "command": "cat /proc/meminfo",
    "delayAfterMs": 500,
    "runMode": "shell",
    "stopOnError": true
  },
  {
    "command": "reboot",
    "delayAfterMs": 10000,
    "runMode": "adb",
    "stopOnError": false
  },
  {
    "command": "ip route",
    "delayAfterMs": 100,
    "runMode": "root",
    "stopOnError": true
  }
]
```

runMode: adb   →   brak prefiksu  
runMode: root  →   adb shell su -c "  
runMode: shell →   adb shell "  

***________________________________________***
```
cmake -B build            
cmake --build build -j$(nproc)
```
<img width="1352" height="745" alt="sequence" src="https://github.com/user-attachments/assets/caf55895-2093-40e5-8117-b84522c7c593" />
