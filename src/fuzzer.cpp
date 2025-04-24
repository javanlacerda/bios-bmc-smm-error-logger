#include "config.h"

#include "buffer.hpp"
#include "read_loop.cpp"
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

class FakePciDataHandler : public DataInterface {
  public:
    explicit FakePciDataHandler(uint8_t *Data, size_t Size)
        : data_(Data), size_(Size) {}

    std::vector<uint8_t> read(uint32_t offset, uint32_t length) override {
      return std::vector<uint8_t>(data_ + offset, data_ + offset + length);
    }

    uint32_t write(const uint32_t offset, const std::span<const uint8_t> bytes) override {
      std::copy(bytes.begin(), bytes.end(), data_ + offset);
      return bytes.size();
    }

    uint32_t getMemoryRegionSize() override {
      return size_;
    }

  private:
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
  std::vector<uint8_t> buffer(Data, Data + Size);

  std::unique_ptr<DataInterface> pciDataHandler =
        std::make_unique<FakePciDataHandler>(buffer.data(), buffer.size());

  std::shared_ptr<BufferInterface> bufferHandler =
        std::make_shared<BufferImpl>(std::move(pciDataHandler));

  std::unique_ptr<rde::ExternalStorerInterface> storer =
        std::make_unique<FakeStorer>();

  std::shared_ptr<rde::RdeCommandHandler> rdeCommandHandler =
        std::make_shared<rde::RdeCommandHandler>(std::move(storer));

  boost::system::error_code ec;
  readLoop(
    static_cast<boost::asio::basic_waitable_timer<std::chrono::_V2::steady_clock>*>(nullptr),
    std::move(bufferHandler), std::move(rdeCommandHandler), ec
  );
  return 0;
}
