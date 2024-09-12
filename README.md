Initialisation :

Make sure you have all the [requirements](https://github.com/Deimyn/SmartCam/blob/main/requirements.txt) in your environment.

Once the repo is cloned create a directory called "build" then use cmake to build and compile :

```
mkdir build && cd build 
cmake ..
make
```

 
How to use :

In build/ :
```
./webrtc_server
```

and in public/ :
```
python3 webserver.py
```

Finally go check the at ht<span>tp://givenIP:8080/ (on any device in the same network).

You can check [usefull](https://github.com/Deimyn/SmartCam/blob/main/usefull.txt) for testing purposes and also [code and structure explanation](https://github.com/Deimyn/SmartCam/blob/main/Code%20and%20structure%20explanation.md). 

