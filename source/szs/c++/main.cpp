// See
// https://github.com/riidefi/RiiStudio/blob/master/source/szs/include/szs.h
//
// Main APIs:
//
// Result<std::vector<uint8_t>> encode(std::span<const uint8_t> buf,
//                                    Algo algo);
//
#include "szs.h"

#include <array>
#include <iostream>
#include <vector>

int main() {
  std::string version = szs::get_version();
  std::cout << "szs version: " << version << std::endl;

  std::string sampleText =
      "Hello, world! This is a sample string for testing the szs compression "
      "library."
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHello, world!";

  // Convert string to byte array (UTF-8)
  std::vector<uint8_t> originalData(sampleText.begin(), sampleText.end());

  std::cout << "Original Data:\n";
  std::cout << sampleText << "\n";
  std::cout << "Original Size: " << originalData.size() << " bytes\n\n";

  // Check if data is compressed
  bool isCompressed = szs::is_compressed(originalData);
  std::cout << "Is Original Data Compressed? "
            << (isCompressed ? "true" : "false") << "\n\n";

  // Encode data
  auto maybe_encodedData = szs::encode(originalData, szs::Algo::LibYaz0);
  if (!maybe_encodedData) {
    std::cout << "Failed to encode: " << maybe_encodedData.error() << std::endl;
    return -1;
  }
  std::vector<uint8_t> encodedData = *maybe_encodedData;

  std::cout << "Encoded Size: " << encodedData.size() << " bytes\n\n";

  // Decode data
  auto maybe_decodedData = szs::decode(encodedData);

  if (!maybe_decodedData) {
    std::cout << "Failed to decode: " << maybe_decodedData.error() << std::endl;
    return -1;
  }
  std::vector<uint8_t> decodedData = *maybe_decodedData;

  std::string decodedText((const char*)decodedData.data(), decodedData.size());
  std::cout << "Decoded Data:\n";
  std::cout << decodedText << "\n\n";

  // Check if encoded data is compressed
  bool isEncodedDataCompressed = szs::is_compressed(encodedData);
  std::cout << "Is Encoded Data Compressed? "
            << (isEncodedDataCompressed ? "true" : "false") << "\n\n";

  return 0;
}
