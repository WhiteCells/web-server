/*
 * @Author: WhiteCells
 * @Date: 2024-03-15 10:31:43
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-15 10:31:43
 * @Description: 
*/

#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sql_connect_pool.h"
#include "../pool/sql_connect_raii.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <mysql/mysql.h>
#include <errno.h>

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HttpRequest();
    ~HttpRequest() = default;

    void init();
    bool parse(Buffer &buff);

    std::string path() const;
    std::string &path();
    std::string method() const;
    std::string version() const;
    std::string getPost(const std::string &key) const;
    std::string getPost(const char *key) const;

    bool isKeepAlive() const;

private:
    static bool userVerify(const std::string &name,
                           const std::string &passwd,
                           bool is_login);
    static int converHexToDec(char ch);

    bool parseRequestLine_(const std::string &line);
    void parseHeader_(const std::string &line);
    void parseBody_(const std::string &line);

    void parsePath_();
    void parsePost_();
    void parseFromUrlEncoded_();

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
};

#endif // _HTTP_REQUEST_H_