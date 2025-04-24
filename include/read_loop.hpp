#pragma once  // Ensures the file is only included once per translation unit

#include "config.h"

#include "buffer.hpp"
#include "pci_handler.hpp"
#include "rde/external_storer_file.hpp"
#include "rde/external_storer_interface.hpp"
#include "rde/rde_handler.hpp"

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <stdplus/fd/create.hpp>
#include <stdplus/fd/impl.hpp>
#include <stdplus/fd/managed.hpp>
#include <stdplus/print.hpp>

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <memory>

namespace bios_bmc_smm_error_logger {

    // Function declaration (make sure it matches the function signature in the .cpp file)
    void readLoop(
        boost::asio::basic_waitable_timer<std::chrono::_V2::steady_clock>* timer,
        std::shared_ptr<BufferInterface> buffer,
        std::shared_ptr<rde::RdeCommandHandler> rdeCommandHandler,
        const boost::system::error_code& ec
    );

}