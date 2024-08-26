const video = document.getElementById('videoStream');
const startButton = document.getElementById('startButton');
const stopButton = document.getElementById('stopButton');
const videoSelect = document.getElementById('videoSelect');
let pc = null;
let signaling = null;
let selectedDeviceId = null;

navigator.mediaDevices.enumerateDevices().then(gotDevices);

videoSelect.onchange = () => selectedDeviceId = videoSelect.value;

startButton.addEventListener('click', async () => {
    startButton.disabled = true;
    stopButton.disabled = false;

    pc = new RTCPeerConnection({
        iceServers: [{ urls: 'stun:stun.l.google.com:19302' }]
    });

    const machine1_ip = 'MACHINE_1_IP_PLACEHOLDER';
    signaling = new WebSocket(`ws://${machine1_ip}:8000/ws`);

    signaling.onopen = async () => {
        console.log('WebSocket connection established');
        
        pc.onicecandidate = (event) => {
            if (event.candidate) {
                console.log('Sending ICE candidate:', event.candidate);
                signaling.send(JSON.stringify({
                    type: 'candidate',
                    ice: event.candidate
                }));
            }
        };

        pc.ontrack = (event) => {
            console.log('Received track:', event);
            if (event.track.kind === 'video') {
                video.srcObject = event.streams[0];
            }
        };

        const constraints = {
            video: { deviceId: selectedDeviceId ? { exact: selectedDeviceId } : undefined }
        };

        const stream = await navigator.mediaDevices.getUserMedia(constraints);
        stream.getTracks().forEach(track => pc.addTrack(track, stream));

        const offer = await pc.createOffer();
        await pc.setLocalDescription(offer);
        signaling.send(JSON.stringify({ type: 'offer', sdp: pc.localDescription.sdp }));
    };

    signaling.onmessage = async (event) => {
        const data = JSON.parse(event.data);
        if (data.type === 'answer') {
            await pc.setRemoteDescription(new RTCSessionDescription(data));
        } else if (data.type === 'candidate') {
            await pc.addIceCandidate(new RTCIceCandidate(data.ice));
        }
    };
});

stopButton.addEventListener('click', () => {
    if (pc) {
        pc.close();
        pc = null;
    }

    if (signaling) {
        signaling.close();
        signaling = null;
    }

    video.srcObject = null;
    startButton.disabled = false;
    stopButton.disabled = true;
});

function gotDevices(deviceInfos) {
    for (let i = 0; i !== deviceInfos.length; ++i) {
        const deviceInfo = deviceInfos[i];
        const option = document.createElement('option');
        option.value = deviceInfo.deviceId;
        if (deviceInfo.kind === 'videoinput') {
            option.text = deviceInfo.label || `Camera ${videoSelect.length + 1}`;
            videoSelect.appendChild(option);
        }
    }
}
