#include "config.h"

#include "buffer.hpp"
#include "read_loop.hpp"
#include "pci_handler.hpp"
#include "rde/external_storer_file.hpp"
#include "rde/external_storer_interface.hpp"
#include "rde/rde_handler.hpp"
#include "read_loop.hpp" 

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

using namespace bios_bmc_smm_error_logger;

namespace {
  constexpr std::size_t memoryRegionSize = MEMORY_REGION_SIZE;
  constexpr std::size_t memoryRegionOffset = MEMORY_REGION_OFFSET;
}

class FakePciDataHandler : public DataInterface {
  public:
    explicit FakePciDataHandler(uint32_t RegionAddress, uint8_t *Data, size_t Size)
        : _regionAddress(RegionAddress), data_(Data), size_(Size) {}

    std::vector<uint8_t> read(uint32_t offset, uint32_t length) override {
      if (offset > size_ || length == 0)
        {
            stdplus::print(stderr,
                          "[read] Offset [{}] was bigger than regionSize [{}] "
                          "OR length [{}] was equal to 0\n",
                          offset, size_, length);
            return {};
        }

      // Read up to regionSize in case the offset + length overflowed
      uint32_t finalLength =
          (offset + length < size_) ? length : size_ - offset;
      return std::vector<uint8_t>(data_  + offset, data_ + finalLength);
    }

    uint32_t write(const uint32_t offset, const std::span<const uint8_t> bytes) override {
      std::copy(bytes.begin(), bytes.end(), data_ + offset);
      return bytes.size();
    }

    uint32_t getMemoryRegionSize() override {
      return size_;
    }

  private:
    uint8_t _regionAddress;
    uint8_t *data_;
    size_t size_;
};

class FakeStorer : public bios_bmc_smm_error_logger::rde::ExternalStorerInterface {
  public:
    bool publishJson(std::string_view jsonStr) override {
      jsonStr = "";
      return true;
    }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  // Copy input data to a writable buffer
  (void)Size;  // Mark as intentionally unused
  std::vector<uint8_t> buffer(Data, Data + memoryRegionSize);
  
  std::unique_ptr<DataInterface> pciDataHandler =
        std::make_unique<FakePciDataHandler>(memoryRegionOffset, buffer.data(), memoryRegionSize);

  std::shared_ptr<BufferInterface> bufferHandler =
        std::make_shared<BufferImpl>(std::move(pciDataHandler));

  std::unique_ptr<rde::ExternalStorerInterface> storer =
        std::make_unique<FakeStorer>();

  std::shared_ptr<rde::RdeCommandHandler> rdeCommandHandler =
        std::make_shared<rde::RdeCommandHandler>(std::move(storer));

  boost::system::error_code ec;
  bios_bmc_smm_error_logger::readLoop(
    static_cast<boost::asio::basic_waitable_timer<std::chrono::_V2::steady_clock>*>(nullptr),
    std::move(bufferHandler), std::move(rdeCommandHandler), ec
  );
  return 0;
}
