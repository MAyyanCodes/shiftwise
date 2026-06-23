#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H
#include <crow.h>
#include <pqxx/pqxx>
#include <string>
using namespace std;

class AuthController {
private:
    string dbConnection;
    string jwtSecret;

    string hashPassword(string plainText);
    bool   checkPassword(string plainText, string hash);
    string createToken(int userId, string role);

public:
    AuthController(string connStr, string secret) {
        dbConnection = connStr;
        jwtSecret    = secret;
    }

    crow::response signup(const crow::request& req);
    crow::response login(const crow::request& req);

    // Admin approval endpoints
    crow::response getPendingManagers(const crow::request& req);
    crow::response updateManagerStatus(const crow::request& req, string userId, string newStatus);
};

#endif
