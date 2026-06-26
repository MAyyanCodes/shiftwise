#include "../include/middleware/AuthMiddleware.h"
#include <jwt-cpp/jwt.h>
#include <iostream>
using namespace std;

// The secret key used for signing all tokens
const string AuthMiddleware::SECRET_KEY = "shiftwise_secret_2024";

// ── verifyJWT ─────────────────────────────────
// Decodes and verifies a JWT string.
// Returns AuthContext with valid=false if anything is wrong.
AuthContext AuthMiddleware::verifyJWT(const string& token) {
    AuthContext ctx;
    if (token.empty()) return ctx;

    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{SECRET_KEY})
                            .with_issuer("shiftwise");
        verifier.verify(decoded);

        ctx.userId = decoded.get_payload_claim("userId").as_string();
        ctx.role   = decoded.get_payload_claim("role").as_string();
        ctx.valid  = true;
        return ctx;

    } catch (exception& e) {
        cerr << "verifyJWT failed: " << e.what() << endl;
        return ctx;
    }
}

// ── generateJWT ───────────────────────────────
// Creates a signed JWT for the given userId and role.
string AuthMiddleware::generateJWT(const string& userId,
                                    const string& role,
                                    int expiryHours) {
    auto token = jwt::create()
                     .set_issuer("shiftwise")
                     .set_type("JWT")
                     .set_payload_claim("userId", jwt::claim(userId))
                     .set_payload_claim("role",   jwt::claim(role))
                     .set_expires_at(chrono::system_clock::now() +
                                     chrono::hours(expiryHours))
                     .sign(jwt::algorithm::hs256{SECRET_KEY});
    return token;
}

// ── authenticate ─────────────────────────────
// Reads Authorization header and verifies the JWT.
AuthContext AuthMiddleware::authenticate(const crow::request& req) {
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty()) return AuthContext{};

    string token = authHeader;
    if (authHeader.substr(0, 7) == "Bearer ") {
        token = authHeader.substr(7);
    }
    return verifyJWT(token);
}

// ── requireAuth ───────────────────────────────
// Any logged-in user may pass.
crow::response AuthMiddleware::requireAuth(
    const crow::request& req,
    function<crow::response(const AuthContext&)> handler)
{
    AuthContext ctx = authenticate(req);
    if (!ctx.valid) {
        return crow::response(401, "{\"error\":\"Authentication required\"}");
    }
    return handler(ctx);
}

// ── requireManager ────────────────────────────
// Only managers and admins may pass.
crow::response AuthMiddleware::requireManager(
    const crow::request& req,
    function<crow::response(const AuthContext&)> handler)
{
    AuthContext ctx = authenticate(req);
    if (!ctx.valid) {
        return crow::response(401, "{\"error\":\"Authentication required\"}");
    }
    if (ctx.role != "manager" && ctx.role != "admin") {
        return crow::response(403, "{\"error\":\"Manager access required\"}");
    }
    return handler(ctx);
}

// ── requireEmployee ───────────────────────────
// Only employees may pass.
crow::response AuthMiddleware::requireEmployee(
    const crow::request& req,
    function<crow::response(const AuthContext&)> handler)
{
    AuthContext ctx = authenticate(req);
    if (!ctx.valid) {
        return crow::response(401, "{\"error\":\"Authentication required\"}");
    }
    if (ctx.role != "employee") {
        return crow::response(403, "{\"error\":\"Employee access required\"}");
    }
    return handler(ctx);
}
