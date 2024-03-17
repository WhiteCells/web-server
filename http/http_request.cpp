/*
 * @Author: WhiteCells
 * @Date: 2024-03-13 10:12:29
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-13 10:12:29
 * @Description:
*/

#include "http_request.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML {
    "/index",
    "/welcome",
    "/video",
    "/picture",
    "/register",
    "/login",
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG {
    {"/register.html", 0},
    {"/login.html", 1},
};

HttpRequest::HttpRequest() {
    init();
}

void HttpRequest::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::parse(Buffer &buff) {
    const char CRLF[] = "\r\n";
    if (buff.getReadableBytes() <= 0) {
        return false;
    }
    while (buff.getReadableBytes() && state_ != FINISH) {
        const char *line_end = std::search(buff.peek(), buff.beginWriteConst(),
                                           CRLF, CRLF + 2);
        std::string line(buff.peek(), line_end);
        switch (state_) {
            case REQUEST_LINE:
                if (!parseRequestLine_(line)) {
                    return false;
                }
                parsePath_();
                break;
            case HEADERS:
                parseHeader_(line);
                if (buff.getReadableBytes() <= 2) {
                    state_ = FINISH;
                }
                break;
            case BODY:
                parseBody_(line);
                break;
            default:
                break;
        }
        if (line_end == buff.beginWrite()) {
            break;
        }
        buff.retrieveUntil(line_end + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

std::string HttpRequest::path() const {
    return path_;
}

std::string &HttpRequest::path() {
    return path_;
}

std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::getPost(const std::string &key) const {
    assert(!key.empty());
    if (post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::getPost(const char *key) const {
    assert(key);
    if (post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

bool HttpRequest::isKeepAlive() const {
    if (header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive"
            && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::userVerify(const std::string &name,
                             const std::string &passwd,
                             bool is_login) {
    if (name.empty() || passwd.empty()) {
        return false;
    }
    LOG_INFO("Verify name: %s pwd: %s", name.c_str(), passwd.c_str());
    MYSQL *sql;
    SqlConnectRAII(&sql, SqlConnectPool::instance());
    assert(sql);

    bool flag = false;
    unsigned j = 0;
    char order[256] {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!is_login) {
        flag = true;
    }
    snprintf(order, 256,
             "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
             name.c_str());
    LOG_DEBUG("%s", order);

    if (mysql_query(sql, order)) {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        if (is_login) {
            if (passwd == password) {
                flag = true;
            } else {
                flag = false;
                LOG_DEBUG("password error");
            }
        } else {
            flag = false;
            LOG_DEBUG("user used");
        }
    }
    mysql_free_result(res);

    if (!is_login && flag == true) {
        LOG_DEBUG("regirster");
        bzero(order, 256);
        snprintf(order, 256,
                 "INSERT INTO user(username, password) VALUES('%s', '%s')",
                 name.c_str(),
                 passwd.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order)) {
            LOG_DEBUG("Insert error");
            flag = false;
        }
        flag = true;
    }

    SqlConnectPool::instance()->freeConnect(sql);
    LOG_DEBUG("userVerify success");
    return flag;
}

int HttpRequest::converHexToDec(char ch) {
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    } else if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return ch;
}

bool HttpRequest::parseRequestLine_(const std::string &line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch sub_match;
    if (std::regex_match(line, sub_match, patten)) {
        method_ = sub_match[1];
        path_ = sub_match[2];
        version_ = sub_match[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine error");
    return false;
}

void HttpRequest::parseHeader_(const std::string &line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch sub_match;
    if (std::regex_match(line, sub_match, patten)) {
        header_[sub_match[1]] = sub_match[2];
    } else {
        state_ = BODY;
    }
}

void HttpRequest::parseBody_(const std::string &line) {
    body_ = line;
    parsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body: %s, len: %d", line.c_str(), line.size());
}

void HttpRequest::parsePath_() {
    if (path_ == "/") {
        path_ = "/index.html";
    } else {
        for (auto &item : DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

void HttpRequest::parsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        parseFromUrlEncoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag: %d", tag);
            if (tag == 0 || tag == 1) {
                bool is_login = (tag == 1);
                if (userVerify(post_["username"], post_["password"], is_login)) {
                    path_ = "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::parseFromUrlEncoded_() {
    if (body_.size() == 0) {
        return;
    }

    std::string key, value;
    int num = 0;
    int i = 0, j = 0;
    for (; i < body_.size(); ++i) {
        char ch = body_[i];
        switch (ch) {
            case '=':
                key = body_.substr(j, i - j);
                break;
            case '+':
                body_[i] = ' ';
                break;
            case '%':
                num = converHexToDec(body_[i + 1]) * 16 + converHexToDec(body_[i + 2]);
                body_[i + 2] = num % 10 + '0';
                body_[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = body_.substr(j, i - j);
                j = i + 1;
                post_[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if (post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i = j);
        post_[key] = value;
    }
}