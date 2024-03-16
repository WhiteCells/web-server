/*
 * @Author: WhiteCells
 * @Date: 2024-03-15 10:30:05
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-15 10:30:05
 * @Description:
 */

#include "http_response.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    {".html",   "text/html"},
    {".css",    "text/css"},
    {".js",     "text/javascript"},
    {".txt",    "text/plain"},
    {".xml",    "text/xml"},
    {".xhtml",  "application/xhtml+xml"},
    {".rtf",    "application/rtf"},
    {".gz",     "application/x-gzip"},
    {".pdf",    "application/pdf"},
    {".wrod",   "application/nsword"},
    {".tar",    "application/x-tar"},
    {".png",    "image/png"},
    {".gif",    "image/gif"},
    {".jpg",    "image/jpg"},
    {".jpeg",   "image/jpeg"},
    {".mpeg",   "video/mpeg"},
    {".mpg",    "video/mpeg"},
    {".avi",    "video/x-msvideo"},
    {".au",     "audio/basic"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS =  {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = src_dir_;
    is_keep_alive_ = false;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
};

HttpResponse::~HttpResponse() {
    unmapFile();
}

void HttpResponse::init(const std::string &src_dir,
                        std::string &path,
                        bool is_keep_alive,
                        int code) {
    assert(!src_dir_.empty());
    if (mm_file_) {
        unmapFile();
    }
    code_ = code;
    is_keep_alive_ = is_keep_alive;
    src_dir_ = src_dir;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
}

void HttpResponse::makeResponse(Buffer &buff) {

}

void HttpResponse::unmapFile() {

}

char *HttpResponse::file() {

}

size_t HttpResponse::fileLen() const {

}

void HttpResponse::errorConnect(Buffer &buff, std::string message) {
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    
    body += std::to_string(code_) + ":" + status + "\n";
}