Initialistaion :

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
python3 -m http.server 8080
```

Finally go check the at http://localhost:8080/
