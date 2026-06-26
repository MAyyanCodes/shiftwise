#ifndef AUTH_MIDDLEWARE_H
#define AUTH_MIDDLEWARE_H

#include <crow.h>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

struct AuthContext {
    string userId;
    string role;
    bool   valid = false;
};

class AuthMiddleware {
private:
    static const string SECRET_KEY;
    string instanceSecret;

public:
    // ── Instance constructor (used by controllers) ──
    AuthMiddleware() : instanceSecret("") {}
    AuthMiddleware(const string& secret) : instanceSecret(secret) {}

    // ── Instance methods (what controllers call) ────

    // Pulls the raw token string out of Authorization header
    string extractToken(const crow::request& req) const {
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.empty()) return "";
        if (authHeader.substr(0, 7) == "Bearer ") {
            return authHeader.substr(7);
        }
        return authHeader;
    }

    // Returns true if the token is valid (any role)
    bool isAuthenticated(const string& token) const {
        if (token.empty()) return false;
        AuthContext ctx = verifyJWT(token);
        return ctx.valid;
    }

    // Returns true if the token belongs to a manager or admin
    bool isManager(const string& token) const {
        if (token.empty()) return false;
        AuthContext ctx = verifyJWT(token);
        return ctx.valid && (ctx.role == "manager" || ctx.role == "admin");
    }

    // Returns true if the token belongs to an employee
    bool isEmployee(const string& token) const {
        if (token.empty()) return false;
        AuthContext ctx = verifyJWT(token);
        return ctx.valid && ctx.role == "employee";
    }

    // ── Static methods (used by AuthController) ─────

    static AuthContext authenticate(const crow::request& req);

    static crow::response requireAuth(
        const crow::request& req,
        function<crow::response(const AuthContext&)> handler
    );

    static crow::response requireManager(
        const crow::request& req,
        function<crow::response(const AuthContext&)> handler
    );

    static crow::response requireEmployee(
        const crow::request& req,
        function<crow::response(const AuthContext&)> handler
    );

    static string generateJWT(const string& userId,
                               const string& role,
                               int expiryHours = 24);

    static AuthContext verifyJWT(const string& token);
};

#endif // AUTH_MIDDLEWARE_H
