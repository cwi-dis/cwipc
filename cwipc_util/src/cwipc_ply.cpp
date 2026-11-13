//
// PLY file reading and writing for cwipc (#66), replacing PCL's PLYReader/PLYWriter.
//
// File layout written (see the decisions on #66):
//
//   ply
//   format {ascii|binary_little_endian} 1.0
//   comment written by cwipc
//   element vertex N
//   property float x
//   property float y
//   property float z
//   property uchar red
//   property uchar green
//   property uchar blue
//   property uchar tile
//   element cwipc_metadata 1
//   property double timestamp      (seconds; cwipc timestamps are milliseconds)
//   property float cellsize
//   [comment padding xxx...]       (binary only: header length a multiple of 16)
//   end_header
//
// In binary files the vertex data therefore has exactly the struct cwipc_point layout,
// starting at a 16-byte aligned offset (#338).
//
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <clocale>
#include <charconv>
#include <fstream>
#include <string>
#include <vector>

#include "cwipc_util/api.h"
#include "cwipc_util/internal/logging.hpp"
#include "cwipc_util/internal/ply.hpp"

static_assert(sizeof(struct cwipc_point) == 16, "struct cwipc_point must be 16 bytes for the PLY binary layout");

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#error "cwipc_ply.cpp writes binary PLY in host byte order, which must be little-endian"
#endif

static const char* PLY_WRITE_MODULE = "cwipc_write";
static const char* PLY_READ_MODULE = "cwipc_read";

// Header up to and including the property list of the metadata element.
static std::string _ply_header_body(bool binary, int npoints) {
    std::string h;
    h += "ply\n";
    h += binary ? "format binary_little_endian 1.0\n" : "format ascii 1.0\n";
    h += "comment written by cwipc\n";
    h += "element vertex " + std::to_string(npoints) + "\n";
    h += "property float x\n";
    h += "property float y\n";
    h += "property float z\n";
    h += "property uchar red\n";
    h += "property uchar green\n";
    h += "property uchar blue\n";
    h += "property uchar tile\n";
    h += "element cwipc_metadata 1\n";
    h += "property double timestamp\n";
    h += "property float cellsize\n";
    return h;
}

static std::string _ply_header(bool binary, int npoints) {
    std::string h = _ply_header_body(binary, npoints);
    const std::string end = "end_header\n";

    if (binary) {
        // Pad with a comment line so the total header length is a multiple of 16.
        const std::string pad_start = "comment padding ";
        size_t len = h.size() + pad_start.size() + 1 /* newline */ + end.size();
        size_t npad = (16 - len % 16) % 16;
        h += pad_start + std::string(npad, 'x') + "\n";
    }

    h += end;
    return h;
}

// Append a number to buf, as the shortest text that reads back to the same value,
// independent of the C locale. Floats and doubles both round-trip exactly.
template<typename T> static void _append_number(std::string& buf, T value) {
    char tmp[64];
    auto result = std::to_chars(tmp, tmp + sizeof(tmp), value);
    buf.append(tmp, result.ptr);
}

static bool _write_all(FILE* fp, const void* data, size_t size) {
    return size == 0 || fwrite(data, 1, size, fp) == size;
}

int _cwipc_ply_write(const char* filename, cwipc_pointcloud* pc, bool binary) {
    size_t dataSize = pc->get_uncompressed_size();
    std::vector<struct cwipc_point> points(dataSize / sizeof(struct cwipc_point));
    int npoints = 0;

    if (dataSize > 0) {
        npoints = pc->copy_uncompressed(points.data(), dataSize);

        if (npoints < 0) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_WRITE_MODULE, "Cannot copy points, size=" + std::to_string(dataSize));
            return -1;
        }
    }

    double timestamp_seconds = (double)pc->timestamp() / 1000.0;
    float cellsize = pc->cellsize();

    // "wb" also for ASCII: line endings are always LF, on every platform.
    FILE* fp = fopen(filename, "wb");

    if (fp == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_WRITE_MODULE, std::string("Cannot open output file: ") + filename);
        return -1;
    }

    std::string header = _ply_header(binary, npoints);
    bool ok = _write_all(fp, header.data(), header.size());

    if (binary) {
        ok = ok && _write_all(fp, points.data(), npoints * sizeof(struct cwipc_point));
        ok = ok && _write_all(fp, &timestamp_seconds, sizeof(timestamp_seconds));
        ok = ok && _write_all(fp, &cellsize, sizeof(cellsize));
    } else {
        std::string line;

        for (int i = 0; ok && i < npoints; i++) {
            const struct cwipc_point& p = points[i];
            line.clear();
            _append_number(line, p.x);
            line += ' ';
            _append_number(line, p.y);
            line += ' ';
            _append_number(line, p.z);
            line += ' ';
            _append_number(line, (int)p.r);
            line += ' ';
            _append_number(line, (int)p.g);
            line += ' ';
            _append_number(line, (int)p.b);
            line += ' ';
            _append_number(line, (int)p.tile);
            line += '\n';
            ok = _write_all(fp, line.data(), line.size());
        }

        line.clear();
        _append_number(line, timestamp_seconds);
        line += ' ';
        _append_number(line, cellsize);
        line += '\n';
        ok = ok && _write_all(fp, line.data(), line.size());
    }

    if (fclose(fp) != 0) {
        ok = false;
    }

    if (!ok) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_WRITE_MODULE, std::string("Error writing output file: ") + filename);
        return -1;
    }

    return 0;
}

//
// Reading.
//
// Accepts (see the decisions on #66): ASCII (LF or CRLF) or binary (either byte order),
// with a vertex element that has x, y, z in any numeric type, optionally uchar
// red/green/blue and the fourth byte as uchar tile or alpha. Everything else is skipped,
// except element cwipc_metadata (timestamp in seconds, cellsize).
//

enum class PlyType { Invalid, Int8, UInt8, Int16, UInt16, Int32, UInt32, Float32, Float64 };

static PlyType _ply_type(const std::string& name) {
    if (name == "char" || name == "int8") return PlyType::Int8;
    if (name == "uchar" || name == "uint8") return PlyType::UInt8;
    if (name == "short" || name == "int16") return PlyType::Int16;
    if (name == "ushort" || name == "uint16") return PlyType::UInt16;
    if (name == "int" || name == "int32") return PlyType::Int32;
    if (name == "uint" || name == "uint32") return PlyType::UInt32;
    if (name == "float" || name == "float32") return PlyType::Float32;
    if (name == "double" || name == "float64") return PlyType::Float64;
    return PlyType::Invalid;
}

static size_t _ply_type_size(PlyType t) {
    switch (t) {
        case PlyType::Int8: case PlyType::UInt8: return 1;
        case PlyType::Int16: case PlyType::UInt16: return 2;
        case PlyType::Int32: case PlyType::UInt32: case PlyType::Float32: return 4;
        case PlyType::Float64: return 8;
        default: return 0;
    }
}

struct PlyProperty {
    std::string name;
    PlyType type = PlyType::Invalid;        // value type (item type for lists)
    bool is_list = false;
    PlyType count_type = PlyType::Invalid;  // for lists
};

struct PlyElement {
    std::string name;
    size_t count = 0;
    std::vector<PlyProperty> properties;

    int find(const char* propname) const {
        for (size_t i = 0; i < properties.size(); i++) {
            if (properties[i].name == propname) return (int)i;
        }
        return -1;
    }
};

enum class PlyFormat { Ascii, BinaryLittleEndian, BinaryBigEndian };

// Sequential access to the data part of a PLY file, in any of the three formats.
class PlyDataReader {
public:
    PlyDataReader(const char* begin, const char* end, PlyFormat format)
    :   m_pos(begin), m_end(end), m_format(format)
    {
        // strtof/strtod use the C locale's decimal point; PLY always uses '.'.
        const char* dp = localeconv()->decimal_point;
        m_decimal_point = (dp && dp[0]) ? dp[0] : '.';
    }

    // Read one value of type t. Returns false on truncated or malformed data.
    bool read(PlyType t, double& value) {
        if (m_format == PlyFormat::Ascii) {
            return _read_ascii(t, value);
        }
        return _read_binary(t, value);
    }

    // Read (and discard) one property value, list or scalar.
    bool skip(const PlyProperty& prop) {
        double v;

        if (!prop.is_list) {
            return read(prop.type, v);
        }

        if (!read(prop.count_type, v) || v < 0) {
            return false;
        }

        size_t n = (size_t)v;

        if (m_format != PlyFormat::Ascii) {
            size_t nbytes = n * _ply_type_size(prop.type);
            if ((size_t)(m_end - m_pos) < nbytes) return false;
            m_pos += nbytes;
            return true;
        }

        for (size_t i = 0; i < n; i++) {
            if (!read(prop.type, v)) return false;
        }

        return true;
    }

    // Binary only: return a pointer to the next nbytes of data and skip them,
    // or NULL if the data is truncated.
    const char* take(size_t nbytes) {
        if ((size_t)(m_end - m_pos) < nbytes) return NULL;
        const char* rv = m_pos;
        m_pos += nbytes;
        return rv;
    }

private:
    bool _read_ascii(PlyType t, double& value) {
        while (m_pos < m_end && (*m_pos == ' ' || *m_pos == '\t' || *m_pos == '\r' || *m_pos == '\n')) {
            m_pos++;
        }

        const char* start = m_pos;

        while (m_pos < m_end && !(*m_pos == ' ' || *m_pos == '\t' || *m_pos == '\r' || *m_pos == '\n')) {
            m_pos++;
        }

        size_t len = m_pos - start;
        if (len == 0 || len >= sizeof(m_token)) return false;

        // Not std::from_chars: floating-point from_chars is missing from the libc++
        // of current Xcode and the Android NDK (deleted function), see #361.
        // strtof/strtod need a NUL-terminated string and the locale's decimal point.
        memcpy(m_token, start, len);
        m_token[len] = '\0';

        if (m_decimal_point != '.') {
            for (size_t i = 0; i < len; i++) {
                if (m_token[i] == '.') m_token[i] = m_decimal_point;
            }
        }

        char* end = NULL;

        // Parse floats as float, so the result is the float nearest to the text
        // (parsing as double and then converting could round twice).
        if (t == PlyType::Float32) {
            value = strtof(m_token, &end);
        } else {
            value = strtod(m_token, &end);
        }

        return end == m_token + len;
    }

    bool _read_binary(PlyType t, double& value) {
        size_t size = _ply_type_size(t);
        if (size == 0 || (size_t)(m_end - m_pos) < size) return false;
        unsigned char bytes[8];
        memcpy(bytes, m_pos, size);
        m_pos += size;

        // Host byte order is little-endian (checked at the top of this file).
        if (m_format == PlyFormat::BinaryBigEndian) {
            for (size_t i = 0; i < size / 2; i++) {
                std::swap(bytes[i], bytes[size - 1 - i]);
            }
        }

        switch (t) {
            case PlyType::Int8: { int8_t v; memcpy(&v, bytes, 1); value = v; break; }
            case PlyType::UInt8: { uint8_t v; memcpy(&v, bytes, 1); value = v; break; }
            case PlyType::Int16: { int16_t v; memcpy(&v, bytes, 2); value = v; break; }
            case PlyType::UInt16: { uint16_t v; memcpy(&v, bytes, 2); value = v; break; }
            case PlyType::Int32: { int32_t v; memcpy(&v, bytes, 4); value = v; break; }
            case PlyType::UInt32: { uint32_t v; memcpy(&v, bytes, 4); value = v; break; }
            case PlyType::Float32: { float v; memcpy(&v, bytes, 4); value = v; break; }
            case PlyType::Float64: { double v; memcpy(&v, bytes, 8); value = v; break; }
            default: return false;
        }

        return true;
    }

    const char* m_pos;
    const char* m_end;
    PlyFormat m_format;
    char m_decimal_point;
    char m_token[128];
};

static bool _read_file(const char* filename, std::vector<char>& data) {
    std::ifstream f(filename, std::ios::binary | std::ios::ate);
    if (!f) return false;
    std::streamsize size = f.tellg();
    if (size < 0) return false;
    f.seekg(0);
    data.resize((size_t)size);
    return size == 0 || (bool)f.read(data.data(), size);
}

static std::vector<std::string> _split(const std::string& line) {
    std::vector<std::string> words;
    size_t i = 0;

    while (i < line.size()) {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) i++;
        size_t start = i;
        while (i < line.size() && !(line[i] == ' ' || line[i] == '\t')) i++;
        if (i > start) words.push_back(line.substr(start, i - start));
    }

    return words;
}

// Parse the header. On success, data_start is the offset of the first data byte.
static bool _parse_header(const std::vector<char>& data, const char* filename, PlyFormat& format, std::vector<PlyElement>& elements, size_t& data_start) {
    size_t pos = 0;
    bool first = true;
    bool have_format = false;

    while (true) {
        size_t eol = pos;
        while (eol < data.size() && data[eol] != '\n') eol++;

        if (eol >= data.size()) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("Not a PLY file, or no end_header: ") + filename);
            return false;
        }

        std::string line(data.data() + pos, eol - pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        pos = eol + 1;

        if (first) {
            if (line != "ply") {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("Not a PLY file: ") + filename);
                return false;
            }
            first = false;
            continue;
        }

        std::vector<std::string> words = _split(line);
        if (words.empty()) continue;
        const std::string& keyword = words[0];

        if (keyword == "end_header") {
            break;
        } else if (keyword == "comment" || keyword == "obj_info") {
            continue;
        } else if (keyword == "format" && words.size() >= 2) {
            if (words[1] == "ascii") format = PlyFormat::Ascii;
            else if (words[1] == "binary_little_endian") format = PlyFormat::BinaryLittleEndian;
            else if (words[1] == "binary_big_endian") format = PlyFormat::BinaryBigEndian;
            else {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("Unknown PLY format ") + words[1] + ": " + filename);
                return false;
            }
            have_format = true;
        } else if (keyword == "element" && words.size() == 3) {
            PlyElement element;
            element.name = words[1];
            unsigned long long count = 0;
            auto r = std::from_chars(words[2].data(), words[2].data() + words[2].size(), count);
            if (r.ec != std::errc()) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("Bad element count in PLY header: ") + line);
                return false;
            }
            element.count = (size_t)count;
            elements.push_back(element);
        } else if (keyword == "property" && !elements.empty()) {
            PlyProperty prop;

            if (words.size() == 5 && words[1] == "list") {
                prop.is_list = true;
                prop.count_type = _ply_type(words[2]);
                prop.type = _ply_type(words[3]);
                prop.name = words[4];
            } else if (words.size() == 3) {
                prop.type = _ply_type(words[1]);
                prop.name = words[2];
            }

            if (prop.type == PlyType::Invalid || (prop.is_list && prop.count_type == PlyType::Invalid)) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("Bad property in PLY header: ") + line);
                return false;
            }

            elements.back().properties.push_back(prop);
        } else {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("Unexpected line in PLY header: ") + line);
            return false;
        }
    }

    if (!have_format) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("No format line in PLY header: ") + filename);
        return false;
    }

    data_start = pos;
    return true;
}

static bool _is_scalar_uchar(const PlyElement& element, int index) {
    return index >= 0 && !element.properties[index].is_list && element.properties[index].type == PlyType::UInt8;
}

// Does this vertex element have exactly the struct cwipc_point layout (#338)?
// float x, y, z; uchar red, green, blue; uchar tile (or alpha, as written by PCL).
static bool _is_cwipc_point_layout(const PlyElement& element) {
    static const char* names[] = { "x", "y", "z", "red", "green", "blue" };
    static const PlyType types[] = { PlyType::Float32, PlyType::Float32, PlyType::Float32, PlyType::UInt8, PlyType::UInt8, PlyType::UInt8 };

    if (element.properties.size() != 7) return false;

    for (int i = 0; i < 6; i++) {
        const PlyProperty& prop = element.properties[i];
        if (prop.is_list || prop.name != names[i] || prop.type != types[i]) return false;
    }

    const PlyProperty& fourth = element.properties[6];
    return !fourth.is_list && fourth.type == PlyType::UInt8 && (fourth.name == "tile" || fourth.name == "alpha");
}

static uint8_t _to_uchar(double v) {
    if (!(v >= 0)) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

cwipc_pointcloud* _cwipc_ply_read(const char* filename, uint64_t timestamp, char** errorMessage) {
    std::vector<char> data;

    if (!_read_file(filename, data)) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("Cannot read file: ") + filename);
        return NULL;
    }

    PlyFormat format = PlyFormat::Ascii;
    std::vector<PlyElement> elements;
    size_t data_start = 0;

    if (!_parse_header(data, filename, format, elements, data_start)) {
        return NULL;
    }

    PlyDataReader reader(data.data() + data_start, data.data() + data.size(), format);
    std::vector<struct cwipc_point> points;
    // Fast path: the vertex data in the file buffer, if it has the cwipc_point layout.
    const struct cwipc_point* fast_points = NULL;
    size_t fast_count = 0;
    bool have_vertices = false;
    bool have_metadata_timestamp = false;
    double metadata_timestamp = 0;
    bool have_metadata_cellsize = false;
    double metadata_cellsize = 0;

    for (const PlyElement& element : elements) {
        bool is_vertex = element.name == "vertex" && !have_vertices;
        bool is_metadata = element.name == "cwipc_metadata";
        // Index into the element's properties for each cwipc_point field, or -1.
        int ix = -1, iy = -1, iz = -1, ired = -1, igreen = -1, iblue = -1, itile = -1;
        int its = -1, icellsize = -1;

        if (is_vertex && format == PlyFormat::BinaryLittleEndian && _is_cwipc_point_layout(element)) {
            fast_points = (const struct cwipc_point*)reader.take(element.count * sizeof(struct cwipc_point));

            if (fast_points == NULL) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("PLY file truncated (element vertex): ") + filename);
                return NULL;
            }

            fast_count = element.count;
            have_vertices = true;
            cwipc_log(CWIPC_LOG_LEVEL_DEBUG, PLY_READ_MODULE, std::string("fast path for vertex data: ") + filename);
            continue;
        }

        if (is_vertex) {
            ix = element.find("x");
            iy = element.find("y");
            iz = element.find("z");

            if (ix < 0 || iy < 0 || iz < 0 || element.properties[ix].is_list || element.properties[iy].is_list || element.properties[iz].is_list) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("PLY vertex element has no x, y, z: ") + filename);
                return NULL;
            }

            int r = element.find("red"), g = element.find("green"), b = element.find("blue");

            if (_is_scalar_uchar(element, r) && _is_scalar_uchar(element, g) && _is_scalar_uchar(element, b)) {
                ired = r;
                igreen = g;
                iblue = b;
            } else if (r >= 0 || g >= 0 || b >= 0) {
                cwipc_log(CWIPC_LOG_LEVEL_WARNING, PLY_READ_MODULE, std::string("red/green/blue are not all of type uchar, which is the only colour type cwipc_read supports; colours ignored: ") + filename);
            } else if (element.find("f_dc_0") >= 0) {
                cwipc_log(CWIPC_LOG_LEVEL_WARNING, PLY_READ_MODULE, std::string("Gaussian splat colours (f_dc_*) are not supported by cwipc_read; colours ignored: ") + filename);
            }

            int t = element.find("tile");
            if (t < 0) t = element.find("alpha");
            if (_is_scalar_uchar(element, t)) itile = t;

            points.resize(element.count);
        } else if (is_metadata) {
            its = element.find("timestamp");
            icellsize = element.find("cellsize");
            if (its >= 0 && element.properties[its].is_list) its = -1;
            if (icellsize >= 0 && element.properties[icellsize].is_list) icellsize = -1;
        }

        for (size_t n = 0; n < element.count; n++) {
            struct cwipc_point point = {};

            for (int i = 0; i < (int)element.properties.size(); i++) {
                const PlyProperty& prop = element.properties[i];
                bool ok;

                if (prop.is_list || !(is_vertex || is_metadata)) {
                    ok = reader.skip(prop);
                } else {
                    double v;
                    ok = reader.read(prop.type, v);

                    if (ok && is_vertex) {
                        if (i == ix) point.x = (float)v;
                        else if (i == iy) point.y = (float)v;
                        else if (i == iz) point.z = (float)v;
                        else if (i == ired) point.r = _to_uchar(v);
                        else if (i == igreen) point.g = _to_uchar(v);
                        else if (i == iblue) point.b = _to_uchar(v);
                        else if (i == itile) point.tile = _to_uchar(v);
                    } else if (ok && is_metadata && n == 0) {
                        if (i == its) { metadata_timestamp = v; have_metadata_timestamp = true; }
                        else if (i == icellsize) { metadata_cellsize = v; have_metadata_cellsize = true; }
                    }
                }

                if (!ok) {
                    cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("PLY file truncated or malformed (element ") + element.name + "): " + filename);
                    return NULL;
                }
            }

            if (is_vertex) {
                points[n] = point;
            }
        }

        if (is_vertex) {
            have_vertices = true;
        }
    }

    if (!have_vertices) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, PLY_READ_MODULE, std::string("PLY file has no vertex element: ") + filename);
        return NULL;
    }

    // A non-zero timestamp from the caller wins; otherwise the file's (seconds -> milliseconds).
    if (timestamp == 0 && have_metadata_timestamp && metadata_timestamp > 0) {
        timestamp = (uint64_t)std::llround(metadata_timestamp * 1000.0);
    }

    // cwipc_from_points copies the points, so the file buffer can go afterwards.
    // (It takes a non-const pointer, but only reads from it.)
    struct cwipc_point* from = fast_points ? (struct cwipc_point*)fast_points : points.data();
    size_t count = fast_points ? fast_count : points.size();
    cwipc_pointcloud* rv = cwipc_from_points(from, count * sizeof(struct cwipc_point), (int)count, timestamp, errorMessage, CWIPC_API_VERSION);

    if (rv != NULL && have_metadata_cellsize) {
        rv->_set_cellsize((float)metadata_cellsize);
    }

    return rv;
}
