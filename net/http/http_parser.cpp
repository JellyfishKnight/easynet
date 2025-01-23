#include "http_parser.hpp"

namespace net {

std::vector<uint8_t> HttpParser::write_req(const HttpRequest& req) {
    m_req_writer.reset_state();
    m_req_writer.begin_header(utils::dump_enum(req.method), req.url);
    for (auto& [key, value]: req.headers) {
        m_req_writer.write_header(key, value);
    }
    m_req_writer.end_header();
    m_req_writer.write_body(req.body);
    return std::vector<uint8_t>(m_req_writer.buffer().begin(), m_req_writer.buffer().end());
}

std::vector<uint8_t> HttpParser::write_res(const HttpResponse& res) {
    m_res_writer.reset_state();
    m_res_writer.begin_header(static_cast<int>(res.status_code));
    for (auto& [key, value]: res.headers) {
        m_res_writer.write_header(key, value);
    }
    if (!res.body.empty() && !res.headers.contains("Content-Length")) {
        m_res_writer.write_header("Content-Length", std::to_string(res.body.size()));
    }
    m_res_writer.end_header();
    m_res_writer.write_body(res.body);
    return std::vector<uint8_t>(m_res_writer.buffer().begin(), m_res_writer.buffer().end());
}

void HttpParser::read_req(std::vector<uint8_t>& req, HttpRequest& out_req) {
    m_req_parser.push_chunk(std::string_view(reinterpret_cast<char*>(req.data()), req.size()));
    if (m_req_parser.request_finished()) {
        out_req.method = m_req_parser.method();
        out_req.url = m_req_parser.url();
        out_req.version = m_req_parser.version();
        out_req.headers = m_req_parser.headers();
        out_req.body = std::move(m_req_parser.body());
    }
}

void HttpParser::read_res(std::vector<uint8_t>& res, HttpResponse& out_res) {
    m_res_parser.push_chunk(std::string_view(reinterpret_cast<char*>(res.data()), res.size()));
    if (m_res_parser.request_finished()) {
        out_res.version = m_res_parser.version();
        out_res.status_code = static_cast<HttpResponseCode>(m_res_parser.status());
        out_res.reason = utils::dump_enum(out_res.status_code);
        out_res.headers = m_res_parser.headers();
        out_res.body = std::move(m_res_parser.body());
    }
}

bool HttpParser::req_read_finished() {
    return m_req_parser.request_finished();
}

bool HttpParser::res_read_finished() {
    return m_res_parser.request_finished();
}

void HttpParser::reset_state() {
    m_res_parser.reset_state();
    m_req_parser.reset_state();
}

} // namespace net