#define GST_USE_UNSTABLE_API
#include <gst/gst.h>
#include <gst/webrtc/webrtc.h>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <thread>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace boost::json;

#define STUN_SERVER "stun://stun.l.google.com:19302"
#define SERVER_PORT 8000

class StreamSession {
public:
    StreamSession(websocket::stream<tcp::socket>& ws)
        : ws(ws), pipeline(nullptr), webrtcbin(nullptr) {
        setup_pipeline();
    }

    ~StreamSession() {
        stop();
    }

    void start() {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
            std::cout << "Pipeline started" << std::endl;
        }
    }

    void stop() {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
            pipeline = nullptr;
            std::cout << "Pipeline stopped and resources released" << std::endl;
        }

        if (webrtcbin) {
            g_object_unref(webrtcbin);
            webrtcbin = nullptr;
        }
    }

private:
    GstElement* pipeline;
    GstElement* webrtcbin;
    websocket::stream<tcp::socket>& ws;

    void setup_pipeline() {
        pipeline = gst_pipeline_new("pipeline");
        GstElement *v4l2src = gst_element_factory_make("v4l2src", "source");
        GstElement *videoconvert = gst_element_factory_make("videoconvert", "convert");
        GstElement *queue = gst_element_factory_make("queue", "queue");
        GstElement *vp8enc = gst_element_factory_make("vp8enc", "encoder");
        GstElement *rtpvp8pay = gst_element_factory_make("rtpvp8pay", "pay");
        webrtcbin = gst_element_factory_make("webrtcbin", "sendrecv");

        if (!pipeline || !v4l2src || !videoconvert || !queue || !vp8enc || !rtpvp8pay || !webrtcbin) {
            g_printerr("Not all elements could be created.\n");
            return;
        }

        g_object_set(v4l2src, "device", "/dev/video4", NULL);
        g_object_set(vp8enc, "deadline", 1, NULL);

        gst_bin_add_many(GST_BIN(pipeline), v4l2src, videoconvert, queue, vp8enc, rtpvp8pay, webrtcbin, NULL);

        if (!gst_element_link_many(v4l2src, videoconvert, queue, vp8enc, rtpvp8pay, NULL)) {
            g_printerr("Elements could not be linked.\n");
            gst_object_unref(pipeline);
            return;
        }

        GstPad *rtp_src_pad = gst_element_get_static_pad(rtpvp8pay, "src");
        GstPad *webrtc_sink_pad = gst_element_request_pad_simple(webrtcbin, "sink_%u");
        gst_pad_link(rtp_src_pad, webrtc_sink_pad);
        gst_object_unref(rtp_src_pad);
        gst_object_unref(webrtc_sink_pad);

        g_signal_connect(webrtcbin, "on-negotiation-needed", G_CALLBACK(on_negotiation_needed), this);
        g_signal_connect(webrtcbin, "on-ice-candidate", G_CALLBACK(on_ice_candidate), this);
    }

    static void on_negotiation_needed(GstElement *webrtc, gpointer user_data) {
        std::cout << "Negotiation needed" << std::endl;
    }

    static void on_ice_candidate(GstElement *webrtc, guint mlineindex, gchar *candidate, gpointer user_data) {
        StreamSession* session = static_cast<StreamSession*>(user_data);
        session->send_ice_candidate_message(mlineindex, candidate);
    }

    void send_ice_candidate_message(guint mlineindex, gchar *candidate) {
        std::cout << "Sending ICE candidate: mlineindex=" << mlineindex << ", candidate=" << candidate << std::endl;

        object ice_json;
        ice_json["candidate"] = candidate;
        ice_json["sdpMLineIndex"] = mlineindex;

        object msg_json;
        msg_json["type"] = "candidate";
        msg_json["ice"] = ice_json;

        std::string text = serialize(msg_json);
        ws.write(net::buffer(text));

        std::cout << "ICE candidate sent" << std::endl;
    }

    void on_answer_created(GstPromise *promise) {
        std::cout << "Answer created" << std::endl;

        GstWebRTCSessionDescription *answer = NULL;
        const GstStructure *reply = gst_promise_get_reply(promise);
        gst_structure_get(reply, "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
        GstPromise *local_promise = gst_promise_new();
        g_signal_emit_by_name(webrtcbin, "set-local-description", answer, local_promise);

        object sdp_json;
        sdp_json["type"] = "answer";
        sdp_json["sdp"] = gst_sdp_message_as_text(answer->sdp);
        std::string text = serialize(sdp_json);
        ws.write(net::buffer(text));

        std::cout << "Local description set and answer sent: " << text << std::endl;

        gst_webrtc_session_description_free(answer);
    }

    void on_set_remote_description(GstPromise *promise) {
        std::cout << "Remote description set, creating answer" << std::endl;
        GstPromise *answer_promise = gst_promise_new_with_change_func(
            [](GstPromise *p, gpointer user_data) {
                static_cast<StreamSession*>(user_data)->on_answer_created(p);
            },
            this, NULL);

        g_signal_emit_by_name(webrtcbin, "create-answer", NULL, answer_promise);
    }

public:
    void handle_sdp_offer(const std::string& sdp) {
        GstSDPMessage *sdp_message;
        gst_sdp_message_new_from_text(sdp.c_str(), &sdp_message);
        GstWebRTCSessionDescription *offer = gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_OFFER, sdp_message);
        GstPromise *promise = gst_promise_new_with_change_func(
            [](GstPromise *p, gpointer user_data) {
                static_cast<StreamSession*>(user_data)->on_set_remote_description(p);
            },
            this, NULL);
        g_signal_emit_by_name(webrtcbin, "set-remote-description", offer, promise);
        gst_webrtc_session_description_free(offer);

        std::cout << "Setting remote description" << std::endl;
    }

    void handle_ice_candidate(const boost::json::object& ice) {
        std::string candidate = ice.at("candidate").as_string().c_str();
        guint sdpMLineIndex = ice.at("sdpMLineIndex").as_int64();
        g_signal_emit_by_name(webrtcbin, "add-ice-candidate", sdpMLineIndex, candidate.c_str());

        std::cout << "Added ICE candidate" << std::endl;
    }

};

void handle_websocket_session(tcp::socket socket) {
    try {
        websocket::stream<tcp::socket> ws{std::move(socket)};
        ws.accept();

        std::cout << "WebSocket connection accepted" << std::endl;

        StreamSession session(ws);
        session.start();

        for (;;) {
            beast::flat_buffer buffer;
            ws.read(buffer);

            auto text = beast::buffers_to_string(buffer.data());
            value jv = parse(text);
            object obj = jv.as_object();
            std::string type = obj["type"].as_string().c_str();

            if (type == "offer") {
                std::cout << "Received offer: " << text << std::endl;
                std::string sdp = obj["sdp"].as_string().c_str();
                session.handle_sdp_offer(sdp);
            } else if (type == "candidate") {
                std::cout << "Received ICE candidate: " << text << std::endl;
                object ice = obj["ice"].as_object();
                session.handle_ice_candidate(ice);
            }
        }

    } catch (beast::system_error const& se) {
        if (se.code() != websocket::error::closed) {
            std::cerr << "Error: " << se.code().message() << std::endl;
        }
    } catch (std::exception const& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void start_server() {
    std::cout << "Starting WebRTC server" << std::endl;
    try {
        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, tcp::endpoint{tcp::v4(), SERVER_PORT}};

        for (;;) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::cout << "Accepted new TCP connection" << std::endl;
            std::thread{handle_websocket_session, std::move(socket)}.detach();
        }
    } catch (std::exception const& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);
    start_server();
    return 0;
}
