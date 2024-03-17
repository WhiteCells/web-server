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
    if (stat((src_dir_ + path_).data(), &mm_file_stat_) < 0
        || S_ISDIR(mm_file_stat_.st_mode)) {
        code_ = 404;
    } else if (!(mm_file_stat_.st_mode & S_IROTH)) {
        code_ = 403;
    } else if (code_ == -1) {
        code_ = 200;
    }
    errorHtml_();
    addStateLine_(buff);
    addHeader_(buff);
    addContent_(buff);
}

void HttpResponse::unmapFile() {
    if (mm_file_) {
        munmap(mm_file_, mm_file_stat_.st_size);
        mm_file_ = nullptr;
    }
}

char *HttpResponse::file() {
    return mm_file_;
}

size_t HttpResponse::fileLen() const {
    return mm_file_stat_.st_size;
}

void HttpResponse::errorContent(Buffer &buff, std::string message) {
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

int HttpResponse::code() const {
    return code_;
}

void HttpResponse::addStateLine_(Buffer &buff) {
    std::string status;
    if (CODE_STATUS.count(code_) == -1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::addHeader_(Buffer &buff) {
    buff.append("connection: ");
    if (is_keep_alive_) {
        buff.append("keep alive\r\n");
        buff.append("keep alive: max = 6, timeout = 120\r\n");
    } else {
        buff.append("close\r\n");
    }
    buff.append("content type: " + getFileType_() + "\r\n");
}

void HttpResponse::addContent_(Buffer &buff) {
    int src_fd = open((src_dir_ + path_).data(), O_RDONLY);
    if (src_fd < 0) {
        errorContent(buff, "File Not Found");
        return;
    }

    LOG_DEBUG("file path: %s", (src_dir_ + path_).data());
    int *mm_ret = (int *)mmap(0, mm_file_stat_.st_size,
                              PROT_READ, MAP_PRIVATE, src_fd, 0);
    if (*mm_ret == -1) {
        errorContent(buff, "file not found");
        return;
    }
    mm_file_ = (char *)mm_ret;
    close(src_fd);
    buff.append("Content length: "
                + std::to_string(mm_file_stat_.st_size) + "\r\n\r\n");
}

void HttpResponse::errorHtml_() {
    if (CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((src_dir_ + path_).data(), &mm_file_stat_);
    }
}

std::string HttpResponse::getFileType_() {
    std::string::size_type idx = path_.find_last_of(".");
    if (idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}