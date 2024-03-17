//
// Created by Laky64 on 12/08/2023.
//
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../ntgcalls.hpp"
#include "ntgcalls/exceptions.hpp"
#include "wrtc/models/rtc_server.hpp"

namespace py = pybind11;

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

template <typename T, typename = std::enable_if_t<std::is_same_v<T, bytes::vector> || std::is_same_v<T, bytes::binary>>>
T toCBytes(const py::bytes& p) {
    const auto data = reinterpret_cast<const uint8_t*>(PYBIND11_BYTES_AS_STRING(p.ptr()));
    const auto size = static_cast<size_t>(PYBIND11_BYTES_SIZE(p.ptr()));
    auto sharedPtr = T(size);
    std::memcpy(sharedPtr.data(), data, size);
    return sharedPtr;
}

template <typename T, typename = std::enable_if_t<std::is_same_v<T, bytes::vector> || std::is_same_v<T, bytes::binary>>>
std::optional<T> toCBytes(const std::optional<py::bytes>& p) {
    if (p) {
        return toCBytes<T>(p.value());
    }
    return std::nullopt;
}

template <typename T, typename = std::enable_if_t<std::is_same_v<T, bytes::vector> || std::is_same_v<T, bytes::binary>>>
py::bytes toBytes(const T& p) {
    return {reinterpret_cast<const char*>(p.data()), p.size()};
}

PYBIND11_MODULE(ntgcalls, m) {
    class PyRTCServer: public wrtc::RTCServer {
    public:
        PyRTCServer(
            const uint64_t id,
            const std::string &ipv4,
            const std::string &ipv6,
            const uint16_t port,
            const std::optional<std::string>& username,
            const std::optional<std::string>& password,
            const bool turn,
            const bool stun,
            const bool tcp,
            const std::optional<py::bytes>& peerTag
        ): RTCServer(id, ipv4, ipv6, port, username, password, turn, stun, tcp, toCBytes<bytes::binary>(peerTag)) {}
    };

    py::class_<ntgcalls::NTgCalls> wrapper(m, "NTgCalls");
    wrapper.def(py::init<>());
    wrapper.def("create_p2p_call", [](ntgcalls::NTgCalls& self, const int64_t userId, const int32_t g, const py::bytes& p, const py::bytes& r, const std::optional<py::bytes>& g_a_hash) {
        return toBytes(self.createP2PCall(userId, g, toCBytes<bytes::vector>(p), toCBytes<bytes::vector>(r), toCBytes<bytes::vector>(g_a_hash)));
    }, py::arg("user_id"), py::arg("g"), py::arg("p"), py::arg("r"), py::arg("g_a_hash"));
    wrapper.def("exchange_keys", [](ntgcalls::NTgCalls& self, const int64_t userId, const py::bytes& p, const py::bytes& g_a_or_b, const int64_t fingerprint) {
        return self.exchangeKeys(userId, toCBytes<bytes::vector>(p), toCBytes<bytes::vector>(g_a_or_b), fingerprint);
    }, py::arg("user_id"), py::arg("p"), py::arg("g_a_or_b"), py::arg("fingerprint"));
    wrapper.def("connect_p2p", [](ntgcalls::NTgCalls& self, const int64_t userId, const std::vector<PyRTCServer>& servers, const std::vector<std::string>& versions) {
        pybind11::gil_scoped_release release;
        std::vector<wrtc::RTCServer> serversTmp;
        for (const auto& server: servers) {
            serversTmp.push_back(server);
        }
        self.connectP2P(userId, serversTmp, versions);
    }, py::arg("user_id"), py::arg("servers"), py::arg("versions"));
    wrapper.def("send_signaling", [] (ntgcalls::NTgCalls& self, const int64_t chatId, const py::bytes& msgKey) {
        self.sendSignalingData(chatId, toCBytes<bytes::binary>(msgKey));
    }, py::arg("chat_id"), py::arg("msg_key"));
    wrapper.def("create_call", &ntgcalls::NTgCalls::createCall, py::arg("chat_id"), py::arg("media"));
    wrapper.def("connect", &ntgcalls::NTgCalls::connect, py::arg("chat_id"), py::arg("params"));
    wrapper.def("change_stream", &ntgcalls::NTgCalls::changeStream, py::arg("chat_id"), py::arg("media"));
    wrapper.def("pause", &ntgcalls::NTgCalls::pause, py::arg("chat_id"));
    wrapper.def("resume", &ntgcalls::NTgCalls::resume, py::arg("chat_id"));
    wrapper.def("mute", &ntgcalls::NTgCalls::mute, py::arg("chat_id"));
    wrapper.def("unmute", &ntgcalls::NTgCalls::unmute, py::arg("chat_id"));
    wrapper.def("stop", &ntgcalls::NTgCalls::stop, py::arg("chat_id"));
    wrapper.def("time", &ntgcalls::NTgCalls::time, py::arg("chat_id"));
    wrapper.def("get_state", &ntgcalls::NTgCalls::getState, py::arg("chat_id"));
    wrapper.def("on_upgrade", &ntgcalls::NTgCalls::onUpgrade);
    wrapper.def("on_stream_end", &ntgcalls::NTgCalls::onStreamEnd);
    wrapper.def("on_disconnect", &ntgcalls::NTgCalls::onDisconnect);
    wrapper.def("on_signaling", [](ntgcalls::NTgCalls& self, const std::function<void(int64_t, py::bytes)>& callback) {
        py::gil_scoped_release release;
        self.onSignalingData([callback](const int64_t chatId, const bytes::binary& data) {
            py::gil_scoped_acquire acquire;
            callback(chatId, toBytes(data));
        });
    }, py::arg("callback"));
    wrapper.def("calls", &ntgcalls::NTgCalls::calls);
    wrapper.def("cpu_usage", &ntgcalls::NTgCalls::cpuUsage);
    wrapper.def_static("ping", &ntgcalls::NTgCalls::ping);
    wrapper.def_static("get_protocol", &ntgcalls::NTgCalls::getProtocol);

    py::enum_<ntgcalls::Stream::Type>(m, "StreamType")
            .value("Audio", ntgcalls::Stream::Type::Audio)
            .value("Video", ntgcalls::Stream::Type::Video)
            .export_values();

    py::enum_<ntgcalls::Stream::Status>(m, "StreamStatus")
            .value("Playing", ntgcalls::Stream::Status::Playing)
            .value("Paused", ntgcalls::Stream::Status::Paused)
            .value("Idling", ntgcalls::Stream::Status::Idling)
            .export_values();

    py::enum_<ntgcalls::BaseMediaDescription::InputMode>(m, "InputMode")
            .value("File", ntgcalls::BaseMediaDescription::InputMode::File)
            .value("Shell", ntgcalls::BaseMediaDescription::InputMode::Shell)
            .value("FFmpeg", ntgcalls::BaseMediaDescription::InputMode::FFmpeg)
            .value("NoLatency", ntgcalls::BaseMediaDescription::InputMode::NoLatency)
            .export_values()
            .def("__and__",[](const ntgcalls::BaseMediaDescription::InputMode& lhs, const ntgcalls::BaseMediaDescription::InputMode& rhs) {
                return static_cast<ntgcalls::BaseMediaDescription::InputMode>(lhs & rhs);
            })
            .def("__and__",[](const ntgcalls::BaseMediaDescription::InputMode& lhs, const int rhs) {
                return static_cast<ntgcalls::BaseMediaDescription::InputMode>(lhs & rhs);
            })
            .def("__or__",[](const ntgcalls::BaseMediaDescription::InputMode& lhs, const ntgcalls::BaseMediaDescription::InputMode& rhs) {
                return static_cast<ntgcalls::BaseMediaDescription::InputMode>(lhs | rhs);
            })
            .def("__or__",[](const ntgcalls::BaseMediaDescription::InputMode& lhs, const int rhs) {
                return static_cast<ntgcalls::BaseMediaDescription::InputMode>(lhs | rhs);
            });


    py::class_<ntgcalls::MediaState>(m, "MediaState")
            .def_readonly("muted", &ntgcalls::MediaState::muted)
            .def_readonly("video_stopped", &ntgcalls::MediaState::videoStopped)
            .def_readonly("video_paused", &ntgcalls::MediaState::videoPaused);

    py::class_<ntgcalls::BaseMediaDescription> mediaWrapper(m, "BaseMediaDescription");
    mediaWrapper.def_readwrite("input", &ntgcalls::BaseMediaDescription::input);

    py::class_<ntgcalls::AudioDescription> audioWrapper(m, "AudioDescription", mediaWrapper);
    audioWrapper.def(
            py::init<ntgcalls::BaseMediaDescription::InputMode, uint32_t, uint8_t, uint8_t, std::string>(),
            py::arg("input_mode"),
            py::arg("sample_rate"),
            py::arg("bits_per_sample"),
            py::arg("channel_count"),
            py::arg("input")
    );
    audioWrapper.def_readwrite("sampleRate", &ntgcalls::AudioDescription::sampleRate);
    audioWrapper.def_readwrite("bitsPerSample", &ntgcalls::AudioDescription::bitsPerSample);
    audioWrapper.def_readwrite("channelCount", &ntgcalls::AudioDescription::channelCount);

    py::class_<ntgcalls::VideoDescription> videoWrapper(m, "VideoDescription", mediaWrapper);
    videoWrapper.def(
            py::init<ntgcalls::BaseMediaDescription::InputMode, uint16_t, uint16_t, uint8_t, std::string>(),
            py::arg("input_mode"),
            py::arg("width"),
            py::arg("height"),
            py::arg("fps"),
            py::arg("input")
    );
    videoWrapper.def_readwrite("width", &ntgcalls::VideoDescription::width);
    videoWrapper.def_readwrite("height", &ntgcalls::VideoDescription::height);
    videoWrapper.def_readwrite("fps", &ntgcalls::VideoDescription::fps);

    py::class_<ntgcalls::MediaDescription> mediaDescWrapper(m, "MediaDescription");
    mediaDescWrapper.def(
            py::init<std::optional<ntgcalls::AudioDescription>, std::optional<ntgcalls::VideoDescription>>(),
            py::arg_v("audio", std::nullopt, "None"),
            py::arg_v("video", std::nullopt, "None")
    );
    mediaDescWrapper.def_readwrite("audio", &ntgcalls::MediaDescription::audio);
    mediaDescWrapper.def_readwrite("video", &ntgcalls::MediaDescription::video);

    py::class_<ntgcalls::Protocol> protocolWrapper(m, "Protocol");
    protocolWrapper.def(py::init<>());
    protocolWrapper.def_readwrite("min_layer", &ntgcalls::Protocol::min_layer);
    protocolWrapper.def_readwrite("max_layer", &ntgcalls::Protocol::max_layer);
    protocolWrapper.def_readwrite("udp_p2p", &ntgcalls::Protocol::udp_p2p);
    protocolWrapper.def_readwrite("udp_reflector", &ntgcalls::Protocol::udp_reflector);
    protocolWrapper.def_readwrite("library_versions", &ntgcalls::Protocol::library_versions);

    py::class_<PyRTCServer> rtcServerWrapper(m, "RTCServer");
    rtcServerWrapper.def(py::init<uint64_t, std::string, std::string, uint16_t, std::optional<std::string>, std::optional<std::string>, bool, bool, bool, std::optional<py::bytes>>());

    py::class_<ntgcalls::AuthParams> authParamsWrapper(m, "AuthParams");
    authParamsWrapper.def(py::init<>());
    authParamsWrapper.def_property_readonly("g_a_or_b", [](const ntgcalls::AuthParams& self) {
        return toBytes(self.g_a_or_b);
    });
    authParamsWrapper.def_readwrite("key_fingerprint", &ntgcalls::AuthParams::key_fingerprint);

    // Exceptions
    const pybind11::exception<wrtc::BaseRTCException> baseExc(m, "BaseRTCException");
    pybind11::register_exception<wrtc::SdpParseException>(m, "SdpParseException", baseExc);
    pybind11::register_exception<wrtc::RTCException>(m, "RTCException", baseExc);
    pybind11::register_exception<ntgcalls::ConnectionError>(m, "ConnectionError", baseExc);
    pybind11::register_exception<ntgcalls::TelegramServerError>(m, "TelegramServerError", baseExc);
    pybind11::register_exception<ntgcalls::ConnectionNotFound>(m, "ConnectionNotFound", baseExc);
    pybind11::register_exception<ntgcalls::InvalidParams>(m, "InvalidParams", baseExc);
    pybind11::register_exception<ntgcalls::RTMPNeeded>(m, "RTMPNeeded", baseExc);
    pybind11::register_exception<ntgcalls::FileError>(m, "FileError", baseExc);
    pybind11::register_exception<ntgcalls::FFmpegError>(m, "FFmpegError", baseExc);
    pybind11::register_exception<ntgcalls::ShellError>(m, "ShellError", baseExc);

    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
}