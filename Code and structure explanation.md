Here is some explanations about the different code files:

1. main.cpp

It contains the main C++ code that handles the WebRTC streaming server using GStreamer and Boost libraries.

Key Libraries:

-	GStreamer: Used for multimedia processing, particularly the WebRTC-related handling.
-	Boost.Beast: For handling WebSocket communication.
-	Boost.Asio: Networking support (TCP/IP).
- Boost.JSON: JSON processing.

Classes and Functions:

- class StreamSession: Manages the media pipeline and WebRTC session for streaming.
-	StreamSession(websocket::stream<tcp::socket>& ws): Constructor that initializes the WebRTC pipeline and stores the WebSocket stream reference.
-	~StreamSession(): Destructor that stops the pipeline and cleans up resources.
-	void start(): Starts the GStreamer pipeline, beginning the media stream.
-	void stop(): Stops the pipeline and releases resources.
-	void setup_pipeline(): Sets up the GStreamer pipeline elements and links them together.

> Components include:
> - v4l2src: Captures video from the camera.
> -	videoconvert: Converts video formats.
> -	queue: Buffers the stream.
> -	vp8enc: Encodes the video in VP8 format.
> -	rtpvp8pay: Encapsulates the encoded video in RTP packets.
> -	webrtcbin: Manages WebRTC-related tasks, including SDP negotiation and ICE candidates.

It also connects necessary signals for handling negotiation and ICE candidates.

-	void handle_sdp_offer(const std::string& sdp): Processes the SDP offer received from the client and initiates the WebRTC connection.
-	void handle_ice_candidate(const boost::json::object& ice): Handles incoming ICE candidates and adds them to the WebRTC session.
-	void on_answer_created(GstPromise *promise): Called when the WebRTC answer is created, sends the answer to the client via WebSocket.
-	void on_set_remote_description(GstPromise *promise): Sets the remote SDP description and triggers the creation of the answer.
-	void handle_websocket_session(tcp::socket socket): Handles the WebSocket session, receiving SDP offers and ICE candidates from the client, and forwarding them to the StreamSession.
-	void start_server(): Initializes the WebSocket server, accepting connections and handling them in separate threads.
-	int main(int argc, char* argv[]): Entry point of the program, initializes GStreamer, and starts the server.
  
2. CMakeLists.txt
   
This file contains the build configuration for CMake, which compiles your project.
Key Elements:

-	project(webrtc_server VERSION 1.0): Defines the project name and version.
-	set(CMAKE_CXX_STANDARD 14): Specifies the C++ standard to use.
-	find_package(PkgConfig REQUIRED): Finds the PkgConfig tool, which is used to locate GStreamer packages.
-	pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-webrtc-1.0 gstreamer-sdp-1.0): Checks for the required GStreamer modules.
-	find_package(Boost 1.65 REQUIRED COMPONENTS system filesystem json): Finds and includes the Boost libraries.
-	include_directories(${GST_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}): Adds include directories for GStreamer and Boost.
-	add_executable(webrtc_server main.cpp): Defines the executable target for the project.
-	target_link_libraries(webrtc_server ${GST_LIBRARIES} Boost::system Boost::filesystem Boost::json): Links the GStreamer and Boost libraries to the executable.
  
3. index.html and style.css

The main HTML file provides a user interface for starting and stopping the WebRTC stream.
The CSS file styles the HTML elements, providing a clean and responsive interface.

4. script.js
   
This JavaScript file handles the client-side WebRTC functionality, managing the PeerConnection and signaling.

-	const video = document.getElementById('videoStream');: References the video element.
-	let pc = null; let signaling = null;: Initializes variables for the RTCPeerConnection and WebSocket signaling.
-	startButton.addEventListener('click', async () => { ... });: Handles the click event for starting the stream.
-	pc = new RTCPeerConnection(...): Creates a new RTCPeerConnection with ICE server configuration.
-	signaling = new WebSocket(ws://${machine1_ip}:8000/ws);: Establishes a WebSocket connection with the server.
-	pc.onicecandidate = (event) => { ... };: Sends ICE candidates to the server.
-	pc.ontrack = (event) => { ... };: Attaches the received video track to the video element.
-	const offer = await pc.createOffer();: Creates an SDP offer and sets it as the local description.
-	signaling.onmessage = async (event) => { ... };: Handles incoming messages from the server (SDP answers and ICE candidates).
-	stopButton.addEventListener('click', () => { ... });: Handles the click event for stopping the stream, closing the connection and cleaning up resources.
  
5. webserver.py

This Python script serves the HTML, CSS, and JavaScript files to the client and dynamically inserts the correct IP address for WebRTC signaling.

-	get_local_ip(): Retrieves the local IP address of the machine running the web server.
-	get_machine1_ip(): Determines the IP address of the machine where the WebRTC server is running (machine 1).
-	class CustomHandler(SimpleHTTPRequestHandler): Custom HTTP request handler that serves files.
-	do_GET(self): Overrides the GET request handling:
-	If the request is for script.js, it reads the file, replaces the placeholder with the actual IP address, and serves it.
-	For other requests, it serves the files normally.
Main Section:
-	if __name__ == "__main__":: The main block that sets up and runs the HTTP server.
-	ip = get_local_ip(): Retrieves the IP address of the current machine.
-	httpd = HTTPServer(server_address, CustomHandler): Initializes the HTTP server with the custom request handler.
-	httpd.serve_forever(): Starts the server, serving files indefinitely.


Overall:
1.	Server Setup (main.cpp): The C++ application initializes a WebRTC server that handles incoming WebSocket connections and negotiates WebRTC streams using GStreamer.
2.	Web Interface (index.html, style.css, script.js): A user opens the web interface in a browser, which connects to the WebRTC server, initiates a WebRTC connection, and streams the camera feed.
3.	Web Server (webserver.py): A Python-based web server serves the web interface files, ensuring that the correct IP address is used for WebRTC signaling.
This combination of C++, Python, HTML, CSS, and JavaScript creates a functional WebRTC-based camera streaming system.

---

And now some explanations about WebRTC and the general structure of this project:

WebRTC (Web Real-Time Communication) is a technology that allows audio, video, and data sharing between browsers and devices in real time without the need for plugins or external software. It enables peer-to-peer communication and is primarily used for video conferencing, file sharing, and other forms of real-time media exchange over the internet.

The most important components of WebRTC are:
1.	Media Capture: Capturing audio and video from cameras, microphones, or other sources.
2.	Peer Connection (RTCPeerConnection): Establishing a direct connection between peers to transmit media and data.
3.	Signaling: Exchanging necessary information (like session descriptions and ICE candidates) to establish and manage the peer connection.
4.	Session Description Protocol (SDP): Describes multimedia communication sessions for signaling.
5.	Interactive Connectivity Establishment (ICE): A framework to allow WebRTC to traverse NATs (Network Address Translators) and firewalls.


![WebRTC diagram]()

This project implements a WebRTC-based video streaming system, consisting of a server-side C++ application (with GStreamer and Boost) and a client-side web interface. Here's how it functions:

1. Signaling Server (WebSocket)
WebRTC relies on signaling to establish a connection between peers. Signaling involves the exchange of messages containing SDP and ICE candidates. In your project, the signaling is handled via WebSockets.

-	Server-Side (main.cpp):
A WebSocket server is created using Boost.Beast and Boost.Asio. When a client connects, the server listens for SDP offers and ICE candidates. The server responds with an SDP answer and forwards ICE candidates back to the client. 
-	Client-Side (script.js):
The client establishes a WebSocket connection to the server. It sends an SDP offer (generated by the browser's RTCPeerConnection) to the server via the WebSocket. The client receives the SDP answer and ICE candidates from the server, which are necessary to establish the WebRTC connection.
  
2. RTCPeerConnection and Media Pipeline
The core of WebRTC is the RTCPeerConnection object, which manages the connection between the client and server.
- Server-Side (GStreamer in main.cpp):
A GStreamer pipeline is set up to capture video from a camera (v4l2src), encode it (vp8enc), and package it into RTP packets (rtpvp8pay). The webrtcbin element in GStreamer handles WebRTC tasks, including SDP negotiation and ICE management.	When the server receives an SDP offer from the client, it uses webrtcbin to set the remote description and generate an SDP answer. The server sends this SDP answer back to the client via the WebSocket.
-	Client-Side (script.js):
The client creates an RTCPeerConnection object and adds a transceiver for receiving video. Upon receiving the SDP answer from the server, the client sets it as the remote description. The client also processes incoming ICE candidates and adds them to the RTCPeerConnection. The received video stream is attached to the HTML <video> element, allowing the user to view the stream.

3. Interactive Connectivity Establishment (ICE)
ICE is a critical part of WebRTC that allows it to work across different network configurations, such as behind NATs and firewalls.
-	Server-Side (main.cpp): The webrtcbin element in GStreamer handles the gathering of ICE candidates. These candidates are sent to the client through the WebSocket connection. The server also receives ICE candidates from the client, which are added to the RTCPeerConnection.
-	Client-Side (script.js): The client gathers ICE candidates automatically and sends them to the server via the WebSocket. It also receives ICE candidates from the server and adds them to the RTCPeerConnection.

4. Media Transmission and Streaming
Once the WebRTC connection is established, the media (in this case, video) is transmitted between the server and client.
-	Server-Side (main.cpp):	The video stream is captured from the camera, processed, and sent over the WebRTC connection using RTP packets. The server sends the video track to the client, which is received and rendered in the client’s browser.
-	Client-Side (script.js): The client’s RTCPeerConnection receives the video stream. The video stream is attached to the HTML <video> element for display.


Overall Workflow in the Project

1.	Initialization:
The user opens the HTML page in their browser and clicks the "Start Stream" button. This triggers the creation of a WebRTC connection on the client side.
2.	Signaling:
The client sends an SDP offer to the server via WebSocket. The server processes the offer, sets up the GStreamer pipeline, and sends an SDP answer back to the client.
3.	ICE Candidate Exchange:
Both the server and client exchange ICE candidates to find the best path for communication.
4.	Media Streaming:
Once the connection is established, the server streams the video from the camera to the client. The client displays the video in the browser.
5.	Termination:
When the user clicks the "Stop Stream" button, the WebRTC connection is closed, and resources are freed on both the server and client sides.

