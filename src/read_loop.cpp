#include "config.h"
#include "buffer.hpp"
#include "rde/rde_handler.hpp"

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <stdplus/print.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <format>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

namespace bios_bmc_smm_error_logger {

    constexpr std::chrono::milliseconds readIntervalinMs(READ_INTERVAL_MS);
    constexpr uint32_t bmcInterfaceVersion = BMC_INTERFACE_VERSION;
    constexpr uint16_t queueSize = QUEUE_REGION_SIZE;
    constexpr uint16_t ueRegionSize = UE_REGION_SIZE;
    static constexpr std::array<uint32_t, 4> magicNumber = {
        MAGIC_NUMBER_BYTE1, MAGIC_NUMBER_BYTE2, MAGIC_NUMBER_BYTE3,
        MAGIC_NUMBER_BYTE4};

    void readLoop(boost::asio::steady_timer* t,
                  const std::shared_ptr<BufferInterface>& bufferInterface,
                  const std::shared_ptr<rde::RdeCommandHandler>& rdeCommandHandler,
                  const boost::system::error_code& error)
    {
        if (error)
        {
            stdplus::print(stderr, "Async wait failed {}\n", error.message());
            return;
        }
    
        try
        {
            std::vector<uint8_t> ueLog =
                bufferInterface->readUeLogFromReservedRegion();
            if (!ueLog.empty())
            {
                stdplus::print(
                    stdout,
                    "UE log found in reserved region, attempting to process\n");
    
                // UE log is BEJ encoded data, requiring RdeOperationInitRequest
                rde::RdeDecodeStatus ueDecodeStatus =
                    rdeCommandHandler->decodeRdeCommand(
                        ueLog, rde::RdeCommandType::RdeOperationInitRequest);
    
                if (ueDecodeStatus != rde::RdeDecodeStatus::RdeOk &&
                    ueDecodeStatus != rde::RdeDecodeStatus::RdeStopFlagReceived)
                {
                    throw std::runtime_error(std::format(
                        "Corruption detected processing UE log from reserved region. RDE decode status: {}",
                        static_cast<int>(ueDecodeStatus)));
                }
                stdplus::print(stdout, "UE log processed successfully.\n");
                // Successfully processed. Toggle BMC's view of ueSwitch flag.
                auto bufferHeader = bufferInterface->getCachedBufferHeader();
                uint32_t bmcSideFlags =
                    boost::endian::little_to_native(bufferHeader.bmcFlags);
                uint32_t newBmcFlags =
                    bmcSideFlags ^ static_cast<uint32_t>(BufferFlags::ueSwitch);
                bufferInterface->updateBmcFlags(newBmcFlags);
            }
    
            if (bufferInterface->checkForOverflowAndAcknowledge())
            {
                stdplus::print(
                    stdout,
                    "[WARN] Buffer overflow had occured and has been acked\n");
            }
    
            std::vector<EntryPair> entryPairs = bufferInterface->readErrorLogs();
            for (const auto& [entryHeader, entry] : entryPairs)
            {
                rde::RdeDecodeStatus rdeDecodeStatus =
                    rdeCommandHandler->decodeRdeCommand(
                        entry, static_cast<rde::RdeCommandType>(
                                   entryHeader.rdeCommandType));
                if (rdeDecodeStatus == rde::RdeDecodeStatus::RdeStopFlagReceived)
                {
                    auto bufferHeader = bufferInterface->getCachedBufferHeader();
                    auto newbmcFlags =
                        boost::endian::little_to_native(bufferHeader.bmcFlags) |
                        static_cast<uint32_t>(BmcFlags::ready);
                    bufferInterface->updateBmcFlags(newbmcFlags);
                }
            }
        }
        catch (const std::exception& e)
        {
            stdplus::print(
                stderr,
                "Error during log processing (std::exception): {}. Attempting to reinitialize buffer.\n",
                e.what());
            try
            {
                bufferInterface->initialize(bmcInterfaceVersion, queueSize,
                                            ueRegionSize, magicNumber);
                stdplus::print(
                    stdout,
                    "Buffer reinitialized successfully after std::exception.\n");
            }
            catch (const std::exception& reinit_e)
            {
                stdplus::print(
                    stderr,
                    "CRITICAL: Failed to reinitialize buffer (std::exception): {}. Terminating read loop.\n",
                    reinit_e.what());
                return;
            }
            catch (...)
            {
                stdplus::print(
                    stderr,
                    "CRITICAL: Failed to reinitialize buffer (unknown exception). Terminating read loop.\n");
                return;
            }
        }
        catch (...)
        {
            stdplus::print(
                stderr,
                "Unknown error during log processing. Attempting to reinitialize buffer.\n");
            try
            {
                bufferInterface->initialize(bmcInterfaceVersion, queueSize,
                                            ueRegionSize, magicNumber);
                stdplus::print(
                    stdout,
                    "Buffer reinitialized successfully after unknown error.\n");
            }
            catch (const std::exception& reinit_e)
            {
                stdplus::print(
                    stderr,
                    "CRITICAL: Failed to reinitialize buffer (std::exception): {}. Terminating read loop.\n",
                    reinit_e.what());
                return;
            }
            catch (...)
            {
                stdplus::print(
                    stderr,
                    "CRITICAL: Failed to reinitialize buffer (unknown exception). Terminating read loop.\n");
                return;
            }
        }
    
        if (t != nullptr) {
            t->expires_after(readIntervalinMs);
            t->async_wait(
                std::bind_front(readLoop, t, bufferInterface, rdeCommandHandler));
        }
    }
}
