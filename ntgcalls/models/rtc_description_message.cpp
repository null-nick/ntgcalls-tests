//
// Created by Laky64 on 22/03/2024.
//

#include "rtc_description_message.hpp"

#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {
    bytes::binary RtcDescriptionMessage::serialize() const {
        return bytes::make_binary(to_string(json{
            {"@type", wrtc::Description::SdpTypeToString(type)},
            {"sdp", sdp},
        }));
    }

    std::unique_ptr<RtcDescriptionMessage> RtcDescriptionMessage::deserialize(const bytes::binary& data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<RtcDescriptionMessage>();
        if (j["@type"] != "offer" && j["@type"] != "answer") {
            RTC_LOG(LS_ERROR) << "Invalid sdp type: " << j["@type"];
            throw InvalidParams("Invalid sdp type");
        }
        message->type = j["@type"] == "offer" ? wrtc::Description::SdpType::Offer : wrtc::Description::SdpType::Answer;
        message->sdp = j["sdp"];
        return std::move(message);
    }
} // ntgcalls