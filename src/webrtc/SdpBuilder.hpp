//
// Created by Laky64 on 28/07/23.
//

#ifndef NTGCALLS_SDP_BUILDER_HPP
#define NTGCALLS_SDP_BUILDER_HPP

#include <vector>
#include <string>
#include <optional>
#include <sstream>

struct Fingerprint {
    std::string hash;
    std::string fingerprint;
};

struct Candidate {
    std::string generation;
    std::string component;
    std::string protocol;
    std::string port;
    std::string ip;
    std::string foundation;
    std::string id;
    std::string priority;
    std::string type;
    std::string network;
};

struct Transport {
    std::string ufrag;
    std::string pwd;
    std::vector<Fingerprint> fingerprints;
    std::vector<Candidate> candidates;
};

struct Ssrc {
    uint32_t ssrc;
    std::vector<uint32_t> ssrc_group;
};

struct Conference {
    int64_t session_id;
    Transport transport;
    std::vector<Ssrc> ssrcs;
};

struct Sdp {
    std::optional<std::string> fingerprint;
    std::optional<std::string> hash;
    std::optional<std::string> setup;
    std::optional<std::string> pwd;
    std::optional<std::string> ufrag;
    uint32_t audioSource;
    std::vector<uint32_t> source_groups;
};

class SdpBuilder{
private:
    std::vector<std::string> lines;
    std::vector<std::string> newLine;

    void add(const std::string& line);
    void push(const std::string& word);
    void addJoined(const std::string& separator = "");
    void addCandidate(const Candidate& c);
    void addHeader(int64_t session_id);
    void addTransport(const Transport& transport);
    void addSsrcEntry(const Transport& transport);
    std::string join();
    std::string finalize();
    void addConference(const Conference& conference);

public:
    static std::string fromConference(const Conference& conference);
    static Sdp parseSdp(const std::string& sdp);
};

#endif