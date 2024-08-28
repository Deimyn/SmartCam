const video = document.getElementById('videoStream');
const startButton = document.getElementById('startButton');
const stopButton = document.getElementById('stopButton');
let pc = null;
let signaling = null;

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
                console.log('Attaching video track to video element');
                video.srcObject = event.streams[0];
            }
        };

        pc.addTransceiver('video', { direction: 'recvonly' });

        const offer = await pc.createOffer();
        console.log('Created offer:', offer);
        await pc.setLocalDescription(offer);
        console.log('Set local description with offer');
        signaling.send(JSON.stringify({ type: 'offer', sdp: pc.localDescription.sdp }));
    };

    signaling.onmessage = async (event) => {
        const data = JSON.parse(event.data);
        console.log('Received signaling message:', data);

        if (data.type === 'answer') {
            console.log('Setting remote description with answer');
            await pc.setRemoteDescription(new RTCSessionDescription(data));
        } else if (data.type === 'candidate') {
            console.log('Adding ICE candidate:', data.ice);
            await pc.addIceCandidate(new RTCIceCandidate(data.ice));
        }
    };
});

stopButton.addEventListener('click', () => {
    if (pc) {
        // Close all tracks
        pc.getSenders().forEach(sender => sender.track && sender.track.stop());
        pc.getReceivers().forEach(receiver => receiver.track && receiver.track.stop());

        // Close the PeerConnection
        pc.getSenders().forEach(sender => pc.removeTrack(sender));
        pc.close();
        pc = null;
        console.log('RTCPeerConnection closed');
    }

    if (signaling) {
        signaling.close();
        signaling = null;
        console.log('WebSocket connection closed');
    }

    // Clear the video element
    video.srcObject = null;

    // Reset buttons
    startButton.disabled = false;
    stopButton.disabled = true;
});
