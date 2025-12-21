// lz4_handler.cpp
#include "lz4_handler.h"

#include <cstring>

#include "../../libs/lz4/lz4.h"
#include "../../libs/lz4/lz4hc.h"

namespace agent::compression {

std::vector<uint8_t> compress_lz4(const std::vector<uint8_t>& data) {
    if (data.empty()) return {};

    // Оцениваем максимальный размер сжатых данных
    int max_compressed_size = LZ4_compressBound(static_cast<int>(data.size()));
    if (max_compressed_size <= 0) {
        throw std::runtime_error("Failed to estimate compressed size");
    }

    // Выделяем буфер для сжатых данных
    std::vector<uint8_t> compressed(static_cast<size_t>(max_compressed_size));

    // Сжимаем данные
    int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(compressed.data()),
        static_cast<int>(data.size()), max_compressed_size);

    if (compressed_size <= 0) {
        // Пробуем HC компрессию (более медленная, но лучшее сжатие)
        compressed_size =
            LZ4_compress_HC(reinterpret_cast<const char*>(data.data()),
                            reinterpret_cast<char*>(compressed.data()),
                            static_cast<int>(data.size()), max_compressed_size,
                            kLZ4CompressionLevel);

        if (compressed_size <= 0) {
            throw std::runtime_error("LZ4 compression failed");
        }
    }

    // Уменьшаем буфер до реального размера
    compressed.resize(static_cast<size_t>(compressed_size));
    return compressed;
}

std::vector<uint8_t> decompress_lz4(
    const std::vector<uint8_t>& compressed_data) {
    if (compressed_data.empty()) return {};
    size_t estimated_size = compressed_data.size() * 2;
    for (int attempt = 0; attempt < 3; ++attempt) {
        // Ограничиваем максимальный размер
        if (estimated_size > kMaxDecompressedSize) {
            estimated_size = kMaxDecompressedSize;
        }

        std::vector<uint8_t> decompressed(estimated_size);

        int result = LZ4_decompress_safe(
            reinterpret_cast<const char*>(compressed_data.data()),
            reinterpret_cast<char*>(decompressed.data()),
            static_cast<int>(compressed_data.size()),
            static_cast<int>(estimated_size));

        if (result > 0) {
            // Успех
            decompressed.resize(static_cast<size_t>(result));
            return decompressed;
        } else if (result == 0) {
            // Буфер слишком мал, увеличиваем
            estimated_size *= 2;
        } else {
            // Ошибка
            throw std::runtime_error(
                "LZ4 decompression failed: corrupted data");
        }
    }

    throw std::runtime_error("Failed to decompress LZ4 data: buffer too small");
}

}  // namespace agent::compression