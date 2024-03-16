//
// Created by Laky64 on 16/03/2024.
//

#pragma once
#include <optional>
#include <string>
#include <vector>
#include <rtc_base/thread.h>

#include "signaling_encryption.hpp"
#include "../utils/auth_key.hpp"

namespace ntgcalls {

    class SignalingInterface {
    public:
        virtual ~SignalingInterface() = default;

        enum class ProtocolVersion{
            V1,
            V2
        };

        SignalingInterface(
            rtc::Thread* networkThread,
            rtc::Thread* signalingThread,
            bool isOutGoing,
            const Key &key,
            const std::function<void(const bytes::binary&)>& onEmitData,
            const std::function<void(const std::optional<bytes::binary>&)>& onSignalData
        );

        virtual void send(const bytes::binary& data) = 0;

        virtual void receive(const bytes::binary& data) const = 0;

        static ProtocolVersion signalingVersion(const std::vector<std::string>& versions);

    protected:
        std::function<void(const std::optional<bytes::binary>&)> onSignalData;
        std::function<void(const bytes::binary&)> onEmitData;
        rtc::Thread *networkThread, *signalingThread;

        [[nodiscard]] std::optional<bytes::binary> preProcessData(const bytes::binary& data, bool isOut) const;

        [[nodiscard]] virtual bool supportsCompression() const = 0;

    private:
        std::shared_ptr<SignalingEncryption> signalingEncryption;
        static constexpr std::string defaultVersion = "11.0.0";
    };
} // ntgcalls
