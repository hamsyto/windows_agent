// lz4_handler.h
#ifndef AGENT_LZ4_HANDLER_H
#define AGENT_LZ4_HANDLER_H

#include <consts.h>

#include <stdexcept>
#include <vector>

namespace agent::compression {

/**
 * @brief Сжимает данные с помощью LZ4
 * @param data Исходные данные для сжатия
 * @return Сжатые данные
 * @throws std::runtime_error при ошибках сжатия
 */
std::vector<uint8_t> compress_lz4(const std::vector<uint8_t>& data);

/**
 * @brief Распаковывает данные с помощью LZ4
 * @param compressed_data Сжатые данные
 * @return Распакованные данные
 * @throws std::runtime_error при ошибках распаковки
 */
std::vector<uint8_t> decompress_lz4(
    const std::vector<uint8_t>& compressed_data);

}  // namespace agent::compression

#endif  // AGENT_LZ4_HANDLER_H